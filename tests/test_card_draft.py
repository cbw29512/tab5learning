import unittest
from tools.card_draft import CardDraft


class DraftTests(unittest.TestCase):
    def test_edit_preserves_identity_and_snapshot(self):
        draft = CardDraft()
        draft.put("2+2", "4")
        before = draft.build("Math")
        draft.put("2+3", "5", 0)
        after = draft.build("Math")
        self.assertEqual(before["cards"][0]["card_id"], after["cards"][0]["card_id"])
        self.assertEqual(before["cards"][0]["front"], "2+2")
        self.assertEqual(after["cards"][0]["front"], "2+3")

    def test_invalid_draft(self):
        draft = CardDraft()
        with self.assertRaises(ValueError):
            draft.put("", "answer")
        with self.assertRaises(ValueError):
            draft.remove(0)
        with self.assertRaises(ValueError):
            draft.build("Empty")
        draft.put("a", "b")
        draft.remove(0)
        self.assertEqual(draft.cards, [])
