"""Test-only host credential preparation. Never returns key material to UI."""
import logging
import secrets
import sqlite3
from contextlib import closing
from datetime import datetime, timezone

from tools.teacher_registry import normalize
from tools.device_credential_record import encode_credential, decode_credential

LOG = logging.getLogger(__name__)


class DeviceEnrollment:
    def __init__(self, path, sessions, registry, *, protector=None):
        self.path = path
        self.sessions = sessions
        self.registry = registry
        self.protector = protector
        try:
            with closing(sqlite3.connect(path)) as db, db:
                db.execute("""CREATE TABLE IF NOT EXISTS device_credentials (
                    device_id TEXT PRIMARY KEY,
                    secret BLOB NOT NULL CHECK(length(secret)=32),
                    actor TEXT NOT NULL,
                    created_at TEXT NOT NULL,
                    status TEXT NOT NULL CHECK(status='KEY_PENDING')
                )""")
                db.execute("""CREATE TABLE IF NOT EXISTS protected_device_credentials (
                    device_id TEXT PRIMARY KEY,
                    protected BLOB NOT NULL CHECK(length(protected) BETWEEN 1 AND 4096),
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
                # Serialize duplicate checks across the legacy and protected tables.
                db.execute("BEGIN IMMEDIATE")
                if db.execute("SELECT device_id FROM device_credentials WHERE device_id=? "
                              "UNION ALL SELECT device_id FROM protected_device_credentials WHERE device_id=?",
                              (device_id, device_id)).fetchone():
                    raise sqlite3.IntegrityError("Device credential already exists")
                key = secrets.token_bytes(32)
                created = datetime.now(timezone.utc).isoformat()
                if self.protector is None:
                    db.execute("INSERT INTO device_credentials VALUES (?,?,?,?,?)",
                               (device_id, key, actor, created, "KEY_PENDING"))
                else:
                    # A protection error rolls back; never fall back to plaintext.
                    record = encode_credential(device_id, key)
                    blob = self.protector.protect(record, self._context(device_id))
                    if not isinstance(blob, bytes) or not 0 < len(blob) <= 4096:
                        raise ValueError("Invalid protected credential blob")
                    db.execute("INSERT INTO protected_device_credentials VALUES (?,?,?,?,?)",
                               (device_id, blob, actor, created, "KEY_PENDING"))
        except (ValueError, PermissionError, sqlite3.IntegrityError):
            LOG.warning("Credential preparation rejected")
            raise
        except (sqlite3.Error, OSError):
            LOG.warning("Credential storage or protection failed")
            raise

    def pending(self, token):
        self.sessions.require(token, "admin")
        try:
            with closing(sqlite3.connect(self.path)) as db:
                return {row[0]: row[1] for row in db.execute(
                    "SELECT device_id,status FROM device_credentials UNION ALL "
                    "SELECT device_id,status FROM protected_device_credentials")}
        except sqlite3.Error:
            LOG.exception("Credential status retrieval failed")
            raise

    @staticmethod
    def _context(device_id):
        return b"PocketLearning/credential/v1\0" + normalize(device_id).encode("ascii")

    def protected_record(self, token, device_id):
        """Internal maintenance reference only; never expose via a web route."""
        try:
            self.sessions.require(token, "admin")
            device_id = normalize(device_id)
            if self.protector is None:
                raise PermissionError("Protected storage unavailable")
            with closing(sqlite3.connect(self.path)) as db:
                row = db.execute("SELECT protected FROM protected_device_credentials WHERE device_id=?",
                                 (device_id,)).fetchone()
            if not row:
                raise ValueError("Protected credential not found")
            record = self.protector.unprotect(row[0], self._context(device_id))
            ident, _ = decode_credential(record)
            if ident != device_id:
                raise ValueError("Credential identity mismatch")
            return record
        except (ValueError, PermissionError, OSError, sqlite3.Error):
            LOG.warning("Protected credential retrieval rejected")
            raise

    def storage_status(self, token):
        self.sessions.require(token, "admin")
        try:
            with closing(sqlite3.connect(self.path)) as db:
                result = {row[0]: "legacy" for row in db.execute(
                    "SELECT device_id FROM device_credentials")}
                for (device_id,) in db.execute("SELECT device_id FROM protected_device_credentials"):
                    result[device_id] = "conflict" if device_id in result else "protected"
                return result
        except sqlite3.Error:
            LOG.warning("Credential storage status unavailable")
            raise
