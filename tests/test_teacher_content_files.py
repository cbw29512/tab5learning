import tempfile
import unittest
from pathlib import Path
from unittest.mock import Mock, patch

from tools.flashcard_deck import import_csv
from tools.teacher_decks import TeacherDecks
from tools.teacher_content_archive import PreviewTeacherArchive, RestoreSummary
from tools.teacher_content_files import PreviewArchiveFiles, read_archive


class TeacherContentFileTests(unittest.TestCase):
    def setUp(self):
        folder = tempfile.TemporaryDirectory()
        self.addCleanup(folder.cleanup)
        self.root = Path(folder.name)
        self.workspace = self.root / "workspace"
        self.workspace.mkdir()
        self.external = self.root / "independent"
        self.external.mkdir()
        self.sessions = Mock()
        self.sessions.require.return_value = "teacher1"
        self.sessions.accounts.role.return_value = "teacher"
        self.source = TeacherDecks(self.workspace / "source.sqlite", self.sessions)
        self.dest = TeacherDecks(self.workspace / "dest.sqlite", self.sessions)
        self.save = PreviewArchiveFiles(PreviewTeacherArchive(self.source, test_mode=True))
        self.restore = PreviewArchiveFiles(PreviewTeacherArchive(self.dest, test_mode=True))
        self.deck = import_csv("front,back\nq,a", "science", "Science")
        self.source.save("token", self.deck)

    def test_file_roundtrip_and_preserve_prior_backup(self):
        first = self.save.save("token", self.external)
        original = first.path.read_bytes()
        self.source.revise("token", dict(self.deck, version=2, title="New"), 1)
        second = self.save.save("token", self.external)
        self.assertNotEqual(first.path, second.path)
        self.assertEqual(first.path.read_bytes(), original)
        self.assertEqual(second.versions, 2)
        preview = self.restore.preview("token", second.path)
        self.assertEqual(preview.summary, RestoreSummary(0, 2))
        self.assertEqual(preview.sha256, second.sha256)
        self.assertEqual(self.dest.list_decks("token"), [])
        self.assertEqual(self.restore.restore("token", second.path,
            expected_sha256=preview.sha256, confirmed=True), RestoreSummary(0, 2))
        self.assertEqual(self.source.list_decks("token"), self.dest.list_decks("token"))

    def test_reject_wrong_destination_and_failed_publication(self):
        with self.assertRaises(ValueError):
            self.save.save("token", self.workspace)
        first = self.save.save("token", self.external)
        original = first.path.read_bytes()
        with patch("tools.teacher_content_files._publish_new", side_effect=OSError("failure")):
            with self.assertRaises(OSError):
                self.save.save("token", self.external)
        self.assertEqual(list(self.external.iterdir()), [first.path])
        self.assertEqual(first.path.read_bytes(), original)

    def test_changed_file_and_corruption_rejected(self):
        first = self.save.save("token", self.external)
        preview = self.restore.preview("token", first.path)
        self.source.revise("token", dict(self.deck, version=2, title="New"), 1)
        second = self.save.save("token", self.external)
        first.path.write_bytes(second.path.read_bytes())
        with self.assertRaises(ValueError):
            self.restore.restore("token", first.path, expected_sha256=preview.sha256, confirmed=True)
        first.path.write_bytes(b"damaged")
        with self.assertRaises(ValueError):
            read_archive(first.path)
        self.assertEqual(self.dest.list_decks("token"), [])
        with self.assertRaises(PermissionError):
            self.restore.restore("token", second.path, expected_sha256=second.sha256)

    def test_unauthorized_and_wrong_owner(self):
        first = self.save.save("token", self.external)
        self.sessions.require.return_value = "other"
        with self.assertRaises(PermissionError):
            self.restore.preview("token", first.path)
        self.sessions.require.side_effect = PermissionError
        with self.assertRaises(PermissionError):
            self.save.save("token", self.external)
