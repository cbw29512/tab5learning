"""Validated Tab5 read-only USB ink protocol records (no student identities)."""
from dataclasses import dataclass
import logging
from typing import Sequence

LOGGER = logging.getLogger(__name__)
MAX_POINTS = 512  # Must match firmware/include/ink_capture.h.


@dataclass(frozen=True)
class InkPoint:
    index: int
    x: int
    y: int
    stroke_start: bool


@dataclass(frozen=True)
class InkReport:
    points: tuple[InkPoint, ...]
    strokes: int
    truncated: bool


def parse_export(lines: Sequence[str]) -> InkReport:
    """Reject malformed, truncated, inconsistent, or oversized device records."""
    try:
        if len(lines) < 2:
            raise ValueError("missing header or footer")
        header = lines[0].strip().split(",")
        if len(header) != 4 or header[:2] != ["BEGIN", "INK_V1"]:
            raise ValueError("unexpected ink protocol header")
        count, expected_strokes = int(header[2]), int(header[3])
        if not (0 <= count <= MAX_POINTS) or not (0 <= expected_strokes <= count):
            raise ValueError("invalid point/stroke counts")
        if len(lines) != count + 2:
            raise ValueError("point record count mismatch")
        footer = lines[-1].strip()
        if footer not in ("END,OK", "END,TRUNCATED"):
            raise ValueError("unexpected or missing export footer")
        points = []
        for i, raw in enumerate(lines[1:-1]):
            fields = raw.strip().split(",")
            if len(fields) != 5 or fields[0] != "PT":
                raise ValueError(f"invalid point record {i}")
            idx, x, y, new_stroke = map(int, fields[1:])
            if idx != i or not (0 <= x <= 65535 and 0 <= y <= 65535):
                raise ValueError(f"invalid point position/index {i}")
            if new_stroke not in (0, 1) or (i == 0 and new_stroke != 1):
                raise ValueError(f"invalid stroke marker {i}")
            points.append(InkPoint(idx, x, y, bool(new_stroke)))
        if sum(point.stroke_start for point in points) != expected_strokes:
            raise ValueError("stroke count does not match recorded boundaries")
        return InkReport(tuple(points), expected_strokes, footer == "END,TRUNCATED")
    except (ValueError, TypeError, IndexError) as exc:
        LOGGER.error("Rejected Tab5 ink export: %s", exc)
        raise ValueError(f"invalid Tab5 ink export: {exc}") from exc
