import re
from unittest.mock import Mock
import unittest

from tools.teacher_bulk_flashcards import AI_PROMPT, handle_bulk, bulk_form


class BulkFlashcardTests(unittest.TestCase):
    def test_prompt_and_preview_save(self):
        self.assertIn("front,back", AI_PROMPT)
        self.assertIn("Do not share student", bulk_form("csrf"))
        decks = Mock()
        args = dict(csrf="csrf", key=b"test key", token="session", decks=decks)
        data = dict(csrf="csrf", title="Science", csv='front,back\n"<script>", "plant"'.replace(', ', ','), proof="", action="preview")
        page, saved = handle_bulk(data, **args)
        self.assertFalse(saved)
        self.assertIn("Review 1 cards", page)
        self.assertNotIn("<script>", page)
        decks.save.assert_not_called()
        proof = re.search(r'name="proof" value="([a-f0-9]{64})"', page).group(1)
        page, saved = handle_bulk(dict(data, action="save", proof=proof, title="Changed"), **args)
        self.assertFalse(saved)
        self.assertIn("Review again", page)
        decks.save.assert_not_called()
        _, saved = handle_bulk(dict(data, action="save", proof=proof), **args)
        self.assertTrue(saved)
        decks.save.assert_called_once()
        self.assertEqual(decks.save.call_args.args[1]["title"], "Science")

    def test_invalid_and_cross_session(self):
        decks = Mock()
        args = dict(csrf="csrf", key=b"key", token="one", decks=decks)
        data = dict(csrf="csrf", title="Math", csv="front,back\n2+2,4", proof="", action="preview")
        page, saved = handle_bulk(dict(data, csv="bad,header"), **args)
        self.assertFalse(saved)
        self.assertIn("Invalid CSV", page)
        page, _ = handle_bulk(data, **args)
        proof = re.search(r'name="proof" value="([a-f0-9]{64})"', page).group(1)
        _, saved = handle_bulk(dict(data, proof=proof, action="save"), **dict(args, token="two"))
        self.assertFalse(saved)
        decks.save.assert_not_called()
