"""Test-only host credential preparation. Never returns key material to UI."""
import logging
import secrets
import sqlite3
from contextlib import closing
from datetime import datetime, timezone

from tools.teacher_registry import normalize

LOG = logging.getLogger(__name__)


class DeviceEnrollment:
    def __init__(self, path, sessions, registry):
        self.path = path
        self.sessions = sessions
        self.registry = registry
        try:
            with closing(sqlite3.connect(path)) as db, db:
                db.execute("""CREATE TABLE IF NOT EXISTS device_credentials (
                    device_id TEXT PRIMARY KEY,
                    secret BLOB NOT NULL CHECK(length(secret)=32),
                    actor TEXT NOT NULL,
                    created_at TEXT NOT NULL,
                    status TEXT NOT NULL CHECK(status='KEY_PENDING')
                )""")
        except sqlite3.Error:
            LOG.exception("Test credential database setup failed")
            raise

    def prepare(self, token, device_id):
        try:
            actor = self.sessions.require(token, "admin")
            device_id = normalize(device_id)
            if not any(row["device_id"] == device_id for row in self.registry.list_assignments(token)):
                raise ValueError("Device requires a planned assignment")
            with closing(sqlite3.connect(self.path)) as db, db:
                db.execute("INSERT INTO device_credentials VALUES (?,?,?,?,?)",
                           (device_id, secrets.token_bytes(32), actor,
                            datetime.now(timezone.utc).isoformat(), "KEY_PENDING"))
        except (ValueError, PermissionError, sqlite3.IntegrityError):
            LOG.warning("Credential preparation rejected")
            raise
        except sqlite3.Error:
            LOG.exception("Credential storage failed")
            raise

    def pending(self, token):
        self.sessions.require(token, "admin")
        try:
            with closing(sqlite3.connect(self.path)) as db:
                return {row[0]: row[1] for row in db.execute(
                    "SELECT device_id,status FROM device_credentials")}
        except sqlite3.Error:
            LOG.exception("Credential status retrieval failed")
            raise
