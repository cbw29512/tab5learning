"""Shared bounded authenticated archive envelope; callers validate payloads."""
import logging
import re
import secrets

LOG = logging.getLogger(__name__)
NONCE_BYTES = 12
TAG_BYTES = 16


def new_recovery_key():
    return secrets.token_hex(32)


def _key(value):
    if not isinstance(value, str) or not re.fullmatch(r"[0-9a-f]{64}", value):
        raise ValueError("Invalid recovery key format")
    return bytes.fromhex(value)


def seal(raw, recovery_key, magic, maximum):
    try:
        from cryptography.hazmat.primitives.ciphers.aead import AESGCM
        if type(raw) is not bytes or not 0 < len(raw) <= maximum:
            raise ValueError("Invalid archive payload size")
        nonce = secrets.token_bytes(NONCE_BYTES)
        header = magic + nonce
        return header + AESGCM(_key(recovery_key)).encrypt(nonce, raw, header)
    except (ImportError, ValueError, TypeError, OSError):
        LOG.warning("Archive sealing rejected")
        raise


def open_sealed(raw, recovery_key, magic, maximum):
    try:
        from cryptography.exceptions import InvalidTag
        from cryptography.hazmat.primitives.ciphers.aead import AESGCM
        overhead = len(magic) + NONCE_BYTES + TAG_BYTES
        if type(raw) is not bytes or not overhead < len(raw) <= maximum + overhead or not raw.startswith(magic):
            raise ValueError("Invalid protected archive framing")
        end = len(magic) + NONCE_BYTES
        try:
            return AESGCM(_key(recovery_key)).decrypt(raw[len(magic):end], raw[end:], raw[:end])
        except InvalidTag:
            raise ValueError("Wrong recovery key or damaged archive") from None
    except (ImportError, ValueError, TypeError, OSError):
        LOG.warning("Protected archive opening rejected")
        raise
