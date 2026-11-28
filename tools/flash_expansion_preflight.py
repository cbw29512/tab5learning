"""Read-only preflight for a synthetic Tab5 flash expansion candidate.

No device access, image generation, erasure, partition writes or provisioning.
"""
import argparse
import csv
import hashlib
import hmac
import logging
import re
import struct
from dataclasses import dataclass
from pathlib import Path

LOG = logging.getLogger(__name__)
FLASH_SIZE = 16 * 1024 * 1024
TABLE_OFFSET, TABLE_SIZE = 0x8000, 0xC00
ENTRY = struct.Struct("<HBBII16sI")
MD5_MARKER = b"\xeb\xeb" + b"\xff" * 14
SECTOR = 4096
ROOT = Path(__file__).resolve().parents[1]


@dataclass(frozen=True)
class Partition:
    name: str
    kind: int
    subtype: int
    offset: int
    size: int
    flags: int = 0


FACTORY = (
    Partition("nvs", 1, 2, 0x9000, 0x6000),
    Partition("phy_init", 1, 1, 0xf000, 0x1000),
    Partition("factory", 0, 0, 0x10000, 0x400000),
    Partition("model", 1, 0x82, 0x410000, 0x800000),
)
ADDITIONS = (
    Partition("pllearning", 1, 2, 0xc10000, 0x200000),
    Partition("plsecure", 1, 2, 0xe10000, 0x3000),
    Partition("plseckeys", 1, 4, 0xe13000, 0x1000, 1),
)
TYPES = {("data", "nvs"): (1, 2), ("data", "phy"): (1, 1),
         ("app", "factory"): (0, 0), ("data", "spiffs"): (1, 0x82),
         ("data", "nvs_keys"): (1, 4)}


def read_layout(path):
    """Accept only known ESP-IDF CSV fields and explicit aligned regions."""
    try:
        with Path(path).open(encoding="utf-8", newline="") as stream:
            rows = csv.reader(line for line in stream
                              if line.strip() and not line.lstrip().startswith("#"))
            result = []
            for row in rows:
                name, kind, subtype, offset, size, flags = map(str.strip, row)
                type_id, sub_id = TYPES[(kind, subtype)]
                if flags not in ("", "encrypted") or not re.fullmatch(r"[a-z0-9_]{1,15}", name):
                    raise ValueError("Unsupported partition fields")
                result.append(Partition(name, type_id, sub_id, int(offset, 0),
                                        int(size, 0), int(flags == "encrypted")))
        end = TABLE_OFFSET + SECTOR
        names = set()
        for part in result:
            if (part.name in names or part.offset < end or part.offset % SECTOR
                    or (part.kind == 0 and part.offset % 65536)
                    or part.size <= 0 or part.size % SECTOR
                    or part.offset + part.size > FLASH_SIZE):
                raise ValueError("Overlapping, duplicate or invalid partition")
            names.add(part.name)
            end = part.offset + part.size
        return tuple(result)
    except (OSError, ValueError, KeyError, csv.Error):
        LOG.warning("Partition layout rejected")
        raise


def verify_factory_table(table):
    """Exact original entries, required ESP-IDF MD5 marker, erased padding."""
    if len(table) != TABLE_SIZE:
        raise ValueError("Invalid partition table size")
    for i, part in enumerate(FACTORY):
        offset = i * ENTRY.size
        expected = ENTRY.pack(0x50aa, part.kind, part.subtype, part.offset,
                              part.size, part.name.encode("ascii"), part.flags)
        if table[offset:offset + ENTRY.size] != expected:
            raise ValueError("Backup does not match the known factory layout")
    end = len(FACTORY) * ENTRY.size
    expected_md5 = MD5_MARKER + hashlib.md5(table[:end], usedforsecurity=False).digest()
    if table[end:end + ENTRY.size] != expected_md5:
        raise ValueError("Missing or invalid partition table checksum")
    if table[end + ENTRY.size:] != b"\xff" * (TABLE_SIZE - end - ENTRY.size):
        raise ValueError("Unexpected partition table content")


def verify_backup(path, expected_sha256, *, layout_path=None):
    """Return only candidate region metadata after read-only preflight."""
    try:
        if not isinstance(expected_sha256, str) or not re.fullmatch(r"[0-9a-fA-F]{64}", expected_sha256):
            raise ValueError("Expected SHA-256 must contain 64 hex characters")
        candidate = read_layout(layout_path or ROOT / "firmware/partitions_tab5_expansion_candidate.csv")
        if candidate != FACTORY + ADDITIONS:
            raise ValueError("Candidate changes the reviewed partition layout")
        with Path(path).open("rb") as stream:
            raw = stream.read(FLASH_SIZE + 1)
        if len(raw) != FLASH_SIZE:
            raise ValueError("Expected an exact 16 MiB flash backup")
        if not hmac.compare_digest(hashlib.sha256(raw).hexdigest(), expected_sha256.lower()):
            raise ValueError("Backup hash mismatch")
        verify_factory_table(raw[TABLE_OFFSET:TABLE_OFFSET + TABLE_SIZE])
        # Require the entire proposed new region to be erased; unallocated
        # addresses are not automatically safe. Never print backup contents.
        for part in ADDITIONS:
            if raw[part.offset:part.offset + part.size] != b"\xff" * part.size:
                raise ValueError("Candidate region contains existing data: " + part.name)
        return ADDITIONS
    except (OSError, ValueError, TypeError, KeyError):
        LOG.warning("Read-only flash expansion preflight failed")
        raise


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("backup", type=Path)
    parser.add_argument("--sha256", required=True)
    args = parser.parse_args()
    try:
        for part in verify_backup(args.backup, args.sha256):
            print(f"{part.name}: offset=0x{part.offset:x}, size={part.size}")
        print("BACKUP_PREFLIGHT_PASS — no device or flash writes; not approval to install")
        return 0
    except (OSError, ValueError, TypeError, KeyError):
        print("BACKUP_PREFLIGHT_FAIL — no changes made")
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
