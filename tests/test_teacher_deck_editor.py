import sqlite3
import unittest
from unittest.mock import Mock

from tools.teacher_deck_editor import Draft, editor, handle_editor


def form(draft, action):
    data = {"csrf": "test", "count": str(len(draft.rows)), "deck_id": draft.deck_id,
            "title": draft.title, "action": action}
    for i, (front, back) in enumerate(draft.rows):
        data[f"front_{i}"], data[f"back_{i}"] = front, back
    return data


class DeckEditorTests(unittest.TestCase):
    def setUp(self):
        self.draft = Draft.new()
        self.draft.title = "Math"
        self.draft.rows[0] = ("2+2", "4")
        self.decks = Mock()

    def apply(self, action):
        return handle_editor(form(self.draft, action), "test", self.decks, "token")

    def test_preview_and_save(self):
        for i in range(5):
            self.draft.rows[i] = (f"Question {i}", f"Answer {i}")
        _, deck, error, saved = self.apply("preview")
        self.assertEqual(len(deck["cards"]), 5)
        self.assertFalse(error or saved)
        self.decks.save.assert_not_called()
        self.assertTrue(self.apply("save")[3])
        self.decks.save.assert_called_once_with("token", deck)

    def test_add_remove_preserves_text(self):
        self.draft, _, _, _ = self.apply("add")
        self.assertEqual(len(self.draft.rows), 6)
        self.draft.rows[-1] = ("Last", "Answer")
        self.draft, _, _, _ = self.apply("remove:1")
        self.assertEqual(self.draft.rows[0], ("2+2", "4"))
        self.assertEqual(self.draft.rows[-1], ("Last", "Answer"))

    def test_partial_card_preserved(self):
        self.draft.rows[1] = ("Unfinished", "")
        draft, preview, error, saved = self.apply("preview")
        self.assertEqual(draft.rows[1], ("Unfinished", ""))
        self.assertIn("card 2", error)
        self.assertIsNone(preview)
        self.assertFalse(saved)
        self.decks.save.assert_not_called()

    def test_invalid_bounds_and_fields(self):
        data = form(self.draft, "preview")
        with self.assertRaises(ValueError):
            Draft.read(dict(data, count="26"))
        with self.assertRaises(ValueError):
            Draft.read(dict(data, front_0="x"*501))
        with self.assertRaises(ValueError):
            Draft.read(dict(data, unknown="x"))

    def test_duplicate_save(self):
        self.decks.save.side_effect = sqlite3.IntegrityError
        _, deck, error, saved = self.apply("save")
        self.assertFalse(saved)
        self.assertIn("already saved", error)
        self.assertEqual(deck["deck_id"], self.draft.deck_id)

    def test_escape(self):
        self.draft.title = '<script>alert("x")</script>'
        self.draft.rows[0] = ("</textarea><script>bad</script>", "<img src=x>")
        html = editor(self.draft, "csrf", preview=self.draft.deck(), error="<bad>")
        self.assertNotIn("<script>", html)
        self.assertNotIn("<img src=x>", html)
        self.assertIn("&lt;script&gt;", html)
        self.assertIn("&lt;bad&gt;", html)
