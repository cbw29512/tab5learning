"""Isolated synthetic-only backup/restore of immutable teacher text decks."""
import json
import logging
import sqlite3
from contextlib import closing
from dataclasses import dataclass

from tools.teacher_content_codec import encode, decode, MAX_BYTES, MAX_VERSIONS
from tools.flashcard_deck import validate_deck

LOG = logging.getLogger(__name__)


@dataclass(frozen=True)
class RestoreSummary:
    existing: int
    new: int


class PreviewTeacherArchive:
    def __init__(self, decks, *, test_mode=False):
        if test_mode is not True:
            raise PermissionError("Synthetic archive preview only")
        self.decks = decks

    def export(self, token):
        """Read one consistent owner-only snapshot; no account or student tables."""
        try:
            actor = self.decks.sessions.require(token)
            with closing(sqlite3.connect(self.decks.path)) as db, db:
                db.execute("BEGIN")
                count, size = db.execute(
                    "SELECT count(*),coalesce(sum(length(CAST(body AS BLOB))),0) "
                    "FROM decks WHERE author=?", (actor,)).fetchone()
                if count > MAX_VERSIONS or size > MAX_BYTES:
                    raise ValueError("Archive exceeds preview capacity")
                rows = db.execute(
                    "SELECT deck_id,version,body FROM decks WHERE author=? "
                    "ORDER BY deck_id,version", (actor,)).fetchall()
                items = []
                for ident, version, body in rows:
                    deck = validate_deck(json.loads(body))
                    if (deck["deck_id"], deck["version"]) != (ident, version):
                        raise ValueError("Stored deck identity mismatch")
                    items.append(deck)
                return encode({"owner": actor, "decks": items})
        except (ValueError, PermissionError, sqlite3.Error, TypeError):
            LOG.warning("Teaching archive export rejected")
            raise

    def _merge(self, token, raw, *, apply):
        try:
            actor = self.decks.sessions.require(token)
            archive = decode(raw)
            # Same-account synthetic recovery only; never claim identity
            # portability based on a username in an unsigned archive.
            if archive["owner"] != actor:
                raise PermissionError("Archive owner mismatch")
            existing = new = 0
            with closing(sqlite3.connect(self.decks.path)) as db, db:
                db.execute("PRAGMA synchronous=FULL")
                db.execute("BEGIN IMMEDIATE" if apply else "BEGIN")
                for deck in archive["decks"]:
                    ident, version = deck["deck_id"], deck["version"]
                    row = db.execute(
                        "SELECT author,body FROM decks WHERE deck_id=? AND version=?",
                        (ident, version)).fetchone()
                    if row:
                        if row[0] != actor or validate_deck(json.loads(row[1])) != deck:
                            raise ValueError("Conflicting deck version")
                        existing += 1
                        continue
                    # Never fill a hole underneath later history or attach a
                    # version to another teacher's deck. A preview is advisory;
                    # apply rechecks all conditions under a write transaction.
                    latest = db.execute(
                        "SELECT author,version FROM decks WHERE deck_id=? "
                        "ORDER BY version DESC LIMIT 1", (ident,)).fetchone()
                    if latest and (latest[0] != actor or latest[1] >= version):
                        raise ValueError("Conflicting deck history")
                    new += 1
                    if apply:
                        db.execute("INSERT INTO decks VALUES (?,?,?,?)",
                                   (ident, version, actor, json.dumps(
                                       deck, ensure_ascii=False, sort_keys=True)))
                # Validate actual rows before transaction commit.
                if apply:
                    for deck in archive["decks"]:
                        row = db.execute(
                            "SELECT author,body FROM decks WHERE deck_id=? AND version=?",
                            (deck["deck_id"], deck["version"])).fetchone()
                        if not row or row[0] != actor or json.loads(row[1]) != deck:
                            raise ValueError("Restore verification failed")
                return RestoreSummary(existing, new)
        except (ValueError, PermissionError, sqlite3.Error, TypeError):
            LOG.warning("Teaching archive restore rejected")
            raise

    def preview(self, token, raw):
        return self._merge(token, raw, apply=False)

    def restore(self, token, raw, *, confirmed=False):
        if confirmed is not True:
            raise PermissionError("Explicit restore confirmation required")
        return self._merge(token, raw, apply=True)
