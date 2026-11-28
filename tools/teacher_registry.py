"""Local planned assignments; never claims USB provisioning."""
import logging
import re
import sqlite3
from contextlib import closing
from datetime import datetime, timezone

LOG = logging.getLogger(__name__)
IDENTIFIER = re.compile(r"[A-Za-z0-9_-]{1,64}\Z")


def normalize(value):
    if not isinstance(value, str) or not IDENTIFIER.fullmatch(value):
        raise ValueError("Use a 1–64 character anonymous identifier")
    return value.lower()


class TeacherRegistry:
    def __init__(self, path, sessions):
        self.path = path
        self.sessions = sessions
        try:
            with closing(sqlite3.connect(path)) as db, db:
                db.execute("""CREATE TABLE IF NOT EXISTS planned_assignments (
                    student_id TEXT PRIMARY KEY,
                    device_id TEXT NOT NULL UNIQUE,
                    starting_grade INTEGER NOT NULL CHECK(starting_grade BETWEEN 1 AND 3),
                    owner TEXT NOT NULL,
                    created_at TEXT NOT NULL,
                    status TEXT NOT NULL CHECK(status='PLANNED')
                )""")
        except sqlite3.Error:
            LOG.exception("Assignment registry setup failed")
            raise

    def create(self, token, student_id, device_id, grade):
        try:
            owner = self.sessions.require(token)
            student_id, device_id = normalize(student_id), normalize(device_id)
            if type(grade) is not int or grade not in (1, 2, 3):
                raise ValueError("Starting grade must be 1–3")
            with closing(sqlite3.connect(self.path)) as db, db:
                db.execute("INSERT INTO planned_assignments VALUES (?,?,?,?,?,?)",
                           (student_id, device_id, grade, owner,
                            datetime.now(timezone.utc).isoformat(), "PLANNED"))
        except (ValueError, PermissionError, sqlite3.IntegrityError):
            LOG.warning("Assignment rejected")
            raise
        except sqlite3.Error:
            LOG.exception("Assignment storage failed")
            raise

    def list_assignments(self, token):
        try:
            owner = self.sessions.require(token)
            admin = self.sessions.accounts.role(owner) == "admin"
            with closing(sqlite3.connect(self.path)) as db:
                db.row_factory = sqlite3.Row
                rows = db.execute(
                    "SELECT * FROM planned_assignments WHERE owner=? OR ?=1 ORDER BY student_id",
                    (owner, int(admin)))
                return [dict(row) for row in rows]
        except (PermissionError, sqlite3.Error):
            LOG.warning("Assignment listing failed")
            raise
