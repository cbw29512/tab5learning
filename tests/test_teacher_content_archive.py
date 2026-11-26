import tempfile
import unittest
from pathlib import Path
from unittest.mock import Mock

from tools.flashcard_deck import import_csv
from tools.teacher_decks import TeacherDecks
from tools.teacher_content_codec import decode, encode
from tools.teacher_content_archive import PreviewTeacherArchive, RestoreSummary


class TeacherContentArchiveTests(unittest.TestCase):
    def setUp(self):
        folder = tempfile.TemporaryDirectory()
        self.addCleanup(folder.cleanup)
        self.root = Path(folder.name)
        self.sessions = Mock()
        self.sessions.require.return_value = "teacher1"
        self.sessions.accounts.role.return_value = "teacher"
        self.source = TeacherDecks(self.root / "source.sqlite", self.sessions)
        self.dest = TeacherDecks(self.root / "dest.sqlite", self.sessions)
        self.backup = PreviewTeacherArchive(self.source, test_mode=True)
        self.recovery = PreviewTeacherArchive(self.dest, test_mode=True)
        self.deck = import_csv("front,back\nq,a", "science", "Science")
        self.source.save("token", self.deck)
        self.source.revise("token", dict(self.deck, version=2, title="Revised"), 1)

    def test_exact_restore_and_retry(self):
        raw = self.backup.export("token")
        self.assertEqual(self.recovery.preview("token", raw), RestoreSummary(0, 2))
        self.assertEqual(self.dest.list_decks("token"), [])
        with self.assertRaises(PermissionError):
            self.recovery.restore("token", raw)
        self.assertEqual(self.recovery.restore("token", raw, confirmed=True), RestoreSummary(0, 2))
        self.assertEqual(self.dest.list_decks("token"), self.source.list_decks("token"))
        self.assertEqual(self.recovery.restore("token", raw, confirmed=True), RestoreSummary(2, 0))
        older = encode({"owner": "teacher1", "decks": [self.deck]})
        self.assertEqual(self.recovery.restore("token", older, confirmed=True), RestoreSummary(1, 0))
        self.assertEqual(len(self.dest.list_decks("token")), 2)

    def test_scope_and_integrity(self):
        raw = self.backup.export("token")
        self.sessions.require.return_value = "teacher2"
        self.sessions.accounts.role.return_value = "admin"
        self.assertEqual(decode(self.backup.export("token"))["decks"], [])
        with self.assertRaises(PermissionError):
            self.recovery.restore("token", raw, confirmed=True)
        self.sessions.require.return_value = "teacher1"
        with self.assertRaises(ValueError):
            self.recovery.restore("token", raw[:-1], confirmed=True)
        with self.assertRaises(ValueError):
            encode({"owner": "teacher1", "decks": [dict(self.deck, version=2)]})
        with self.assertRaises(PermissionError):
            PreviewTeacherArchive(self.dest)

    def test_conflict_rolls_back_whole_restore(self):
        raw = self.backup.export("token")
        self.dest.save("token", dict(self.deck, title="Conflicting"))
        extra = import_csv("front,back\nx,y", "aaa", "Extra")
        archive = decode(raw)
        archive["decks"].insert(0, extra)
        with self.assertRaises(ValueError):
            self.recovery.restore("token", encode(archive), confirmed=True)
        self.assertEqual(self.dest.list_decks("token"), [dict(self.deck, title="Conflicting")])

    def test_foreign_owner_cannot_merge(self):
        raw = self.backup.export("token")
        self.sessions.require.return_value = "other"
        self.dest.save("token", self.deck)
        self.sessions.require.return_value = "teacher1"
        with self.assertRaises(ValueError):
            self.recovery.restore("token", raw, confirmed=True)
        self.assertEqual(self.dest.list_decks("token"), [])
