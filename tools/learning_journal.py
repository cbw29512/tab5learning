"""Host SQLite reference journal; not ESP32 firmware or USB authorization."""
import json
import logging
import sqlite3
from contextlib import closing

from tools.learning_event import canonical_event

LOG = logging.getLogger(__name__)


class LearningJournal:
    """One provisioned student per database; durable sequence assigned locally."""

    def __init__(self, path, device_id, student_id):
        self.path = path
        self.identity = (device_id, student_id)
        try:
            with closing(self._connect()) as db, db:
                db.execute("BEGIN IMMEDIATE")
                db.execute("CREATE TABLE IF NOT EXISTS identity (slot INTEGER PRIMARY KEY CHECK(slot=1), device TEXT NOT NULL, student TEXT NOT NULL)")
                db.execute("CREATE TABLE IF NOT EXISTS events (sequence INTEGER PRIMARY KEY AUTOINCREMENT, event_id TEXT UNIQUE NOT NULL, body TEXT NOT NULL)")
                db.execute("INSERT OR IGNORE INTO identity VALUES (1, ?, ?)", self.identity)
                self._check_identity(db)
        except (sqlite3.Error, ValueError):
            LOG.exception("Journal initialization failed")
            raise

    def _connect(self):
        try:
            db = sqlite3.connect(self.path, timeout=5)
            try:
                db.execute("PRAGMA synchronous=FULL")
                return db
            except sqlite3.Error:
                db.close()
                raise
        except sqlite3.Error:
            LOG.exception("Journal connection failed")
            raise

    def _check_identity(self, db):
        if db.execute("SELECT device, student FROM identity WHERE slot=1").fetchone() != self.identity:
            raise ValueError("Wrong journal identity")

    def append(self, event):
        """Acknowledge only after commit; exact retries return original sequence."""
        try:
            body = canonical_event(event)
            if (event["device_id"], event["student_id"]) != self.identity:
                raise ValueError("Wrong event identity")
            with closing(self._connect()) as db, db:
                db.execute("BEGIN IMMEDIATE")
                self._check_identity(db)
                existing = db.execute("SELECT sequence, body FROM events WHERE event_id=?", (event["event_id"],)).fetchone()
                if existing:
                    if existing[1] != body:
                        raise ValueError("Conflicting event ID")
                    sequence = existing[0]
                else:
                    sequence = db.execute("INSERT INTO events(event_id, body) VALUES (?, ?)", (event["event_id"], body)).lastrowid
            return sequence
        except (sqlite3.Error, ValueError, TypeError):
            LOG.warning("Journal append rejected or failed")
            raise

    def timeline(self, after=0, limit=100):
        """Read bounded ordered pages; the caller must authorize teacher access."""
        try:
            if type(after) is not int or after < 0 or type(limit) is not int or not 1 <= limit <= 500:
                raise ValueError("Invalid timeline page")
            with closing(self._connect()) as db:
                self._check_identity(db)
                rows = db.execute("SELECT sequence, body FROM events WHERE sequence>? ORDER BY sequence LIMIT ?", (after, limit)).fetchall()
            return [{"sequence": seq, **json.loads(body)} for seq, body in rows]
        except (sqlite3.Error, ValueError, TypeError):
            LOG.warning("Journal read failed")
            raise
