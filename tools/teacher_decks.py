"""Local immutable deck storage behind Teacher Studio session checks."""
import json
import logging
import sqlite3
from collections import defaultdict, deque
from uuid import uuid4
from contextlib import closing

from tools.flashcard_deck import ID, validate_deck

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
            if deck["version"] != 1:
                raise ValueError("New decks must start at version 1; use revise for updates")
            body = json.dumps(deck, ensure_ascii=False, sort_keys=True)
            if len(body.encode("utf-8")) > 2 * 1024 * 1024:
                raise ValueError("Deck too large")
            with closing(sqlite3.connect(self.path)) as db, db:
                db.execute("BEGIN IMMEDIATE")
                # Creation must not attach a new owner to any existing deck history.
                if db.execute("SELECT 1 FROM decks WHERE deck_id=? LIMIT 1",
                              (deck["deck_id"],)).fetchone():
                    raise sqlite3.IntegrityError("Deck ID already exists")
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

    def _latest(self, db, token, deck_id):
        actor = self.sessions.require(token)
        if not isinstance(deck_id, str) or not ID.fullmatch(deck_id):
            raise ValueError("Invalid deck selection")
        row = db.execute(
            "SELECT author,body FROM decks WHERE deck_id=? ORDER BY version DESC LIMIT 1",
            (deck_id,)).fetchone()
        if not row or (row[0] != actor and self.sessions.accounts.role(actor) != "admin"):
            raise PermissionError("Deck unavailable")
        return row[0], validate_deck(json.loads(row[1]))

    def latest(self, token, deck_id):
        try:
            with closing(sqlite3.connect(self.path)) as db:
                return self._latest(db, token, deck_id)[1]
        except (ValueError, PermissionError, sqlite3.Error):
            LOG.warning("Deck selection rejected")
            raise

    def revise(self, token, candidate, base_version):
        """Append a revision atomically, preserving author and old versions."""
        try:
            validate_deck(candidate)
            if type(base_version) is not int or base_version < 1:
                raise ValueError("Invalid base version")
            if candidate["version"] != base_version + 1:
                raise ValueError("Invalid revision number")
            with closing(sqlite3.connect(self.path)) as db, db:
                db.execute("BEGIN IMMEDIATE")
                owner, previous = self._latest(db, token, candidate["deck_id"])
                if previous["version"] != base_version:
                    raise StaleRevision("This deck has changed. Reopen its latest version before saving.")
                # Reuse only identical card content. Never recycle a changed ID.
                unchanged = defaultdict(deque)
                for card in previous["cards"]:
                    unchanged[(card["front"], card["back"])].append(card["card_id"])
                cards = []
                for card in candidate["cards"]:
                    ids = unchanged[(card["front"], card["back"])]
                    cards.append({**card, "card_id": ids.popleft() if ids else uuid4().hex})
                revision = validate_deck({**candidate, "cards": cards})
                body = json.dumps(revision, ensure_ascii=False, sort_keys=True)
                if len(body.encode("utf-8")) > 2 * 1024 * 1024:
                    raise ValueError("Deck too large")
                db.execute("INSERT INTO decks VALUES (?,?,?,?)",
                           (revision["deck_id"], revision["version"], owner, body))
                return revision
        except (ValueError, PermissionError, sqlite3.Error):
            LOG.warning("Deck revision rejected")
            raise


class StaleRevision(ValueError):
    """The submitted draft is no longer based on the latest saved version."""
