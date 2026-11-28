import sqlite3
import sys
import tempfile
import unittest
from contextlib import closing
from pathlib import Path
from unittest.mock import Mock

from tools.teacher_device_enrollment import DeviceEnrollment
from tools.device_credential_record import decode_credential
from tools.windows_credential_protection import WindowsProtector


class ProtectedEnrollmentTests(unittest.TestCase):
    def setUp(self):
        self.folder = tempfile.TemporaryDirectory()
        self.addCleanup(self.folder.cleanup)
        self.path = Path(self.folder.name) / "credentials.sqlite"
        self.sessions, self.registry = Mock(), Mock()
        self.sessions.require.return_value = "admin"
        self.registry.list_assignments.return_value = [
            {"device_id": "tab5-001"}, {"device_id": "tab5-002"}]
        self.protector = Mock()
        self.legacy = DeviceEnrollment(self.path, self.sessions, self.registry)
        self.legacy.prepare("session", "tab5-001")
        self.service = DeviceEnrollment(self.path, self.sessions, self.registry,
                                        protector=self.protector)

    def test_protected_pending_and_legacy_preserved(self):
        self.protector.protect.return_value = b"mock-ciphertext"
        self.service.prepare("session", "tab5-002")
        record, entropy = self.protector.protect.call_args.args
        self.assertEqual(decode_credential(record)[0], "tab5-002")
        self.assertTrue(entropy.endswith(b"tab5-002"))
        with closing(sqlite3.connect(self.path)) as db:
            self.assertEqual(db.execute("SELECT device_id FROM device_credentials").fetchall(),
                             [("tab5-001",)])
            self.assertEqual(db.execute("SELECT protected FROM protected_device_credentials").fetchall(),
                             [(b"mock-ciphertext",)])
        self.assertEqual(self.service.pending("session"),
                         {"tab5-001": "KEY_PENDING", "tab5-002": "KEY_PENDING"})
        self.protector.unprotect.return_value = record
        self.assertEqual(self.service.protected_record("session", "tab5-002"), record)
        with self.assertRaises(ValueError):
            self.service.protected_record("session", "tab5-001")
        for service, ident in ((self.service, "tab5-001"), (self.legacy, "tab5-002")):
            with self.assertRaises(sqlite3.IntegrityError):
                service.prepare("session", ident)

    def test_protection_failure_no_fallback(self):
        self.protector.protect.side_effect = OSError("synthetic provider failure")
        with self.assertRaises(OSError):
            self.service.prepare("session", "tab5-002")
        self.assertEqual(self.service.pending("session"), {"tab5-001": "KEY_PENDING"})
        with closing(sqlite3.connect(self.path)) as db:
            self.assertEqual(db.execute("SELECT count(*) FROM protected_device_credentials").fetchone(), (0,))

    def test_admin_required(self):
        self.sessions.require.side_effect = PermissionError
        with self.assertRaises(PermissionError):
            self.service.prepare("session", "tab5-002")
        with self.assertRaises(PermissionError):
            self.service.protected_record("session", "tab5-002")
        self.protector.protect.assert_not_called()
        self.protector.unprotect.assert_not_called()

    @unittest.skipUnless(sys.platform == "win32", "Requires native Windows DPAPI")
    def test_native_dpapi(self):
        service = DeviceEnrollment(self.path, self.sessions, self.registry,
                                   protector=WindowsProtector())
        service.prepare("session", "tab5-002")
        record = service.protected_record("session", "tab5-002")
        self.assertEqual(decode_credential(record)[0], "tab5-002")
        with closing(sqlite3.connect(self.path)) as db:
            blob = db.execute("SELECT protected FROM protected_device_credentials").fetchone()[0]
        self.assertNotEqual(blob, record)
        with self.assertRaises(OSError):
            service.protector.unprotect(blob, b"wrong context")
