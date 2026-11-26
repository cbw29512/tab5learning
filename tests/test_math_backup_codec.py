"""Synthetic fixtures matching the embedded V1 wire layout."""
import hashlib
import struct
import unittest
import zlib

from tools.math_backup_codec import (
    MAGIC, MAX_RECORDS, Checkpoint, MathBackup, checkpoint, decode, encode, validate,
)


def crc(body):
    return body + struct.pack("<I", zlib.crc32(body))


def cp(index=0, independent=0, retried=False, finished=False, attempts=0):
    return crc(b"PL\x01" + bytes((index, independent, retried, finished, 0))
               + struct.pack("<I", attempts))


def attempt(question, choice, result, after, points=()):
    ink = b"".join(struct.pack("<HHB", *p) for p in points)
    return crc(b"PA\x01" + bytes((question, choice, result))
               + struct.pack("<H", len(points)) + after + ink)


def rehash(body):
    return body + hashlib.sha256(body).digest()


class MathBackupTests(unittest.TestCase):
    def setUp(self):
        self.base = cp()
        self.retry = cp(retried=True, attempts=1)
        self.final = cp(index=1, attempts=2)
        self.first = attempt(0, 0, 1, self.retry, ((0, 0, 1), (591, 243, 0)))
        self.second = attempt(0, 1, 2, self.final)
        self.backup = MathBackup(self.base, self.final, (self.first, self.second))

    def test_round_trip_preserves_ink_and_progress(self):
        raw = encode(self.backup)
        self.assertEqual(decode(raw), self.backup)
        self.assertEqual(encode(decode(raw)), raw)
        self.assertEqual(validate(self.backup), Checkpoint(1, 0, False, False, 2))

    def test_empty_and_legacy_summary(self):
        self.assertEqual(decode(encode(MathBackup(self.base, self.base, ()))), 
                         MathBackup(self.base, self.base, ()))
        legacy = MathBackup(self.retry, self.final, (self.second,))
        self.assertEqual(decode(encode(legacy)), legacy)
        self.assertEqual(checkpoint(legacy.base).attempts, 1)

    def test_finish_and_independent_scoring(self):
        records = tuple(attempt(i, choice, 3 if i == 4 else 2,
                                cp(i + 1, i + 1, False, i == 4, i + 1))
                        for i, choice in enumerate((1, 0, 2, 3, 0)))
        final = cp(5, 5, False, True, 5)
        self.assertEqual(validate(MathBackup(self.base, final, records)).independent, 5)

    def test_corrupt_and_reordered_records(self):
        for records in ((self.second, self.first), (self.first, self.first),
                        (self.first,), (self.first[:-1] + bytes([self.first[-1] ^ 1]),
                                        self.second)):
            with self.subTest(records=len(records)), self.assertRaises(ValueError):
                validate(MathBackup(self.base, self.final, records))

    def test_forged_valid_crc_rejected(self):
        wrong_score = attempt(0, 1, 2, cp(index=1, independent=1, attempts=2))
        wrong_ink = attempt(0, 0, 1, self.retry, ((592, 0, 1),))
        wrong_boundary = attempt(0, 0, 1, self.retry, ((0, 0, 0),))
        for records in ((self.first, wrong_score), (wrong_ink, self.second),
                        (wrong_boundary, self.second)):
            with self.assertRaises(ValueError):
                validate(MathBackup(self.base, self.final, records))

    def test_container_bounds_and_tamper(self):
        raw = encode(self.backup)
        body = raw[:-32]
        count_offset = len(MAGIC) + 32
        first_size = len(MAGIC) + 36
        cases = (raw[:-1], raw + b"x", raw[:-1] + bytes([raw[-1] ^ 1]),
                 rehash(body + b"x"),
                 rehash(body[:count_offset] + struct.pack("<I", MAX_RECORDS + 1)
                        + body[count_offset + 4:]),
                 rehash(body[:first_size] + b"\xff\xff" + body[first_size + 2:]),
                 rehash(body[:-1]))
        for candidate in cases:
            with self.subTest(size=len(candidate)), self.assertRaises(ValueError):
                decode(candidate)

    def test_checkpoint_validation(self):
        for raw in (cp(index=6, attempts=6), cp(index=1, independent=2, attempts=1),
                    cp(index=1, attempts=0), cp(finished=True), b"invalid"):
            with self.assertRaises(ValueError):
                checkpoint(raw)

    def test_more_than_32_records(self):
        records = tuple(attempt(0, 0, 1, cp(retried=True, attempts=i))
                        for i in range(1, 101))
        snapshot = MathBackup(self.base, cp(retried=True, attempts=100), records)
        self.assertEqual(decode(encode(snapshot)), snapshot)


if __name__ == "__main__":
    unittest.main()
