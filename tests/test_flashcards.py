import unittest

from tools.flashcard_deck import import_csv, validate_deck
from tools.flashcard_review import next_review


class FlashcardTests(unittest.TestCase):
    def test_csv(self):
        deck = import_csv('front,back\n"7 + 5",12\nCat,Animal\n', "math", "Practice")
        self.assertEqual(deck["cards"][0]["back"], "12")
        self.assertEqual(deck["cards"][1]["card_id"], "card-2")

    def test_invalid_csv(self):
        for data in ("question,answer\nx,y", "front,back\nx", "front,back\nx,y,z",
                     "front,back\n,answer", "front,back\n"):
            with self.subTest(data=data), self.assertRaises(ValueError):
                import_csv(data, "math", "Practice")

    def test_duplicate_id(self):
        deck = import_csv("front,back\nx,y\na,b", "math", "Practice")
        deck["cards"][1]["card_id"] = deck["cards"][0]["card_id"]
        with self.assertRaises(ValueError):
            validate_deck(deck)

    def test_progression(self):
        self.assertEqual(next_review(0, "independent", 10)["due_day"], 13)
        self.assertEqual(next_review(1, "independent", 10)["due_day"], 17)
        self.assertEqual(next_review(2, "independent", 10)["due_day"], 40)
        self.assertEqual(next_review(3, "missed", 10)["streak"], 0)

    def test_uncertain_and_self_rating(self):
        self.assertEqual(next_review(2, "uncertain", 10),
                         {"streak": 2, "due_day": None, "needs_review": True})
        self.assertEqual(next_review(0, "self_known", 10)["streak"], 0)

    def test_invalid_day(self):
        with self.assertRaises(ValueError):
            next_review(0, "independent", False)
