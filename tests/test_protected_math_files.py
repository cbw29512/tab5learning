import tempfile
import unittest
from pathlib import Path
from unittest.mock import Mock

from tools.archive_envelope import new_recovery_key
from tools.math_backup_archive import PreviewMathArchive
from tools.protected_math_files import ProtectedMathFiles
from tools.teacher_registry import TeacherRegistry
from test_math_backup_archive import snapshot


class ProtectedMathFilesTests(unittest.TestCase):
    def setUp(self):
        folder = tempfile.TemporaryDirectory()
        self.addCleanup(folder.cleanup)
        root = Path(folder.name)
        self.work = root / "work"
        self.work.mkdir()
        self.out = root / "external"
        self.out.mkdir()
        self.sessions = Mock()
        self.sessions.require.return_value = "teacher1"
        self.sessions.accounts.role.return_value = "teacher"
        registry = TeacherRegistry(self.work / "registry.sqlite", self.sessions)
        registry.create("token", "student-001", "tab5-001", 1)
        registry.create("token", "student-002", "tab5-002", 1)
        self.src = PreviewMathArchive(self.work / "src.sqlite", self.sessions, registry, test_mode=True)
        self.dst = PreviewMathArchive(self.work / "dst.sqlite", self.sessions, registry, test_mode=True)
        self.saver = ProtectedMathFiles(self.src)
        self.reader = ProtectedMathFiles(self.dst)
        self.key = new_recovery_key()
        self.args = ("token", "tab5-001", "student-001")
        self.src.save(*self.args, snapshot(3))
        self.receipt = self.saver.save(*self.args, self.out, recovery_key=self.key)

    def test_exact_restore_and_newer_history(self):
        path = self.receipt.path
        self.assertEqual(self.receipt.records, 3)
        self.assertNotIn(b"student-001", path.read_bytes())
        review = self.reader.preview(*self.args, path, recovery_key=self.key)
        self.assertEqual(review.records_to_add, 3)
        with self.assertRaises(ValueError):
            self.dst.load(*self.args)
        self.reader.restore(*self.args, path, recovery_key=self.key,
                            expected_sha256=review.sha256, confirmed=True)
        self.assertEqual(self.dst.load(*self.args), snapshot(3))
        self.dst.save(*self.args, snapshot(5))
        self.assertTrue(self.reader.preview(*self.args, path, recovery_key=self.key).already_present)
        self.reader.restore(*self.args, path, recovery_key=self.key,
                            expected_sha256=review.sha256, confirmed=True)
        self.assertEqual(self.dst.load(*self.args), snapshot(5))

    def test_scope_key_conflicts_and_confirmation(self):
        path = self.receipt.path
        with self.assertRaises(ValueError):
            self.reader.preview(*self.args, path, recovery_key=new_recovery_key())
        with self.assertRaises(PermissionError):
            self.reader.preview("token","tab5-002","student-002",path,recovery_key=self.key)
        with self.assertRaises(PermissionError):
            self.reader.restore(*self.args,path,recovery_key=self.key,
                                expected_sha256=self.receipt.sha256)
        self.dst.save(*self.args, snapshot(1, alternate=True))
        with self.assertRaises(ValueError):
            self.reader.preview(*self.args,path,recovery_key=self.key)
        self.assertEqual(self.dst.load(*self.args),snapshot(1,alternate=True))
        self.sessions.require.return_value = "other"
        with self.assertRaises(PermissionError):
            self.reader.preview(*self.args,path,recovery_key=self.key)

    def test_changed_file(self):
        path = self.receipt.path
        self.src.save(*self.args,snapshot(4))
        next_file = self.saver.save(*self.args,self.out,recovery_key=self.key)
        path.write_bytes(next_file.path.read_bytes())
        with self.assertRaises(ValueError):
            self.reader.restore(*self.args,path,recovery_key=self.key,
                                expected_sha256=self.receipt.sha256,confirmed=True)
        with self.assertRaises(ValueError):
            self.dst.load(*self.args)
