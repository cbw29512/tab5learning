"""Local immutable deck storage behind Teacher Studio session checks."""
import json
import logging
import sqlite3
from contextlib import closing

from tools.flashcard_deck import validate_deck

LOG = logging.getLogger(__name__)


class TeacherDecks:
    def __init__(self, path, sessions):
        self.path = path
        self.sessions = sessions
        try:
            with closing(sqlite3.connect(path)) as db, db:
                db.execute("""CREATE TABLE IF NOT EXISTS decks (
                    deck_id TEXT NOT NULL, version INTEGER NOT NULL,
                    author TEXT NOT NULL, body TEXT NOT NULL,
                    PRIMARY KEY(deck_id, version))""")
        except sqlite3.Error:
            LOG.exception("Deck database setup failed")
            raise

    def save(self, token, deck):
        try:
            author = self.sessions.require(token)
            validate_deck(deck)
            body = json.dumps(deck, ensure_ascii=False, sort_keys=True)
            if len(body.encode("utf-8")) > 2 * 1024 * 1024:
                raise ValueError("Deck too large")
            with closing(sqlite3.connect(self.path)) as db, db:
                db.execute("INSERT INTO decks VALUES (?,?,?,?)",
                           (deck["deck_id"], deck["version"], author, body))
        except (sqlite3.Error, ValueError, PermissionError):
            LOG.warning("Deck save rejected")
            raise

    def list_decks(self, token):
        try:
            author = self.sessions.require(token)
            admin = self.sessions.accounts.role(author) == "admin"
            with closing(sqlite3.connect(self.path)) as db:
                rows = db.execute(
                    "SELECT body FROM decks WHERE author=? OR ?=1 ORDER BY deck_id,version",
                    (author, int(admin))).fetchall()
            return [json.loads(row[0]) for row in rows]
        except (sqlite3.Error, ValueError, PermissionError):
            LOG.warning("Deck listing rejected")
            raise
