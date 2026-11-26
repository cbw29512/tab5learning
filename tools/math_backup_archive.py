"""Durable synthetic math archive. Not an authenticated device import route."""
import hashlib
import logging
import sqlite3
from contextlib import closing
from datetime import datetime, timezone

from tools.math_backup_codec import MAX_RECORDS, MathBackup, decode, encode
from tools.teacher_registry import normalize

LOG = logging.getLogger(__name__)


class PreviewMathArchive:
    """Store original attempts once; immutable receipts identify verified prefixes."""

    def __init__(self, path, sessions, registry, *, test_mode=False):
        if test_mode is not True:
            raise PermissionError("Synthetic mathematics archive requires test mode")
        self.path, self.sessions, self.registry = path, sessions, registry
        try:
            with closing(self._connect()) as db, db:
                db.executescript("""
                    CREATE TABLE IF NOT EXISTS preview_math_base (
                        device TEXT PRIMARY KEY, student TEXT NOT NULL UNIQUE,
                        base BLOB NOT NULL CHECK(length(base)=16));
                    CREATE TABLE IF NOT EXISTS preview_math_attempt (
                        device TEXT NOT NULL REFERENCES preview_math_base(device),
                        slot INTEGER NOT NULL CHECK(slot BETWEEN 1 AND 4096),
                        body BLOB NOT NULL CHECK(length(body) BETWEEN 28 AND 2588),
                        PRIMARY KEY(device,slot));
                    CREATE TABLE IF NOT EXISTS preview_math_receipt (
                        device TEXT NOT NULL REFERENCES preview_math_base(device),
                        count INTEGER NOT NULL CHECK(count BETWEEN 0 AND 4096),
                        final BLOB NOT NULL CHECK(length(final)=16),
                        digest TEXT NOT NULL CHECK(length(digest)=64),
                        actor TEXT NOT NULL, created_at TEXT NOT NULL,
                        PRIMARY KEY(device,count), UNIQUE(device,digest));
                """)
        except sqlite3.Error:
            LOG.exception("Preview archive initialization failed")
            raise

    def _connect(self):
        db = sqlite3.connect(self.path, timeout=5)
        try:
            db.execute("PRAGMA foreign_keys=ON")
            db.execute("PRAGMA synchronous=FULL")
            return db
        except sqlite3.Error:
            db.close()
            raise

    def _scope(self, token, device, student):
        actor = self.sessions.require(token)
        device, student = normalize(device), normalize(student)
        if not any(row["device_id"] == device and row["student_id"] == student
                   and row["status"] == "PLANNED"
                   for row in self.registry.list_assignments(token)):
            raise PermissionError("No accessible planned assignment")
        return actor, device, student

    @staticmethod
    def _read(db, device, base, receipt):
        count, final, digest = receipt
        if type(count) is not int or not 0 <= count <= MAX_RECORDS:
            raise ValueError("Invalid archived record count")
        rows = db.execute(
            "SELECT slot,body FROM preview_math_attempt WHERE device=? AND slot<=? ORDER BY slot",
            (device, count)).fetchall()
        if len(rows) != count or any(slot != i for i, (slot, _) in enumerate(rows, 1)):
            raise ValueError("Incomplete archived history")
        raw = encode(MathBackup(base, final, tuple(body for _, body in rows)))
        if hashlib.sha256(raw).hexdigest() != digest:
            raise ValueError("Archived digest mismatch")
        return raw

    def save(self, token, device, student, raw):
        """Commit only a new prefix extension or an exact retry; never erase."""
        try:
            actor, device, student = self._scope(token, device, student)
            candidate = decode(raw)
            count = len(candidate.records)
            digest = hashlib.sha256(raw).hexdigest()
            with closing(self._connect()) as db, db:
                db.execute("BEGIN IMMEDIATE")
                identity = db.execute("SELECT student,base FROM preview_math_base WHERE device=?",
                                      (device,)).fetchone()
                if identity is None:
                    db.execute("INSERT INTO preview_math_base VALUES (?,?,?)",
                               (device, student, candidate.base))
                elif identity != (student, candidate.base):
                    raise ValueError("Archive identity or legacy base conflict")
                head = db.execute(
                    "SELECT count,final,digest FROM preview_math_receipt WHERE device=? ORDER BY count DESC LIMIT 1",
                    (device,)).fetchone()
                prior_count = head[0] if head else 0
                if db.execute("SELECT COUNT(*) FROM preview_math_attempt WHERE device=?",
                              (device,)).fetchone()[0] != prior_count:
                    raise ValueError("Unexpected archived attempt count")
                if head:
                    previous = decode(self._read(db, device, candidate.base, head))
                    if count <= prior_count:
                        receipt = db.execute(
                            "SELECT count,final,digest FROM preview_math_receipt WHERE device=? AND count=?",
                            (device, count)).fetchone()
                        if receipt and self._read(db, device, candidate.base, receipt) == raw:
                            return digest
                        raise ValueError("Stale or conflicting archive")
                    if candidate.records[:prior_count] != previous.records:
                        raise ValueError("History diverges from archived evidence")
                elif identity is not None:
                    raise ValueError("Missing archive receipt")
                db.executemany("INSERT INTO preview_math_attempt VALUES (?,?,?)",
                               ((device, slot, body) for slot, body in
                                enumerate(candidate.records[prior_count:], prior_count + 1)))
                db.execute("INSERT INTO preview_math_receipt VALUES (?,?,?,?,?,?)",
                           (device, count, candidate.final, digest, actor,
                            datetime.now(timezone.utc).isoformat()))
                if self._read(db, device, candidate.base, (count, candidate.final, digest)) != raw:
                    raise ValueError("Archive readback mismatch")
            return digest
        except (ValueError, TypeError, PermissionError, sqlite3.Error):
            LOG.warning("Synthetic archive save rejected or failed")
            raise

    def preview_restore(self, token, device, student, raw):
        """Read-only admissibility check; save revalidates under write lock."""
        try:
            _, device, student = self._scope(token, device, student)
            candidate = decode(raw)
            with closing(self._connect()) as db, db:
                db.execute("BEGIN")
                identity = db.execute("SELECT student,base FROM preview_math_base WHERE device=?",
                                      (device,)).fetchone()
                if identity is None:
                    if db.execute("SELECT 1 FROM preview_math_base WHERE student=?", (student,)).fetchone():
                        raise ValueError("Student already has a different archive device")
                    return len(candidate.records), False
                if identity != (student, candidate.base):
                    raise ValueError("Archive identity or base conflict")
                head = db.execute("SELECT count,final,digest FROM preview_math_receipt WHERE device=? "
                                  "ORDER BY count DESC LIMIT 1", (device,)).fetchone()
                if not head or db.execute("SELECT count(*) FROM preview_math_attempt WHERE device=?",
                                          (device,)).fetchone()[0] != head[0]:
                    raise ValueError("Incomplete archive")
                previous = decode(self._read(db, device, candidate.base, head))
                count = len(candidate.records)
                if count <= head[0]:
                    receipt = db.execute("SELECT count,final,digest FROM preview_math_receipt "
                                         "WHERE device=? AND count=?", (device,count)).fetchone()
                    if receipt and self._read(db, device, candidate.base, receipt) == raw:
                        return 0, True
                    raise ValueError("Stale or conflicting archive")
                if candidate.records[:head[0]] != previous.records:
                    raise ValueError("History diverges")
                return count-head[0], False
        except (ValueError, TypeError, PermissionError, sqlite3.Error):
            LOG.warning("Synthetic archive restore preview rejected")
            raise

    def load(self, token, device, student, *, digest=None):
        """Return a validated snapshot; default to latest, never restore to device."""
        try:
            _, device, student = self._scope(token, device, student)
            with closing(self._connect()) as db, db:
                db.execute("BEGIN")
                row = db.execute("SELECT student,base FROM preview_math_base WHERE device=?",
                                 (device,)).fetchone()
                if not row or row[0] != student:
                    raise ValueError("Archive not found")
                receipt = db.execute(
                    "SELECT count,final,digest FROM preview_math_receipt WHERE device=? "
                    "AND (? IS NULL OR digest=?) ORDER BY count DESC LIMIT 1",
                    (device, digest, digest)).fetchone()
                if not receipt:
                    raise ValueError("Archive receipt not found")
                if digest is None and db.execute("SELECT count(*) FROM preview_math_attempt WHERE device=?", (device,)).fetchone()[0] != receipt[0]:
                    raise ValueError("Unexpected archived attempt count")
                return self._read(db, device, row[1], receipt)
        except (ValueError, TypeError, PermissionError, sqlite3.Error):
            LOG.warning("Synthetic archive load rejected or failed")
            raise
