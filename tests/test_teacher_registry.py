import sqlite3
import tempfile
import unittest
from pathlib import Path
from unittest.mock import Mock

from tools.teacher_registry import TeacherRegistry


class RegistryTests(unittest.TestCase):
    def test_assignment_contract(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / "registry.sqlite"
            sessions = Mock()
            sessions.require.return_value = "teacher1"
            sessions.accounts.role.return_value = "teacher"
            registry = TeacherRegistry(path, sessions)
            registry.create("session", "Student-001", "Tab5-001", 1)
            rows = TeacherRegistry(path, sessions).list_assignments("session")
            self.assertEqual(len(rows), 1)
            self.assertEqual(rows[0]["student_id"], "student-001")
            self.assertEqual(rows[0]["status"], "PLANNED")
            for student, device in [("student-002", "TAB5-001"), ("STUDENT-001", "tab5-002")]:
                with self.assertRaises(sqlite3.IntegrityError):
                    registry.create("session", student, device, 2)
            for student, device, grade in [("bad name", "d1", 1), ("s2", "", 1),
                                           ("s2", "d2", 4), ("s2", "d2", True)]:
                with self.assertRaises(ValueError):
                    registry.create("session", student, device, grade)
            sessions.require.return_value = "teacher2"
            self.assertEqual(registry.list_assignments("session"), [])
            sessions.accounts.role.return_value = "admin"
            self.assertEqual(len(registry.list_assignments("session")), 1)
            sessions.require.side_effect = PermissionError
            with self.assertRaises(PermissionError):
                registry.create("session", "s2", "d2", 2)
            with self.assertRaises(PermissionError):
                registry.list_assignments("session")
