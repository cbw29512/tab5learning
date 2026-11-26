import io
import re
import tempfile
import unittest
from pathlib import Path
from urllib.parse import urlencode

from tools.teacher_web import TeacherWeb, HOST, ORIGIN


class TeacherWebTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.app = TeacherWeb(Path(self.temp.name))
        self.app.accounts.create("admin", "synthetic-admin-pass", "admin", bootstrap=True)
        self.cookie = ""

    def call(self, path="/", form=None, **overrides):
        body = urlencode(form or {}).encode()
        env = {"HTTP_HOST": HOST, "REMOTE_ADDR": "127.0.0.1",
               "HTTP_ORIGIN": ORIGIN, "HTTP_COOKIE": self.cookie,
               "REQUEST_METHOD": "GET" if form is None else "POST",
               "PATH_INFO": path, "CONTENT_LENGTH": str(len(body)),
               "CONTENT_TYPE": "application/x-www-form-urlencoded",
               "wsgi.input": io.BytesIO(body)}
        env.update(overrides)
        captured = []
        result = b"".join(self.app(env, lambda status, headers: captured.extend([status, dict(headers)]))).decode()
        status, headers = captured
        if "Set-Cookie" in headers:
            self.cookie = headers["Set-Cookie"].split(";")[0]
        return int(status.split()[0]), headers, result

    def login_form(self, user="admin", password="synthetic-admin-pass"):
        status, _, page = self.call("/login")
        self.assertEqual(status, 200)
        proof = re.search(r'name="csrf" value="([a-f0-9]{64})"', page).group(1)
        return {"username": user, "password": password, "csrf": proof}

    def login(self, user="admin", password="synthetic-admin-pass", **headers_override):
        status, headers, _ = self.call("/login", self.login_form(user, password), **headers_override)
        self.assertEqual(status, 303)
        self.assertIn("HttpOnly", headers["Set-Cookie"])
        self.assertIn("SameSite=Strict", headers["Set-Cookie"])
        return self.app.csrf(self.cookie.split("=", 1)[1])

    def test_access_guards_and_logout(self):
        self.assertIn("Teacher sign in", self.call()[2])
        self.assertEqual(self.call(HTTP_HOST="evil.example")[0], 403)
        self.assertEqual(self.call(REMOTE_ADDR="192.0.2.1")[0], 403)
        self.assertEqual(self.call("/login", {"username":"admin","password":"synthetic-admin-pass"},
                                   HTTP_ORIGIN="https://evil.example")[0], 403)
        self.assertEqual(self.call("/decks", {"csrf":"bad"})[0], 403)
        csrf = self.login()
        self.assertIn("Teacher dashboard", self.call()[2])
        self.assertEqual(self.call("/logout", {"csrf":"bad"})[0], 403)
        self.assertEqual(self.call("/logout", {"csrf":csrf})[0], 303)
        self.assertIn("Teacher sign in", self.call()[2])

    def test_decks_permissions_and_escaping(self):
        csrf = self.login()
        self.assertEqual(self.call("/decks", {"csrf":csrf,"title":"<script>","csv":"front,back\n2+2,4"})[0], 303)
        page = self.call()[2]
        self.assertIn("&lt;script&gt;", page)
        self.assertNotIn("<script>", page)
        self.assertEqual(self.call("/decks", {"csrf":csrf,"title":"Invalid","csv":"wrong,header"})[0], 400)
        self.assertEqual(self.call("/teachers", {"csrf":csrf,"username":"teacher1","password":"synthetic-teacher-pass"})[0], 303)
        csrf = self.login("teacher1", "synthetic-teacher-pass")
        self.assertNotIn("&lt;script&gt;", self.call()[2])
        self.assertEqual(self.call("/teachers", {"csrf":csrf,"username":"teacher2","password":"synthetic-teacher-pass"})[0], 403)
        self.assertEqual(self.call("/decks", {"csrf":csrf,"title":"Reading","csv":"front,back\ncat,animal"})[0], 303)
        self.assertIn("Reading", self.call()[2])
        self.login()
        self.assertIn("Reading", self.call()[2])

    def test_no_web_recovery_or_bootstrap(self):
        self.login()
        for path in ("/bootstrap", "/flash", "/reset", "/ssh"):
            self.assertEqual(self.call(path, {"csrf":"x"})[0], 404)

    def test_isolated_short_password_mode(self):
        # Normal accounts retain the original minimum.
        with self.assertRaises(ValueError):
            self.app.accounts.create("shortuser", "1234", "teacher", actor="admin")
        normal = self.app
        self.app = TeacherWeb(Path(self.temp.name) / "test", test_mode=True)
        with self.assertRaises(ValueError):
            self.app.accounts.create("admin", "123", "admin", bootstrap=True)
        self.app.accounts.create("admin", "1234", "admin", bootstrap=True)
        self.assertIn("TEST MODE", self.call()[2])
        csrf = self.login("admin", "1234")
        self.assertIn('minlength="4"', self.call()[2])
        self.assertEqual(self.call("/teachers", {"csrf":csrf,"username":"teacher1","password":"5678"})[0], 303)
        self.login("teacher1", "5678")
        self.assertIsNone(normal.accounts.verify("admin", "1234"))
        self.assertIsNone(normal.accounts.role("teacher1"))

    def test_browser_form_referrer_policy(self):
        status, headers, _ = self.call()
        self.assertEqual(status, 200)
        self.assertEqual(headers["Referrer-Policy"], "same-origin")
        for origin in ("null", "", "http://127.0.0.1:8765"):
            self.assertEqual(self.call("/login", {"username":"admin","password":"synthetic-admin-pass"},
                                       HTTP_ORIGIN=origin)[0], 403)
        for origin in ("null", "", None):
            csrf = self.login(HTTP_ORIGIN=origin)
            self.assertIn("Teacher dashboard", self.call()[2])
            self.assertEqual(self.call("/logout", {"csrf":csrf}, HTTP_ORIGIN=origin)[0], 303)

    def test_login_challenge_blocks_cross_site_and_invalid_proof(self):
        form = self.login_form()
        self.assertEqual(self.call("/login", form, HTTP_ORIGIN="http://127.0.0.1:8765")[0], 403)
        self.assertEqual(self.call("/login", form, HTTP_ORIGIN="null", HTTP_SEC_FETCH_SITE="cross-site")[0], 403)
        self.assertEqual(self.call("/login", form, HTTP_ORIGIN="null", HTTP_REFERER="https://evil.example/")[0], 403)
        form["csrf"] = "bad"
        self.assertEqual(self.call("/login", form, HTTP_ORIGIN="null")[0], 403)
        self.login(HTTP_ORIGIN="null")
        self.assertIn("Teacher dashboard", self.call()[2])


    def test_planned_assignments_only_in_test_mode(self):
        csrf = self.login()
        form = {"csrf":csrf,"student_id":"student-001","device_id":"tab5-001","grade":"1"}
        self.assertEqual(self.call("/assignments", form)[0], 403)
        self.app = TeacherWeb(Path(self.temp.name) / "registry-test", test_mode=True)
        self.app.accounts.create("admin", "1234", "admin", bootstrap=True)
        csrf = self.login("admin", "1234")
        form["csrf"] = csrf
        self.assertEqual(self.call("/assignments", dict(form, csrf="bad"))[0], 403)
        self.assertEqual(self.call("/assignments", form)[0], 303)
        self.assertIn("Planned — not synced", self.call()[2])
        self.assertIn("student-001", self.call()[2])
        self.assertEqual(self.call("/assignments", form)[0], 400)
        self.assertEqual(self.call("/assignments", dict(form, student_id="s2", device_id="d2", grade="4"))[0], 400)

    def test_usb_scan_admin_test_mode_only(self):
        from unittest.mock import patch
        from tools.teacher_usb_discovery import UsbSnapshot
        csrf = self.login()
        with patch("tools.teacher_web.scan_usb", return_value=UsbSnapshot("detected", ("COM5",))) as scan:
            self.assertEqual(self.call("/usb-scan", {"csrf":csrf})[0], 403)
            self.app = TeacherWeb(Path(self.temp.name) / "usb-test", test_mode=True)
            self.app.accounts.create("admin", "1234", "admin", bootstrap=True)
            csrf = self.login("admin", "1234")
            self.assertEqual(self.call("/usb-scan", {"csrf":"bad"})[0], 403)
            scan.assert_not_called()
            status, _, page = self.call("/usb-scan", {"csrf":csrf})
            self.assertEqual(status, 200)
            self.assertIn("COM5", page)
            self.assertIn("No assignment has been synced", page)
            self.assertEqual(self.call("/teachers", {"csrf":csrf,"username":"teacher1","password":"5678"})[0], 303)
            csrf = self.login("teacher1", "5678")
            self.assertEqual(self.call("/usb-scan", {"csrf":csrf})[0], 403)
            scan.assert_called_once()

    def test_usb_identification_authorization(self):
        from unittest.mock import patch
        from tools.teacher_usb_identity import IdentityResult
        csrf = self.login()
        with patch("tools.teacher_web.identify", return_value=IdentityResult("responding", "COM5")) as probe:
            self.assertEqual(self.call("/usb-identify", {"csrf":csrf,"port":"COM5"})[0], 403)
            self.app = TeacherWeb(Path(self.temp.name) / "identity-test", test_mode=True)
            self.app.accounts.create("admin", "1234", "admin", bootstrap=True)
            csrf = self.login("admin", "1234")
            self.assertEqual(self.call("/usb-identify", {"csrf":"bad","port":"COM5"})[0], 403)
            probe.assert_not_called()
            status, _, page = self.call("/usb-identify", {"csrf":csrf,"port":"COM5"})
            self.assertEqual(status, 200)
            self.assertIn("Learner protocol v1 responding", page)
            self.assertIn("NOT authenticated", page)
            self.assertEqual(self.call("/teachers", {"csrf":csrf,"username":"teacher1","password":"5678"})[0], 303)
            csrf = self.login("teacher1", "5678")
            self.assertEqual(self.call("/usb-identify", {"csrf":csrf,"port":"COM5"})[0], 403)
            probe.assert_called_once_with("COM5")

    def test_pending_enrollment_is_admin_only(self):
        csrf = self.login()
        self.assertEqual(self.call("/enrollment", {"csrf":csrf,"device_id":"tab5-001"})[0], 403)
        self.app = TeacherWeb(Path(self.temp.name) / "enrollment-test", test_mode=True)
        self.app.accounts.create("admin", "1234", "admin", bootstrap=True)
        csrf = self.login("admin", "1234")
        form = {"csrf":csrf,"device_id":"tab5-001"}
        self.assertEqual(self.call("/enrollment", form)[0], 400)
        self.assertEqual(self.call("/assignments", dict(form, student_id="student-001", grade="1"))[0], 303)
        self.assertEqual(self.call("/enrollment", dict(form, csrf="bad"))[0], 403)
        self.assertEqual(self.call("/enrollment", form)[0], 303)
        page = self.call()[2]
        self.assertIn("KEY_PENDING (not installed)", page)
        self.assertIn("Planned — not synced", page)
        self.assertEqual(self.call("/enrollment", form)[0], 400)
        self.assertEqual(self.call("/teachers", {"csrf":csrf,"username":"teacher1","password":"5678"})[0], 303)
        csrf = self.login("teacher1", "5678")
        self.assertEqual(self.call("/enrollment", dict(form, csrf=csrf))[0], 403)
        self.assertNotIn("Prepare key for", self.call()[2])

    def test_explicit_credential_migration_action(self):
        from unittest.mock import Mock, patch
        csrf = self.login()
        form = {"csrf": csrf, "device_id": "tab5-001", "confirm": "yes"}
        self.assertEqual(self.call("/protect-credential", form)[0], 403)
        self.app = TeacherWeb(Path(self.temp.name) / "migration-test", test_mode=True)
        self.app.accounts.create("admin", "1234", "admin", bootstrap=True)
        csrf = self.login("admin", "1234")
        form["csrf"] = csrf
        self.assertEqual(self.call("/assignments", {"csrf": csrf, "device_id": "tab5-001",
                                                  "student_id": "student-001", "grade": "1"})[0], 303)
        # Prepare a legacy test key deliberately, then inject a mock protection provider.
        self.app.enrollment.protector = None
        self.assertEqual(self.call("/enrollment", {"csrf": csrf, "device_id": "tab5-001"})[0], 303)
        protector = Mock()
        protector.protect.return_value = b"mock-protected"
        protector.unprotect.side_effect = lambda *_: protector.protect.call_args.args[0]
        self.app.enrollment.protector = protector
        self.assertIn("Protect existing key", self.call()[2])
        self.assertEqual(self.call("/protect-credential", dict(form, csrf="bad"))[0], 403)
        self.assertEqual(self.call("/protect-credential", dict(form, confirm="no"))[0], 403)
        self.assertEqual(self.call("/protect-credential", form, HTTP_ORIGIN="https://evil.example")[0], 403)
        protector.protect.assert_not_called()
        with patch("tools.teacher_web.identify") as usb:
            self.assertEqual(self.call("/protect-credential", form)[0], 303)
            usb.assert_not_called()
        page = self.call()[2]
        self.assertIn("Protected host record (test only)", page)
        self.assertIn("KEY_PENDING (not installed)", page)
        self.assertIn("Planned — not synced", page)
        self.assertNotIn("Protect existing key", page)
        self.assertEqual(self.call("/teachers", {"csrf": csrf, "username": "teacher1", "password": "5678"})[0], 303)
        csrf = self.login("teacher1", "5678")
        self.assertEqual(self.call("/protect-credential", dict(form, csrf=csrf))[0], 403)
        protector.protect.assert_called_once()

    def test_bulk_flashcards_review_before_save(self):
        self.assertIn("Teacher sign in", self.call("/decks/bulk")[2])
        csrf = self.login()
        self.assertIn("Copyable prompt", self.call("/decks/bulk")[2])
        form = dict(csrf=csrf, title="Bulk science", csv="front,back\nSun,star", proof="", action="preview")
        status, _, page = self.call("/decks/bulk", form)
        self.assertEqual(status, 200)
        self.assertIn("Review 1 cards", page)
        self.assertNotIn("Bulk science", self.call()[2])
        proof = re.search(r'name="proof" value="([a-f0-9]{64})"', page).group(1)
        self.assertEqual(self.call("/decks/bulk", dict(form, action="save", proof=proof, csrf="bad"))[0], 403)
        self.assertEqual(self.call("/decks/bulk", dict(form, action="save", proof=proof))[0], 200)
        self.assertIn("Bulk science", self.call()[2])

    def test_bulk_prompt_generation(self):
        csrf = self.login()
        form = dict(csrf=csrf, action="generate", grade="4", subject="Science",
                    count="50", notes="Parts of a plant")
        self.assertEqual(self.call("/decks/bulk", dict(form, csrf="bad"))[0], 403)
        status, _, page = self.call("/decks/bulk", form)
        self.assertEqual(status, 200)
        self.assertIn("50 age-appropriate study flashcards for Grade 4 Science", page)
        self.assertIn("Parts of a plant", page)
        self.assertIn("No saved decks", self.call()[2])

    def test_backup_center_scope_and_restore_preview(self):
        root = Path(self.temp.name) / "external"
        root.mkdir()
        csrf = self.login()
        self.assertEqual(self.call("/backups")[0], 403)
        self.app = TeacherWeb(Path(self.temp.name) / "test-backup", test_mode=True, backup_dir=root)
        self.app.accounts.create("admin", "1234", "admin", bootstrap=True)
        csrf = self.login("admin", "1234")
        self.assertIn("Open teaching backup center", self.call()[2])
        self.assertIn("NOT encrypted", self.call("/backups")[2])
        self.assertEqual(self.call("/decks", dict(csrf=csrf,title="Test cards",csv="front,back\n2+2,4"))[0],303)
        self.assertEqual(self.call("/backups", dict(csrf="bad", action="save"))[0],403)
        status, _, page = self.call("/backups", dict(csrf=csrf,action="save"))
        self.assertEqual(status,200)
        self.assertIn("Verified backup saved",page)
        name = re.search(r'pocket-teaching-test-[a-f0-9]{32}\.plta',page).group(0)
        self.assertTrue((root/"admin"/name).is_file())
        self.assertNotIn("Review restore",page)
        status, _, page = self.call("/backups", dict(csrf=csrf,action="preview",filename=name))
        self.assertIn("1 existing versions",page)
        proof = re.search(r'name="proof" value="([a-f0-9]{64})"',page).group(1)
        digest = re.search(r'name="digest" value="([a-f0-9]{64})"',page).group(1)
        self.assertEqual(self.call("/backups",dict(csrf=csrf,action="restore",filename=name,digest=digest,proof="0"*64))[0],403)
        self.assertIn("Restore verified",self.call("/backups",dict(csrf=csrf,action="restore",filename=name,digest=digest,proof=proof))[2])
        self.assertIn("failed",self.call("/backups",dict(csrf=csrf,action="preview",filename="../"+name))[2])
        self.assertEqual(self.call("/teachers",dict(csrf=csrf,username="teacher1",password="5678"))[0],303)
        csrf = self.login("teacher1","5678")
        self.assertNotIn(name,self.call("/backups")[2])
        self.assertIn("failed",self.call("/backups",dict(csrf=csrf,action="preview",filename=name))[2])
