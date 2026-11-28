import re
import unittest
from unittest.mock import Mock

from tools.flashcard_deck import import_csv
from tools.flashcard_quality import inspect_cards, without_identical_cards
from tools.teacher_bulk_flashcards import handle_bulk


class FlashcardQualityTests(unittest.TestCase):
    def test_exact_only_and_conflict_warning(self):
        deck = import_csv('front,back\n"m","meter"\n"M","capital"\n"m","meter"\n"m","other"\n'
                          '"Comma, question","A ""quote"""', "deck", "Science")
        found = inspect_cards(deck)
        self.assertEqual(found.duplicates, ((1, 3),))
        self.assertEqual(found.conflicts, ((1, 4),))
        text, removed = without_identical_cards(deck)
        cleaned = import_csv(text, "deck", "Science")
        self.assertEqual(removed, 1)
        self.assertEqual([(c["front"], c["back"]) for c in cleaned["cards"]],
                         [("m", "meter"), ("M", "capital"), ("m", "other"),
                          ("Comma, question", 'A "quote"')])
        self.assertEqual(len(deck["cards"]), 5)

    def test_explicit_preview_cleanup_and_save(self):
        decks = Mock()
        args = dict(csrf="csrf", key=b"key", token="session", decks=decks)
        form = dict(csrf="csrf", title="Math", csv="front,back\n2+2,4\n2+2,4\n2+2,5",
                    action="preview", proof="")
        page, saved = handle_bulk(form, **args)
        self.assertFalse(saved)
        self.assertIn("1 identical duplicate cards", page)
        self.assertIn("Repeated questions with different answers", page)
        proof = re.search(r'name="proof" value="([a-f0-9]{64})"', page).group(1)
        rejected, saved = handle_bulk(dict(form, action="deduplicate", proof="bad"), **args)
        self.assertFalse(saved)
        self.assertIn("Review again before removing", rejected)
        page, saved = handle_bulk(dict(form, action="deduplicate", proof=proof), **args)
        self.assertFalse(saved)
        self.assertIn("Review 2 cards", page)
        self.assertIn("Removed 1 identical duplicate cards", page)
        self.assertIn("Repeated questions with different answers", page)
        decks.save.assert_not_called()
        from html import unescape
        cleaned = unescape(re.search(r'<textarea id="csv"[^>]*>(.*?)</textarea>', page, re.S).group(1))
        new_proof = re.search(r'name="proof" value="([a-f0-9]{64})"', page).group(1)
        _, saved = handle_bulk(dict(form, csv=cleaned, action="save", proof=proof), **args)
        self.assertFalse(saved)
        _, saved = handle_bulk(dict(form, csv=cleaned, action="save", proof=new_proof), **args)
        self.assertTrue(saved)
        self.assertEqual(len(decks.save.call_args.args[1]["cards"]), 2)

    def test_large_deck_does_not_truncate(self):
        text = "front,back\n" + "\n".join(f"Question {i},Answer {i}" for i in range(499))
        text += "\nQuestion 0,Answer 0"
        deck = import_csv(text, "large", "Large")
        output, removed = without_identical_cards(deck)
        self.assertEqual(removed, 1)
        self.assertEqual(len(import_csv(output, "large", "Large")["cards"]), 499)
