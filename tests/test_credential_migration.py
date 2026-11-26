import sqlite3
import tempfile
import unittest
from contextlib import closing
from pathlib import Path
from unittest.mock import Mock

from tools.credential_migration import migrate_legacy
from tools.device_credential_record import decode_credential
from tools.teacher_device_enrollment import DeviceEnrollment


class MigrationTests(unittest.TestCase):
    def setUp(self):
        folder = tempfile.TemporaryDirectory()
        self.addCleanup(folder.cleanup)
        self.path = Path(folder.name) / "credentials.sqlite"
        sessions, registry = Mock(), Mock()
        sessions.require.return_value = "admin"
        registry.list_assignments.return_value = [{"device_id": "tab5-001"}]
        self.service = DeviceEnrollment(self.path, sessions, registry)
        self.service.prepare("session", "tab5-001")
        self.protector = self.service.protector = Mock()
        self.protector.protect.return_value = b"synthetic-protected-blob"
        self.protector.unprotect.side_effect = lambda *_: self.protector.protect.call_args.args[0]

    def rows(self, table):
        with closing(sqlite3.connect(self.path)) as db:
            return db.execute(f"SELECT * FROM {table}").fetchall()

    def migrate(self, confirmed=True):
        return migrate_legacy(self.service, "session", "tab5-001", confirmed=confirmed)

    def test_success_preserves_key_metadata_and_pending_state(self):
        original = self.rows("device_credentials")[0]
        self.assertEqual(self.service.storage_status("session"), {"tab5-001": "legacy"})
        self.assertEqual(self.migrate(), "MIGRATED")
        self.assertEqual(self.rows("device_credentials"), [])
        row = self.rows("protected_device_credentials")[0]
        self.assertEqual(row[2:], original[2:])
        self.assertEqual(decode_credential(self.service.protected_record("session", "tab5-001")),
                         (original[0], original[1]))
        self.assertEqual(self.service.storage_status("session"), {"tab5-001": "protected"})
        self.assertEqual(self.service.pending("session"), {"tab5-001": "KEY_PENDING"})
        self.assertEqual(self.migrate(), "ALREADY_PROTECTED")
        self.protector.protect.assert_called_once()

    def test_confirmation_and_role_required(self):
        with self.assertRaises(PermissionError):
            self.migrate(False)
        self.service.sessions.require.side_effect = PermissionError
        with self.assertRaises(PermissionError):
            self.migrate()
        self.protector.protect.assert_not_called()

    def test_roundtrip_mismatch_preserves_legacy(self):
        original = self.rows("device_credentials")
        self.protector.unprotect.side_effect = None
        self.protector.unprotect.return_value = b"invalid-record"
        with self.assertRaises(ValueError):
            self.migrate()
        self.assertEqual(self.rows("device_credentials"), original)
        self.assertEqual(self.rows("protected_device_credentials"), [])

    def test_provider_failure_preserves_legacy(self):
        original = self.rows("device_credentials")
        self.protector.protect.side_effect = OSError("provider failed")
        with self.assertRaises(OSError):
            self.migrate()
        self.assertEqual(self.rows("device_credentials"), original)
        self.assertEqual(self.rows("protected_device_credentials"), [])

    def test_delete_failure_rolls_back_insert(self):
        original = self.rows("device_credentials")
        with closing(sqlite3.connect(self.path)) as db, db:
            db.execute("CREATE TRIGGER block_delete BEFORE DELETE ON device_credentials "
                       "BEGIN SELECT RAISE(ABORT, 'synthetic failure'); END")
        with self.assertRaises(sqlite3.IntegrityError):
            self.migrate()
        self.assertEqual(self.rows("device_credentials"), original)
        self.assertEqual(self.rows("protected_device_credentials"), [])
