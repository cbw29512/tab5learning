import re
import unittest
from unittest.mock import Mock

from tools.teacher_flashcard_prompt import build_prompt, prompt_panel
from tools.teacher_bulk_flashcards import handle_bulk


class PromptTests(unittest.TestCase):
    def test_all_grades_and_subjects(self):
        from tools.teacher_flashcard_prompt import SUBJECTS
        for grade in ("1", "2", "3", "4"):
            for subject in SUBJECTS:
                prompt = build_prompt(grade, subject, "50", "Lesson notes")
                self.assertIn(f"50 age-appropriate study flashcards for Grade {grade} {subject}", prompt)
                self.assertIn("front,back", prompt)
                self.assertTrue(prompt.endswith("Lesson notes"))

    def test_limits_and_escaped_notes(self):
        for grade, subject, count, notes in (
            ("5", "Science", "30", ""), ("1", "Unknown", "30", ""),
            ("1", "Science", "101", ""), ("1", "Science", "0", ""),
            ("1", "Science", "1", "é" * 6001),
        ):
            with self.assertRaises(ValueError):
                build_prompt(grade, subject, count, notes)
        page = prompt_panel("csrf", notes="<script>alert(1)</script>")
        self.assertIn("&lt;script&gt;", page)
        self.assertNotIn("<script>", page)

    def test_generate_no_save_and_browser_line_endings(self):
        decks = Mock()
        args = dict(csrf="csrf", key=b"key", token="session", decks=decks)
        page, saved = handle_bulk(dict(csrf="csrf", action="generate", grade="4",
                                      subject="Science", count="50", notes="Plants"), **args)
        self.assertFalse(saved)
        self.assertIn("50 age-appropriate study flashcards for Grade 4 Science", page)
        self.assertIn("Plants", page)
        decks.save.assert_not_called()
        form = dict(csrf="csrf", title="Science", csv='front,back\n"Plant","Living thing"',
                    action="preview", proof="")
        page, saved = handle_bulk(form, **args)
        self.assertFalse(saved)
        proof = re.search(r'name="proof" value="([a-f0-9]{64})"', page).group(1)
        _, saved = handle_bulk(dict(form, action="save", proof=proof,
                                   csv=form["csv"].replace("\n", "\r\n")), **args)
        self.assertTrue(saved)
        decks.save.assert_called_once()
