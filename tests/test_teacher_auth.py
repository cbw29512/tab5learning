import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from tools.teacher_accounts import AccountStore
from tools.teacher_sessions import TeacherSessions


class TeacherAuthTests(unittest.TestCase):
    def setUp(self):
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        self.path = Path(tmp.name) / "accounts.sqlite"
        self.store = AccountStore(self.path)
        self.password = "synthetic-test-password"
        self.store.create("admin1", self.password, "admin", bootstrap=True)
        self.sessions = TeacherSessions(self.store)
        self.admin = self.sessions.login("ADMIN1", self.password)

    def test_bootstrap_and_roles(self):
        with self.assertRaises(PermissionError):
            self.store.create("admin2", self.password, "admin", bootstrap=True)
        self.sessions.create_account(self.admin, "teacher1", self.password, "teacher")
        token = self.sessions.login("teacher1", self.password)
        with self.assertRaises(PermissionError):
            self.sessions.create_account(token, "other", self.password, "admin")

    def test_disable_revokes_session(self):
        self.sessions.create_account(self.admin, "teacher1", self.password, "teacher")
        token = self.sessions.login("teacher1", self.password)
        self.sessions.disable_account(self.admin, "teacher1")
        with self.assertRaises(PermissionError):
            self.sessions.require(token)
        self.assertIsNone(self.sessions.login("teacher1", self.password))

    def test_lockout_persists(self):
        with patch("tools.teacher_accounts.time.time", return_value=1000):
            for _ in range(5):
                self.assertIsNone(self.store.verify("admin1", "wrong-password"))
            self.assertIsNone(AccountStore(self.path).verify("admin1", self.password))
        with patch("tools.teacher_accounts.time.time", return_value=1301):
            self.assertEqual(self.store.verify("admin1", self.password), "admin1")

    def test_logout_and_expiration(self):
        self.sessions.logout(self.admin)
        with self.assertRaises(PermissionError):
            self.sessions.require(self.admin)
        with patch("tools.teacher_sessions.time.monotonic", return_value=100):
            token = self.sessions.login("admin1", self.password)
        with patch("tools.teacher_sessions.time.monotonic", return_value=1901):
            with self.assertRaises(PermissionError):
                self.sessions.require(token)
