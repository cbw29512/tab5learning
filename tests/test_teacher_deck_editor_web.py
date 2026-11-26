import io
import tempfile
import unittest
from pathlib import Path
from urllib.parse import urlencode

from tools.teacher_deck_editor import Draft
from tools.teacher_web import TeacherWeb, HOST, ORIGIN, COOKIE


class VisualEditorWebTests(unittest.TestCase):
    def setUp(self):
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        self.app = TeacherWeb(Path(directory.name))
        self.app.accounts.create("admin", "synthetic-admin-pass", "admin", bootstrap=True)
        self.token = self.app.sessions.login("admin", "synthetic-admin-pass")
        self.csrf = self.app.csrf(self.token)

    def request(self, path="/decks/new", form=None, **overrides):
        body = urlencode(form or {}).encode()
        env = {"HTTP_HOST": HOST, "REMOTE_ADDR": "127.0.0.1",
               "HTTP_ORIGIN": ORIGIN, "HTTP_COOKIE": f"{COOKIE}={self.token}",
               "REQUEST_METHOD": "GET" if form is None else "POST",
               "PATH_INFO": path, "CONTENT_LENGTH": str(len(body)),
               "CONTENT_TYPE": "application/x-www-form-urlencoded",
               "wsgi.input": io.BytesIO(body)}
        env.update(overrides)
        status = []
        page = b"".join(self.app(env, lambda value, headers: status.append(value)))
        return int(status[0].split()[0]), page.decode()

    def draft_form(self):
        draft = Draft.new()
        data = {"csrf": self.csrf, "count": "5", "deck_id": draft.deck_id,
                "title": "Five math cards", "action": "preview"}
        for i in range(5):
            data[f"front_{i}"] = f"{i}+1"
            data[f"back_{i}"] = str(i+1)
        return data

    def test_editor_workflow(self):
        self.assertIn("Create flashcards", self.request("/")[1])
        self.assertIn("Teacher sign in", self.request(HTTP_COOKIE="")[1])
        status, page = self.request()
        self.assertEqual(status, 200)
        self.assertIn("Question / front", page)
        data = self.draft_form()
        self.assertEqual(self.request(form=dict(data, csrf="bad"))[0], 403)
        self.assertEqual(self.request(form=data, HTTP_ORIGIN="https://evil.example")[0], 403)
        status, page = self.request(form=data)
        self.assertEqual(status, 200)
        self.assertIn("Preview — 5 cards", page)
        self.assertEqual(self.app.decks.list_decks(self.token), [])
        status, page = self.request(form=dict(data, action="save"))
        self.assertEqual(status, 200)
        self.assertIn("Flashcard deck saved successfully.", page)
        self.assertEqual(len(self.app.decks.list_decks(self.token)), 1)
        self.assertIn("already saved", self.request(form=dict(data, action="save"))[1])
        self.assertEqual(len(self.app.decks.list_decks(self.token)), 1)

    def test_invalid_card_keeps_draft(self):
        data = self.draft_form()
        data["back_2"] = ""
        status, page = self.request(form=data)
        self.assertEqual(status, 200)
        self.assertIn("Complete both sides of card 3", page)
        self.assertIn("2+1", page)
        self.assertEqual(self.app.decks.list_decks(self.token), [])
