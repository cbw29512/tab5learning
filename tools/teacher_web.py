"""Loopback-only Teacher Studio HTTP adapter; no USB or flash commands."""
import csv
import hashlib
import hmac
import logging
import secrets
import sqlite3
import sys
from http.cookies import SimpleCookie, CookieError
from urllib.parse import parse_qs
from uuid import uuid4

from tools.teacher_accounts import AccountStore
from tools.teacher_sessions import TeacherSessions
from tools.teacher_decks import TeacherDecks
from tools.teacher_registry import TeacherRegistry
from tools.teacher_device_enrollment import DeviceEnrollment
from tools.credential_migration import migrate_legacy
from tools.windows_credential_protection import WindowsProtector
from tools.teacher_usb_discovery import scan_usb, scan_result
from tools.teacher_usb_identity import identify, identity_panel
from tools.flashcard_deck import import_csv, MAX_BYTES
from tools.teacher_deck_editor import Draft, editor, handle_editor, FORM_FIELDS
from tools import teacher_web_views as views
from tools.teacher_bulk_flashcards import bulk_form, handle_bulk
from tools.teacher_web_security import (LOGIN_COOKIE, LOGIN_TTL, browser_origin_ok,
                                        login_challenge, valid_login_challenge)

LOG = logging.getLogger(__name__)
BIND_ADDRESS = "127.0.0.1"
PORT = 18765
HOST = f"{BIND_ADDRESS}:{PORT}"
ORIGIN = "http://" + HOST
COOKIE = "pocket_teacher"
MAX_FORM = MAX_BYTES * 3 + 4096


