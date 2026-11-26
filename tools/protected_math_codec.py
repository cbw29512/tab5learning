"""Bounded identity-bound mathematics recovery envelope, synthetic only."""
from dataclasses import dataclass

from tools.math_backup_codec import decode, MAX_BYTES
from tools.teacher_registry import normalize
from tools.archive_envelope import seal, open_sealed, NONCE_BYTES, TAG_BYTES

MAGIC = b"PLSE\x01"
MAX_PAYLOAD = MAX_BYTES + 130
MAX_PROTECTED = MAX_PAYLOAD + len(MAGIC) + NONCE_BYTES + TAG_BYTES


@dataclass(frozen=True)
class StudentMathSnapshot:
    student: str
    device: str
    history: bytes


def encode(snapshot):
    student = normalize(snapshot.student).encode("ascii")
    device = normalize(snapshot.device).encode("ascii")
    decode(snapshot.history)
    return bytes([len(student)]) + student + bytes([len(device)]) + device + snapshot.history


def parse(raw):
    if type(raw) is not bytes or not 4 <= len(raw) <= MAX_PAYLOAD:
        raise ValueError("Invalid student archive size")
    n = raw[0]
    if not 1 <= n <= 64 or len(raw) <= n+1:
        raise ValueError("Invalid student identity")
    m = raw[n+1]
    if not 1 <= m <= 64 or len(raw) <= n+m+2:
        raise ValueError("Invalid device identity")
    student = raw[1:n+1].decode("ascii")
    device = raw[n+2:n+m+2].decode("ascii")
    if normalize(student) != student or normalize(device) != device:
        raise ValueError("Noncanonical archive identity")
    history = raw[n+m+2:]
    decode(history)
    return StudentMathSnapshot(student, device, history)


def protect(snapshot, key):
    return seal(encode(snapshot), key, MAGIC, MAX_PAYLOAD)


def unprotect(raw, key):
    return parse(open_sealed(raw, key, MAGIC, MAX_PAYLOAD))
