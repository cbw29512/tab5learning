"""In-memory prototype credential record; never log or export the key."""
import logging
import zlib

from tools.teacher_registry import normalize

LOG = logging.getLogger(__name__)
SIZE = 105


def encode_credential(device_id, key):
    try:
        ident = normalize(device_id).encode("ascii")
        if not isinstance(key, bytes) or len(key) != 32 or not any(key):
            raise ValueError("Invalid device credential")
        body = b"PLC1" + bytes([len(ident)]) + ident.ljust(64, b"\0") + key
        return body + zlib.crc32(body).to_bytes(4, "little")
    except (TypeError, ValueError):
        LOG.warning("Invalid credential record input")
        raise


def decode_credential(record):
    try:
        if not isinstance(record, bytes) or len(record) != SIZE or record[:4] != b"PLC1":
            raise ValueError("Invalid credential record")
        length = record[4]
        if not 1 <= length <= 64 or any(record[5+length:69]):
            raise ValueError("Invalid credential identifier field")
        if zlib.crc32(record[:101]) != int.from_bytes(record[101:], "little"):
            raise ValueError("Credential record corruption")
        ident = record[5:5+length].decode("ascii")
        key = record[69:101]
        if normalize(ident) != ident or not any(key):
            raise ValueError("Invalid credential record fields")
        return ident, key
    except (TypeError, ValueError):
        LOG.warning("Credential record rejected")
        raise
