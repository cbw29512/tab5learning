import tempfile
import unittest
from pathlib import Path
from unittest.mock import Mock

from tools.flashcard_deck import import_csv
from tools.teacher_decks import TeacherDecks
from tools.teacher_content_archive import PreviewTeacherArchive, RestoreSummary
from tools.teacher_content_codec import encode
from tools.protected_teacher_archive import new_recovery_key, protect, unprotect
from tools.protected_teacher_files import ProtectedPreviewArchiveFiles


class ProtectedArchiveTests(unittest.TestCase):
    def test_authenticated_envelope(self):
        raw = encode({"owner": "teacher1", "decks": []})
        key = new_recovery_key()
        encrypted = protect(raw, key)
        self.assertNotEqual(protect(raw, key), encrypted)
        self.assertEqual(unprotect(encrypted, key), raw)
        self.assertNotIn(b"teacher1", encrypted)
        with self.assertRaises(ValueError):
            unprotect(encrypted, new_recovery_key())
        for offset in (0, 5, len(encrypted)-1):
            changed = bytearray(encrypted)
            changed[offset] ^= 1
            with self.assertRaises(ValueError):
                unprotect(bytes(changed), key)
        with self.assertRaises(ValueError):
            protect(raw, "password")
        with self.assertRaises(ValueError):
            unprotect(raw, key)

    def test_independent_restore(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            workspace = root / "workspace"
            workspace.mkdir()
            external = root / "external"
            external.mkdir()
            sessions = Mock()
            sessions.require.return_value = "teacher1"
            sessions.accounts.role.return_value = "teacher"
            source = TeacherDecks(workspace / "source.sqlite", sessions)
            dest = TeacherDecks(workspace / "dest.sqlite", sessions)
            source.save("token", import_csv("front,back\nquestion,answer", "math", "Math"))
            saver = ProtectedPreviewArchiveFiles(PreviewTeacherArchive(source, test_mode=True))
            restorer = ProtectedPreviewArchiveFiles(PreviewTeacherArchive(dest, test_mode=True))
            key = new_recovery_key()
            receipt = saver.save("token", external, recovery_key=key)
            self.assertEqual(receipt.path.suffix, ".plte")
            self.assertNotIn(b"question", receipt.path.read_bytes())
            with self.assertRaises(ValueError):
                restorer.preview("token", receipt.path, recovery_key=new_recovery_key())
            preview = restorer.preview("token", receipt.path, recovery_key=key)
            self.assertEqual(preview.summary, RestoreSummary(0, 1))
            with self.assertRaises(PermissionError):
                restorer.restore("token", receipt.path, recovery_key=key, expected_sha256=preview.sha256)
            with self.assertRaises(ValueError):
                restorer.restore("token", receipt.path, recovery_key=key,
                                  expected_sha256="0"*64, confirmed=True)
            self.assertEqual(restorer.restore("token", receipt.path, recovery_key=key,
                expected_sha256=preview.sha256, confirmed=True), RestoreSummary(0, 1))
            self.assertEqual(dest.list_decks("token"), source.list_decks("token"))
            sessions.require.return_value = "teacher2"
            with self.assertRaises(PermissionError):
                restorer.preview("token", receipt.path, recovery_key=key)
