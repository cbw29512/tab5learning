"""Local account storage and password verification for Teacher Studio."""
import hashlib
import hmac
import logging
import re
import secrets
import sqlite3
import time
from contextlib import closing

LOG = logging.getLogger(__name__)
ROLES = {"admin", "teacher"}


def username_key(value):
    if not isinstance(value, str) or not re.fullmatch(r"[a-zA-Z0-9_.-]{3,64}", value):
        raise ValueError("Invalid username")
    return value.lower()


def derive(password, salt):
    # Bound input and use a memory-hard KDF; parameters are fixed for schema v1.
    if not isinstance(password, str) or not 1 <= len(password) <= 256:
        raise ValueError("Invalid password")
    return hashlib.scrypt(password.encode("utf-8"), salt=salt,
                          n=2**15, r=8, p=3, maxmem=64*1024*1024)


class AccountStore:
    def __init__(self, path):
        self.path = path
        try:
            with closing(self.connect()) as db, db:
                db.execute("""CREATE TABLE IF NOT EXISTS accounts (
                    username TEXT PRIMARY KEY, role TEXT NOT NULL,
                    salt BLOB NOT NULL, digest BLOB NOT NULL,
                    enabled INTEGER NOT NULL DEFAULT 1,
                    failures INTEGER NOT NULL DEFAULT 0,
                    locked_until REAL NOT NULL DEFAULT 0)""")
        except sqlite3.Error:
            LOG.exception("Account database initialization failed")
            raise

    def connect(self):
        return sqlite3.connect(self.path, timeout=10)

    def create(self, username, password, role, *, bootstrap=False, actor=None):
        """Internal storage operation; actor comes from validated local session."""
        try:
            key = username_key(username)
            if role not in ROLES or not isinstance(password, str) or len(password) < 15:
                raise ValueError("Invalid role or password shorter than 15 characters")
            salt = secrets.token_bytes(32)
            digest = derive(password, salt)
            with closing(self.connect()) as db, db:
                db.execute("BEGIN IMMEDIATE")
                if bootstrap:
                    if role != "admin" or db.execute("SELECT 1 FROM accounts LIMIT 1").fetchone():
                        raise PermissionError("Bootstrap unavailable")
                elif not self.is_admin(db, actor):
                    raise PermissionError("Administrator required")
                db.execute("INSERT INTO accounts(username,role,salt,digest) VALUES (?,?,?,?)",
                           (key, role, salt, digest))
        except (sqlite3.Error, ValueError, PermissionError):
            LOG.warning("Account creation rejected")
            raise

    @staticmethod
    def is_admin(db, actor):
        return db.execute("SELECT 1 FROM accounts WHERE username=? AND role='admin' AND enabled=1",
                          (actor,)).fetchone() is not None

    def role(self, key):
        with closing(self.connect()) as db:
            row = db.execute("SELECT role FROM accounts WHERE username=? AND enabled=1", (key,)).fetchone()
            return row[0] if row else None

    def verify(self, username, password):
        try:
            key = username_key(username)
            with closing(self.connect()) as db, db:
                db.execute("BEGIN IMMEDIATE")
                row = db.execute("SELECT salt,digest,enabled,failures,locked_until FROM accounts WHERE username=?", (key,)).fetchone()
                now = time.time()
                # Perform comparable password work even for nonexistent accounts.
                candidate = derive(password, row[0] if row else bytes(32))
                if not row or not row[2] or row[4] > now:
                    return None
                if hmac.compare_digest(candidate, row[1]):
                    db.execute("UPDATE accounts SET failures=0,locked_until=0 WHERE username=?", (key,))
                    return key
                failures = row[3]+1
                db.execute("UPDATE accounts SET failures=?,locked_until=? WHERE username=?",
                           (0 if failures >= 5 else failures, now+300 if failures >= 5 else 0, key))
                return None
        except ValueError:
            LOG.warning("Invalid login input")
            return None
        except sqlite3.Error:
            LOG.exception("Account verification failed")
            raise

    def disable(self, actor, target):
        try:
            with closing(self.connect()) as db, db:
                db.execute("BEGIN IMMEDIATE")
                if not self.is_admin(db, actor):
                    raise PermissionError("Administrator required")
                key = username_key(target)
                if key == actor:
                    raise ValueError("Cannot disable own account")
                if db.execute("UPDATE accounts SET enabled=0 WHERE username=?", (key,)).rowcount != 1:
                    raise ValueError("Unknown account")
        except (sqlite3.Error, ValueError, PermissionError):
            LOG.warning("Account disable rejected")
            raise
