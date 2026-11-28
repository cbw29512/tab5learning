import sqlite3
import tempfile
import unittest
from contextlib import closing
from pathlib import Path
from unittest.mock import Mock

from tools.teacher_device_enrollment import DeviceEnrollment


class EnrollmentTests(unittest.TestCase):
    def test_pending_only_and_key_uniqueness(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / "credentials.sqlite"
            sessions, registry = Mock(), Mock()
            sessions.require.return_value = "admin"
            registry.list_assignments.return_value = [{"device_id":"tab5-001"}, {"device_id":"tab5-002"}]
            service = DeviceEnrollment(path, sessions, registry)
            service.prepare("session", "TAB5-001")
            service.prepare("session", "tab5-002")
            self.assertEqual(service.pending("session"), {"tab5-001":"KEY_PENDING", "tab5-002":"KEY_PENDING"})
            # SQLite's transaction context does not close the connection.
            # Explicit closure releases the Windows file handle before temp cleanup.
            with closing(sqlite3.connect(path)) as db:
                keys = [row[0] for row in db.execute("SELECT secret FROM device_credentials")]
            self.assertEqual(len(set(keys)), 2)
            self.assertTrue(all(len(key)==32 for key in keys))
            with self.assertRaises(sqlite3.IntegrityError):
                service.prepare("session", "tab5-001")
            with self.assertRaises(ValueError):
                service.prepare("session", "unknown")
            sessions.require.side_effect = PermissionError
            with self.assertRaises(PermissionError):
                service.prepare("session", "tab5-002")
            with self.assertRaises(PermissionError):
                service.pending("session")
