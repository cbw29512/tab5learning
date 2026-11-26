"""Synthetic flash images; never read a real device during CI."""
import hashlib
import tempfile
import unittest
from pathlib import Path

from tools.flash_expansion_preflight import (
    ADDITIONS, ENTRY, FACTORY, FLASH_SIZE, MD5_MARKER, ROOT, TABLE_OFFSET,
    TABLE_SIZE, read_layout, verify_backup,
)


class FlashExpansionTests(unittest.TestCase):
    def setUp(self):
        folder = tempfile.TemporaryDirectory()
        self.addCleanup(folder.cleanup)
        self.path = Path(folder.name) / "synthetic.bin"
        self.layout = ROOT / "firmware/partitions_tab5_expansion_candidate.csv"
        table = b"".join(ENTRY.pack(0x50aa, p.kind, p.subtype, p.offset, p.size,
                                    p.name.encode(), p.flags) for p in FACTORY)
        table += MD5_MARKER + hashlib.md5(table, usedforsecurity=False).digest()
        self.raw = bytearray(b"\xff" * FLASH_SIZE)
        self.raw[TABLE_OFFSET:TABLE_OFFSET + TABLE_SIZE] = table.ljust(TABLE_SIZE, b"\xff")
        # Existing application/model/NVS data must not prevent tail expansion.
        self.raw[0x9000] = 1
        self.raw[0x10000] = 2
        self.raw[0x410000] = 3

    def check(self, **kwargs):
        self.path.write_bytes(self.raw)
        return verify_backup(self.path, hashlib.sha256(self.raw).hexdigest(), **kwargs)

    def test_preserve_existing_layout(self):
        self.assertEqual(read_layout(ROOT / "firmware/partitions_tab5_existing.csv"), FACTORY)
        self.assertEqual(read_layout(self.layout), FACTORY + ADDITIONS)
        self.assertEqual(self.check(), ADDITIONS)
        self.assertEqual(self.path.read_bytes(), self.raw)

    def test_non_erased_candidate_region(self):
        for part in ADDITIONS:
            with self.subTest(part=part.name):
                self.raw[part.offset + part.size - 1] = 0
                with self.assertRaises(ValueError):
                    self.check()
                self.raw[part.offset + part.size - 1] = 255

    def test_wrong_hash_and_length(self):
        self.path.write_bytes(self.raw)
        with self.assertRaises(ValueError):
            verify_backup(self.path, "0" * 64)
        self.raw.pop()
        with self.assertRaises(ValueError):
            self.check()

    def test_corrupt_table_and_extra_entries(self):
        for offset in (TABLE_OFFSET, TABLE_OFFSET + 4 * ENTRY.size + 16,
                       TABLE_OFFSET + 5 * ENTRY.size):
            with self.subTest(offset=offset):
                self.raw[offset] ^= 1
                with self.assertRaises(ValueError):
                    self.check()
                self.raw[offset] ^= 1

    def test_candidate_mutation_and_overlap(self):
        path = self.path.with_suffix(".csv")
        original = self.layout.read_text()
        for mutated in (original.replace("0xc10000", "0xc00000"),
                        original.replace("0x200000", "0x100000"),
                        original.replace(",encrypted", ","),
                        original.replace("0x400000", "0x300000")):
            path.write_text(mutated)
            with self.assertRaises(ValueError):
                self.check(layout_path=path)
