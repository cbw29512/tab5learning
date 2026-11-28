"""Independent host decoder for synthetic V1 mathematics evidence, not authorization."""
import hashlib
import hmac
import logging
import struct
import zlib
from dataclasses import dataclass

LOG = logging.getLogger(__name__)
MAGIC = b"PLMB\x01"
MAX_RECORDS = 4096  # Host preview format bound, not device retention policy.
MAX_RECORD_SIZE = 28 + 5 * 512
MAX_BYTES = len(MAGIC) + 36 + MAX_RECORDS * (2 + MAX_RECORD_SIZE) + 32
# V1 synthetic bank, exact correct choice indices from math_practice.h.
CORRECT_CHOICES = (1, 0, 2, 3, 0)


@dataclass(frozen=True)
class Checkpoint:
    index: int
    independent: int
    retried: bool
    finished: bool
    attempts: int


@dataclass(frozen=True)
class MathBackup:
    base: bytes
    final: bytes
    records: tuple[bytes, ...]


def _crc_ok(raw):
    return zlib.crc32(raw[:-4]) == int.from_bytes(raw[-4:], "little")


def checkpoint(raw):
    """Validate the exact firmware V1 checkpoint layout and invariants."""
    if (type(raw) is not bytes or len(raw) != 16 or raw[:3] != b"PL\x01"
            or raw[5] > 1 or raw[6] > 1 or raw[7] or not _crc_ok(raw)):
        raise ValueError("Invalid mathematics checkpoint")
    s = Checkpoint(raw[3], raw[4], bool(raw[5]), bool(raw[6]),
                   int.from_bytes(raw[8:12], "little"))
    if (not 0 <= s.independent <= s.index <= len(CORRECT_CHOICES)
            or s.finished != (s.index == len(CORRECT_CHOICES))
            or (s.finished and s.retried) or s.attempts < s.index
            or s.attempts - s.index < s.index - s.independent + s.retried):
        raise ValueError("Invalid checkpoint state")
    return s


def replay(raw, before):
    """Derive scoring independently; preserve all original bytes unchanged."""
    if (type(raw) is not bytes or not 28 <= len(raw) <= MAX_RECORD_SIZE
            or raw[:3] != b"PA\x01" or raw[3] != before.index or raw[4] >= 4
            or before.finished or before.attempts == 0xFFFFFFFF or not _crc_ok(raw)):
        raise ValueError("Invalid attempt")
    count = int.from_bytes(raw[6:8], "little")
    if count > 512 or len(raw) != 28 + 5 * count:
        raise ValueError("Invalid ink length")
    for i in range(count):
        x, y, start = struct.unpack_from("<HHB", raw, 24 + 5 * i)
        if x >= 592 or y >= 244 or start > 1 or (i == 0 and not start):
            raise ValueError("Invalid ink point")
    correct = raw[4] == CORRECT_CHOICES[before.index]
    index = before.index + int(correct)
    finished = index == len(CORRECT_CHOICES)
    expected = Checkpoint(index, before.independent + int(correct and not before.retried),
                          not correct, finished, before.attempts + 1)
    result = 3 if finished else 2 if correct else 1
    if raw[5] != result or checkpoint(raw[8:24]) != expected:
        raise ValueError("Attempt contradicts scoring or sequence")
    return expected


def validate(backup):
    """Reject missing, duplicated, reordered, corrupted or inconsistent evidence."""
    try:
        if (not isinstance(backup, MathBackup) or type(backup.records) is not tuple
                or len(backup.records) > MAX_RECORDS):
            raise ValueError("Invalid snapshot")
        state, final = checkpoint(backup.base), checkpoint(backup.final)
        if final.attempts - state.attempts != len(backup.records):
            raise ValueError("Snapshot count mismatch")
        for raw in backup.records:
            state = replay(raw, state)
        if state != final:
            raise ValueError("Snapshot terminal state mismatch")
        return final
    except (ValueError, TypeError, struct.error):
        LOG.warning("Mathematics snapshot rejected")
        raise


def encode(backup):
    """Local candidate container; SHA-256 detects damage, not hostile changes."""
    try:
        validate(backup)
        body = bytearray(MAGIC + backup.base + backup.final
                         + struct.pack("<I", len(backup.records)))
        for raw in backup.records:
            body.extend(struct.pack("<H", len(raw)))
            body.extend(raw)
        return bytes(body) + hashlib.sha256(body).digest()
    except (ValueError, TypeError, struct.error):
        LOG.warning("Mathematics backup encoding rejected")
        raise


def decode(raw):
    """Validate bounds, exact framing, digest and independent replay before use."""
    try:
        if (type(raw) is not bytes or not len(MAGIC) + 36 + 32 <= len(raw) <= MAX_BYTES
                or not raw.startswith(MAGIC)):
            raise ValueError("Invalid backup header or size")
        body, digest = raw[:-32], raw[-32:]
        if not hmac.compare_digest(hashlib.sha256(body).digest(), digest):
            raise ValueError("Backup digest mismatch")
        pos = len(MAGIC)
        base, final = body[pos:pos + 16], body[pos + 16:pos + 32]
        count = struct.unpack_from("<I", body, pos + 32)[0]
        pos += 36
        if count > MAX_RECORDS or count > (len(body) - pos) // 30:
            raise ValueError("Invalid backup record count")
        records = []
        for _ in range(count):
            if len(body) - pos < 2:
                raise ValueError("Truncated record length")
            n = struct.unpack_from("<H", body, pos)[0]
            pos += 2
            if not 28 <= n <= MAX_RECORD_SIZE or n > len(body) - pos:
                raise ValueError("Invalid record size")
            records.append(body[pos:pos + n])
            pos += n
        if pos != len(body):
            raise ValueError("Unexpected trailing bytes")
        backup = MathBackup(base, final, tuple(records))
        validate(backup)
        return backup
    except (ValueError, TypeError, struct.error):
        LOG.warning("Mathematics backup decoding rejected")
        raise
