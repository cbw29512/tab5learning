"""Synthetic-only archive regression with real SQLite and scoped teacher sessions."""
import sqlite3
import struct
import tempfile
import unittest
import zlib
from contextlib import closing
from pathlib import Path
from unittest.mock import Mock

from tools.math_backup_codec import MathBackup, encode
from tools.math_backup_archive import PreviewMathArchive
from tools.teacher_registry import TeacherRegistry


def crc(body):
    return body + struct.pack("<I", zlib.crc32(body))


def checkpoint(attempts):
    return crc(b"PL\x01" + bytes((0, 0, bool(attempts), 0, 0))
               + struct.pack("<I", attempts))


def snapshot(count, alternate=False, base=0):
    rows = tuple(crc(b"PA\x01" + bytes((0, 2 if alternate and i == base + 1 else 0, 1))
                     + b"\0\0" + checkpoint(i)) for i in range(base + 1, count + 1))
    return encode(MathBackup(checkpoint(base), checkpoint(count), rows))


class PreviewArchiveTests(unittest.TestCase):
    def setUp(self):
        folder = tempfile.TemporaryDirectory()
        self.addCleanup(folder.cleanup)
        self.path = Path(folder.name) / "preview.sqlite"
        self.sessions = Mock()
        self.sessions.require.return_value = "teacher1"
        self.sessions.accounts.role.return_value = "teacher"
        self.registry = TeacherRegistry(Path(folder.name) / "registry.sqlite", self.sessions)
        self.registry.create("token", "student-001", "tab5-001", 1)
        self.archive = self.reopen()

    def reopen(self):
        return PreviewMathArchive(self.path, self.sessions, self.registry, test_mode=True)

    def save(self, raw):
        return self.archive.save("token", "tab5-001", "student-001", raw)

    def load(self, **kwargs):
        return self.archive.load("token", "tab5-001", "student-001", **kwargs)

    def counts(self):
        with closing(sqlite3.connect(self.path)) as db:
            return tuple(db.execute("SELECT COUNT(*) FROM " + table).fetchone()[0]
                         for table in ("preview_math_base", "preview_math_attempt",
                                       "preview_math_receipt"))

    def test_reopen_and_extension_without_duplicate_attempts(self):
        first, second = snapshot(2), snapshot(100)
        old = self.save(first)
        latest = self.save(second)
        self.archive = self.reopen()
        self.assertEqual(self.load(), second)
        self.assertEqual(self.load(digest=old), first)
        self.assertEqual(self.save(first), old)
        self.assertEqual(self.save(second), latest)
        self.assertEqual(self.counts(), (1, 100, 2))

    def test_conflict_and_stale_unknown_prefix_preserve_history(self):
        original = snapshot(4)
        self.save(original)
        for raw in (snapshot(3), snapshot(4, alternate=True), snapshot(5, alternate=True),
                    snapshot(5, base=1)):
            with self.assertRaises(ValueError):
                self.save(raw)
        self.assertEqual(self.load(), original)
        self.assertEqual(self.counts(), (1, 4, 1))

    def test_rollback_on_receipt_failure(self):
        original = snapshot(2)
        self.save(original)
        with closing(sqlite3.connect(self.path)) as db, db:
            db.execute("CREATE TRIGGER fail_receipt BEFORE INSERT ON preview_math_receipt "
                       "BEGIN SELECT RAISE(ABORT,'synthetic disk failure'); END")
        with self.assertRaises(sqlite3.Error):
            self.save(snapshot(5))
        self.assertEqual(self.counts(), (1, 2, 1))
        self.assertEqual(self.load(), original)

    def test_corrupted_archive_fails_closed(self):
        self.save(snapshot(2))
        with closing(sqlite3.connect(self.path)) as db, db:
            db.execute("UPDATE preview_math_attempt SET body=? WHERE slot=1", (bytes(28),))
        with self.assertRaises(ValueError):
            self.load()
        with self.assertRaises(ValueError):
            self.save(snapshot(3))
        self.assertEqual(self.counts(), (1, 2, 1))

    def test_scope_and_test_mode(self):
        self.save(snapshot(1))
        with self.assertRaises(PermissionError):
            PreviewMathArchive(self.path, self.sessions, self.registry)
        with self.assertRaises(PermissionError):
            self.archive.save("token", "tab5-001", "other", snapshot(2))
        self.sessions.require.return_value = "teacher2"
        with self.assertRaises(PermissionError):
            self.load()
        with self.assertRaises(PermissionError):
            self.save(snapshot(2))
        self.sessions.accounts.role.return_value = "admin"
        self.assertEqual(self.load(), snapshot(1))
        self.sessions.require.side_effect = PermissionError
        with self.assertRaises(PermissionError):
            self.load()

    def test_empty_and_migrated_base(self):
        self.save(snapshot(0))
        self.save(snapshot(1))
        self.assertEqual(self.counts(), (1, 1, 2))
        self.registry.create("token", "student-002", "tab5-002", 2)
        migrated = snapshot(4, base=3)
        self.archive.save("token", "tab5-002", "student-002", migrated)
        self.assertEqual(self.archive.load("token", "tab5-002", "student-002"), migrated)
        self.assertEqual(self.counts(), (2, 2, 3))

    def test_invalid_input_creates_no_rows(self):
        with self.assertRaises(ValueError):
            self.save(b"bad")
        self.assertEqual(self.counts(), (0, 0, 0))
