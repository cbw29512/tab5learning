"""Portable protected teaching archive candidate; synthetic data only."""
from tools.teacher_content_codec import decode, MAX_BYTES
from tools.archive_envelope import (seal, open_sealed, new_recovery_key,
                                    NONCE_BYTES, TAG_BYTES)

MAGIC = b"PLTE\x01"
MAX_PROTECTED = MAX_BYTES + len(MAGIC) + NONCE_BYTES + TAG_BYTES


def protect(raw, recovery_key):
    decode(raw)
    return seal(raw, recovery_key, MAGIC, MAX_BYTES)


def unprotect(raw, recovery_key):
    plaintext = open_sealed(raw, recovery_key, MAGIC, MAX_BYTES)
    decode(plaintext)
    return plaintext
