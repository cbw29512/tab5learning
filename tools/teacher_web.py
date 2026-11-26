"""Loopback-only Teacher Studio HTTP adapter; no USB or flash commands."""
import hashlib
import hmac
import logging
import secrets
import sqlite3
from http.cookies import SimpleCookie, CookieError
from urllib.parse import parse_qs
from uuid import uuid4

from tools.teacher_accounts import AccountStore
from tools.teacher_sessions import TeacherSessions
from tools.teacher_decks import TeacherDecks
from tools.flashcard_deck import import_csv, MAX_BYTES
from tools import teacher_web_views as views

LOG = logging.getLogger(__name__)
HOST = "127.0.0.1:8765"
ORIGIN = "http://" + HOST
COOKIE = "pocket_teacher"
MAX_FORM = MAX_BYTES * 3 + 4096


class TeacherWeb:
    def __init__(self, directory):
        directory.mkdir(parents=True, exist_ok=True)
        self.accounts = AccountStore(directory / "accounts.sqlite")
        self.sessions = TeacherSessions(self.accounts)
        self.decks = TeacherDecks(directory / "decks.sqlite", self.sessions)
        self.csrf_key = secrets.token_bytes(32)

    def csrf(self, token):
        return hmac.new(self.csrf_key, token.encode(), hashlib.sha256).hexdigest()

    @staticmethod
    def token(env):
        try:
            cookies = SimpleCookie(env.get("HTTP_COOKIE", ""))
            return cookies[COOKIE].value if COOKIE in cookies else None
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
                       ("Referrer-Policy", "no-referrer"),
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
    def form(env):
        if env.get("CONTENT_TYPE", "").split(";")[0] != "application/x-www-form-urlencoded":
            raise ValueError("Unsupported form format")
        length = int(env.get("CONTENT_LENGTH") or "0")
        if not 0 < length <= MAX_FORM:
            raise ValueError("Invalid body size")
        raw = env["wsgi.input"].read(length)
        if len(raw) != length:
            raise ValueError("Incomplete form")
        data = parse_qs(raw.decode("utf-8"), strict_parsing=True,
                        keep_blank_values=True, max_num_fields=5, errors="strict")
        if any(len(v) != 1 for v in data.values()):
            raise ValueError("Repeated field")
        return {k: v[0] for k, v in data.items()}

    def dashboard(self, token, notice=""):
        user = self.sessions.require(token)
        return views.workspace(user, self.accounts.role(user),
                               self.decks.list_decks(token), self.csrf(token), notice)

    def __call__(self, env, start):
        # Exact host and origin checks prevent DNS rebinding and cross-site writes.
        try:
            if env.get("HTTP_HOST") != HOST or env.get("REMOTE_ADDR") != "127.0.0.1":
                return self.respond(start, "403 Forbidden", "Local access only.")
            method, path = env["REQUEST_METHOD"], env["PATH_INFO"]
            token = self.token(env)
            if method == "GET" and path == "/":
                try:
                    body = self.dashboard(token)
                except PermissionError:
                    body = views.login()
                return self.respond(start, "200 OK", body)
            if method != "POST" or path not in {"/login", "/decks", "/teachers", "/logout"}:
                return self.respond(start, "404 Not Found", "Not found.")
            if env.get("HTTP_ORIGIN") != ORIGIN:
                return self.respond(start, "403 Forbidden", "Origin rejected.")
            data = self.form(env)
            if path == "/login":
                self.sessions.logout(token)
                new_token = self.sessions.login(data["username"], data["password"])
                if not new_token:
                    return self.respond(start, "401 Unauthorized", views.login("Invalid credentials or account locked."))
                return self.redirect(start, new_token)
            self.sessions.require(token)
            if not hmac.compare_digest(data.get("csrf", ""), self.csrf(token)):
                raise PermissionError("CSRF mismatch")
            if path == "/logout":
                self.sessions.logout(token)
                return self.redirect(start, clear=True)
            if path == "/teachers":
                self.sessions.create_account(token, data["username"], data["password"], "teacher")
            else:
                deck = import_csv(data["csv"], uuid4().hex, data["title"])
                self.decks.save(token, deck)
            return self.redirect(start)
        except PermissionError:
            LOG.warning("Web operation authorization denied")
            return self.respond(start, "403 Forbidden", "Access denied. Return to the dashboard.")
        except (ValueError, KeyError, UnicodeError, sqlite3.IntegrityError):
            LOG.warning("Invalid web form or duplicate record")
            return self.respond(start, "400 Bad Request", "Invalid form or duplicate record.")
        except Exception:
            LOG.exception("Local teacher web operation failed")
            return self.respond(start, "500 Internal Server Error", "Local operation failed.")
