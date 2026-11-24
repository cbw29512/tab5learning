import tempfile
import unittest
import sqlite3
from pathlib import Path
from unittest.mock import Mock

from tools.teacher_decks import TeacherDecks
from tools.flashcard_deck import import_csv


class DeckStoreTests(unittest.TestCase):
    def test_scope_and_immutable_versions(self):
        with tempfile.TemporaryDirectory() as folder:
            sessions = Mock()
            sessions.require.return_value = "teacher1"
            sessions.accounts.role.return_value = "teacher"
            path = Path(folder) / "decks.sqlite"
            store = TeacherDecks(path, sessions)
            deck = import_csv("front,back\n2+2,4", "math", "Math")
            store.save("token", deck)
            self.assertEqual(TeacherDecks(path, sessions).list_decks("token"), [deck])
            with self.assertRaises(sqlite3.IntegrityError):
                store.save("token", deck)
            sessions.require.return_value = "teacher2"
            self.assertEqual(store.list_decks("token2"), [])
            sessions.accounts.role.return_value = "admin"
            self.assertEqual(store.list_decks("admin"), [deck])
            sessions.require.side_effect = PermissionError
            with self.assertRaises(PermissionError):
                store.save("bad", deck)
