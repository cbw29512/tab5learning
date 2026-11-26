import tempfile
import unittest
from pathlib import Path
from unittest.mock import Mock

from tools.flashcard_deck import import_csv
from tools.teacher_decks import TeacherDecks, StaleRevision
from tools.teacher_deck_editor import Draft, editor, handle_editor


class RevisionTests(unittest.TestCase):
    def setUp(self):
        folder = tempfile.TemporaryDirectory()
        self.addCleanup(folder.cleanup)
        self.sessions = Mock()
        self.sessions.require.return_value = "teacher1"
        self.sessions.accounts.role.return_value = "teacher"
        self.store = TeacherDecks(Path(folder.name) / "decks.sqlite", self.sessions)
        self.original = import_csv("front,back\n2+2,4\n3+3,6", "math", "Math")
        self.store.save("session", self.original)

    def form(self, draft, action):
        data = {"csrf": "test", "deck_id": draft.deck_id, "title": draft.title,
                "base_version": str(draft.base_version), "count": str(len(draft.rows)),
                "action": action}
        for i, (front, back) in enumerate(draft.rows):
            data[f"front_{i}"], data[f"back_{i}"] = front, back
        return data

    def test_revision_keeps_history_and_card_identity(self):
        draft = Draft.existing(self.store.latest("session", "math"))
        draft.rows[1] = ("3+3", "Six")
        data = self.form(draft, "preview")
        _, preview, error, saved = handle_editor(data, "test", self.store, "session")
        self.assertEqual(preview["version"], 2)
        self.assertFalse(error or saved)
        self.assertIn("Save new version", editor(draft, "test", preview=preview))
        data["action"] = "save"
        self.assertTrue(handle_editor(data, "test", self.store, "session")[3])
        old, new = self.store.list_decks("session")
        self.assertEqual(old, self.original)
        self.assertEqual(new["cards"][0]["card_id"], old["cards"][0]["card_id"])
        self.assertNotEqual(new["cards"][1]["card_id"], old["cards"][1]["card_id"])
        self.assertEqual(new["cards"][1]["back"], "Six")
        _, _, error, saved = handle_editor(data, "test", self.store, "session")
        self.assertFalse(saved)
        self.assertIn("has changed", error)
        self.assertEqual(len(self.store.list_decks("session")), 2)

    def test_owner_scope_and_admin_preserves_owner(self):
        draft = Draft.existing(self.original)
        self.sessions.require.return_value = "teacher2"
        with self.assertRaises(PermissionError):
            self.store.latest("session", "math")
        with self.assertRaises(PermissionError):
            self.store.revise("session", draft.deck(), 1)
        self.sessions.accounts.role.return_value = "admin"
        self.store.revise("session", draft.deck(), 1)
        self.sessions.accounts.role.return_value = "teacher"
        self.assertEqual(self.store.list_decks("session"), [])
        self.sessions.require.return_value = "teacher1"
        self.assertEqual(len(self.store.list_decks("session")), 2)

    def test_stale_revision_rejected(self):
        candidate = Draft.existing(self.original).deck()
        self.store.revise("session", candidate, 1)
        with self.assertRaises(StaleRevision):
            self.store.revise("session", candidate, 1)

    def test_large_import_not_truncated(self):
        deck = import_csv("front,back\n" + "q,a\n"*26, "large", "Large")
        self.store.save("session", deck)
        with self.assertRaises(ValueError):
            Draft.existing(deck)
        forged = Draft("large", "Large", [("q", "a")], 1)
        with self.assertRaises(ValueError):
            handle_editor(self.form(forged, "save"), "test", self.store, "session")
        self.assertEqual(self.store.latest("session", "large"), deck)