class TeacherWeb:
    def __init__(self, directory, *, test_mode=False):
        directory.mkdir(parents=True, exist_ok=True)
        self.test_mode = test_mode
        self.accounts = AccountStore(directory / "accounts.sqlite", test_mode=test_mode)
        self.sessions = TeacherSessions(self.accounts)
        self.decks = TeacherDecks(directory / "decks.sqlite", self.sessions)
        self.registry = TeacherRegistry(directory / "registry.sqlite", self.sessions) if test_mode else None
        # Windows test enrollment uses DPAPI for new keys; startup fails closed
        # if the native provider cannot be initialized. Legacy records stay pending.
        protector = WindowsProtector() if test_mode and sys.platform == "win32" else None
        self.enrollment = DeviceEnrollment(directory / "credentials.sqlite", self.sessions,
                                           self.registry, protector=protector) if test_mode else None
        self.csrf_key = secrets.token_bytes(32)

    def csrf(self, token):
        return hmac.new(self.csrf_key, token.encode(), hashlib.sha256).hexdigest()

    @staticmethod
    def token(env, name=COOKIE):
        try:
            cookies = SimpleCookie(env.get("HTTP_COOKIE", ""))
            return cookies[name].value if name in cookies else None
        except CookieError:
            return None

    @staticmethod
    def respond(start, status, body="", headers=()):
        raw = body.encode("utf-8")
        start(status, [("Content-Type", "text/html; charset=utf-8"),
                       ("Content-Length", str(len(raw))),
                       ("Cache-Control", "no-store"),
                       ("X-Content-Type-Options", "nosniff"),
                       ("X-Frame-Options", "DENY"),
                       # Preserve Origin for same-origin HTML form POSTs.
                       ("Referrer-Policy", "same-origin"),
                       ("Content-Security-Policy",
                        "default-src 'none'; style-src 'unsafe-inline'; form-action 'self'; frame-ancestors 'none'; base-uri 'none'"),
                       *headers])
        return [raw]

    def redirect(self, start, token=None, clear=False):
        headers = [("Location", "/")]
        if token or clear:
            value = token or ""
            age = "; Max-Age=0" if clear else ""
            headers.append(("Set-Cookie", f"{COOKIE}={value}; Path=/; HttpOnly; SameSite=Strict{age}"))
        return self.respond(start, "303 See Other", headers=headers)

    @staticmethod
    def form(env, *, max_fields=5):
        if env.get("CONTENT_TYPE", "").split(";")[0] != "application/x-www-form-urlencoded":
            raise ValueError("Unsupported form format")
        length = int(env.get("CONTENT_LENGTH") or "0")
        if not 0 < length <= MAX_FORM:
            raise ValueError("Invalid body size")
        raw = env["wsgi.input"].read(length)
        if len(raw) != length:
            raise ValueError("Incomplete form")
        data = parse_qs(raw.decode("utf-8"), strict_parsing=True,
                        keep_blank_values=True, max_num_fields=max_fields, errors="strict")
        if any(len(v) != 1 for v in data.values()):
            raise ValueError("Repeated field")
        return {k: v[0] for k, v in data.items()}

    def login_page(self, start, notice="", status="200 OK"):
        nonce, proof = login_challenge(self.csrf_key)
        cookie = f"{LOGIN_COOKIE}={nonce}; Path=/login; HttpOnly; SameSite=Strict; Max-Age={LOGIN_TTL}"
        return self.respond(start, status, views.login(notice, test_mode=self.test_mode, csrf=proof),
                            headers=[("Set-Cookie", cookie)])

    def dashboard(self, token, notice=""):
        user = self.sessions.require(token)
        enrollment_admin = self.enrollment is not None and self.accounts.role(user) == "admin"
        return views.workspace(user, self.accounts.role(user),
                               self.decks.list_decks(token), self.csrf(token), notice,
                               minimum=self.accounts.minimum_password_length, test_mode=self.test_mode,
                               assignments=self.registry.list_assignments(token) if self.registry else None,
                               enrollment=self.enrollment.pending(token) if enrollment_admin else None,
                               credential_storage=self.enrollment.storage_status(token) if enrollment_admin else None,
                               can_migrate=bool(enrollment_admin and self.enrollment.protector is not None))

    def __call__(self, env, start):
        # Exact host and origin checks prevent DNS rebinding and cross-site writes.
        try:
            if env.get("HTTP_HOST") != HOST or env.get("REMOTE_ADDR") != "127.0.0.1":
                return self.respond(start, "403 Forbidden", "Local access only.")
            method, path = env["REQUEST_METHOD"], env["PATH_INFO"]
            token = self.token(env)
            if method == "GET" and path == "/login":
                return self.login_page(start)
            if method == "GET" and path == "/":
                try:
                    body = self.dashboard(token)
                except PermissionError:
                    return self.login_page(start)
                return self.respond(start, "200 OK", body)
            if method == "GET" and path == "/decks/bulk":
                try:
                    self.sessions.require(token)
                except PermissionError:
                    return self.login_page(start)
                return self.respond(start, "200 OK", views.page("Bulk flashcards", bulk_form(self.csrf(token))))
            if method == "GET" and path == "/decks/new":
                try:
                    self.sessions.require(token)
                except PermissionError:
                    return self.login_page(start)
                return self.respond(start, "200 OK", views.page("Create flashcards", editor(Draft.new(), self.csrf(token))))
            if method != "POST" or path not in {"/login", "/decks", "/teachers", "/logout", "/assignments", "/usb-scan", "/usb-identify", "/enrollment", "/protect-credential", "/decks/new", "/decks/edit", "/decks/bulk"}:
                return self.respond(start, "404 Not Found", "Not found.")
            if not browser_origin_ok(env, ORIGIN):
                LOG.warning("Form origin rejected: missing=%s, null=%s",
                            not env.get("HTTP_ORIGIN"), env.get("HTTP_ORIGIN") == "null")
                return self.respond(start, "403 Forbidden", views.page(
                    "Request rejected", '<p><a href="/">Return to sign in</a> and submit a fresh form.</p>'))
            data = self.form(env, max_fields=FORM_FIELDS if path == "/decks/new" else 5)
            if path == "/login":
                if not valid_login_challenge(self.csrf_key, self.token(env, LOGIN_COOKIE), data.get("csrf")):
                    return self.login_page(start, "Login form expired. Please sign in again.", "403 Forbidden")
                self.sessions.logout(token)
                new_token = self.sessions.login(data["username"], data["password"])
                if not new_token:
                    return self.login_page(start, "Invalid credentials or account locked.", "401 Unauthorized")
                return self.redirect(start, new_token)
            self.sessions.require(token)
            if not hmac.compare_digest(data.get("csrf", ""), self.csrf(token)):
                raise PermissionError("CSRF mismatch")
            if path == "/decks/bulk":
                body, saved = handle_bulk(data, csrf=self.csrf(token), key=self.csrf_key,
                                          token=token, decks=self.decks)
                if saved:
                    return self.respond(start, "200 OK", self.dashboard(token, "Flashcard deck saved successfully."))
                return self.respond(start, "200 OK", views.page("Bulk flashcards", body))
            if path == "/decks/edit":
                try:
                    draft = Draft.existing(self.decks.latest(token, data["deck_id"]))
                except ValueError as exc:
                    return self.respond(start, "200 OK", self.dashboard(token, str(exc)))
                return self.respond(start, "200 OK", views.page("Edit flashcards", editor(draft, self.csrf(token))))
            if path == "/decks/new":
                draft, preview, error, saved = handle_editor(data, self.csrf(token), self.decks, token)
                if saved:
                    return self.respond(start, "200 OK", self.dashboard(token, "Flashcard deck saved successfully."))
                return self.respond(start, "200 OK", views.page("Edit flashcards" if draft.base_version else "Create flashcards", editor(draft, self.csrf(token), preview=preview, error=error)))
            if path == "/logout":
                self.sessions.logout(token)
                return self.redirect(start, clear=True)
            if path == "/usb-scan":
                if not self.test_mode:
                    raise PermissionError("USB discovery is test-only")
                self.sessions.require(token, "admin")
                return self.respond(start, "200 OK", views.page("USB discovery", scan_result(scan_usb(), self.csrf(token))))
            if path == "/usb-identify":
                if not self.test_mode:
                    raise PermissionError("USB identification is test-only")
                self.sessions.require(token, "admin")
                return self.respond(start, "200 OK", views.page("Learner identification", identity_panel(identify(data["port"]))))
            if path == "/protect-credential":
                if not self.enrollment:
                    raise PermissionError("Migration is test-only")
                migrate_legacy(self.enrollment, token, data["device_id"],
                               confirmed=data.get("confirm") == "yes")
            elif path == "/enrollment":
                if not self.enrollment:
                    raise PermissionError("Enrollment is test-only")
                self.enrollment.prepare(token, data["device_id"])
            elif path == "/assignments":
                if not self.registry:
                    raise PermissionError("Registry is test-only")
                self.registry.create(token, data["student_id"], data["device_id"], int(data["grade"]))
            elif path == "/teachers":
                self.sessions.create_account(token, data["username"], data["password"], "teacher")
            else:
                deck = import_csv(data["csv"], uuid4().hex, data["title"])
                self.decks.save(token, deck)
            return self.redirect(start)
        except PermissionError:
            LOG.warning("Web operation authorization denied")
            return self.respond(start, "403 Forbidden", "Access denied. Return to the dashboard.")
        except (ValueError, KeyError, UnicodeError, csv.Error, sqlite3.IntegrityError):
            LOG.warning("Invalid web form or duplicate record")
            return self.respond(start, "400 Bad Request", "Invalid form or duplicate record.")
        except Exception:
            LOG.exception("Local teacher web operation failed")
            return self.respond(start, "500 Internal Server Error", "Local operation failed.")
