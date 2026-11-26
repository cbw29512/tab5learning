"""Local test-only migration; never exports or replaces a credential."""
import hmac
import logging
import sqlite3
from contextlib import closing

from tools.device_credential_record import encode_credential
from tools.teacher_registry import normalize

LOG = logging.getLogger(__name__)


def migrate_legacy(service, token, device_id, *, confirmed=False):
    """Preserve the original key and metadata; refuse ambiguous records."""
    try:
        service.sessions.require(token, "admin")
        device_id = normalize(device_id)
        if confirmed is not True or service.protector is None:
            raise PermissionError("Explicit confirmation and protection required")
        if not any(row["device_id"] == device_id
                   for row in service.registry.list_assignments(token)):
            raise ValueError("Device requires a planned assignment")
        with closing(sqlite3.connect(service.path)) as db, db:
            db.execute("BEGIN IMMEDIATE")
            legacy = db.execute(
                "SELECT secret,actor,created_at,status FROM device_credentials WHERE device_id=?",
                (device_id,)).fetchone()
            protected = db.execute(
                "SELECT protected FROM protected_device_credentials WHERE device_id=?",
                (device_id,)).fetchone()
            if legacy and protected:
                raise ValueError("Conflicting credential records")
            if protected:
                return "ALREADY_PROTECTED"
            if not legacy or legacy[3] != "KEY_PENDING":
                raise ValueError("No pending legacy credential")
            record = encode_credential(device_id, legacy[0])
            context = service._context(device_id)
            blob = service.protector.protect(record, context)
            if not isinstance(blob, bytes) or not 0 < len(blob) <= 4096:
                raise ValueError("Invalid protected credential")
            verified = service.protector.unprotect(blob, context)
            if not isinstance(verified, bytes) or not hmac.compare_digest(record, verified):
                raise ValueError("Protected credential readback mismatch")
            db.execute("INSERT INTO protected_device_credentials VALUES (?,?,?,?,?)",
                       (device_id, blob, *legacy[1:]))
            deleted = db.execute("DELETE FROM device_credentials WHERE device_id=?",
                                 (device_id,))
            if deleted.rowcount != 1:
                raise ValueError("Legacy credential removal failed")
        return "MIGRATED"
    except Exception:
        LOG.warning("Local test credential migration rejected")
        raise
