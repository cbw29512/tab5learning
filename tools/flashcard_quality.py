"""Conservative bulk-deck quality checks: warnings are not grading decisions."""
import csv
import io
import logging
from dataclasses import dataclass

from tools.flashcard_deck import validate_deck, MAX_BYTES

LOG = logging.getLogger(__name__)


@dataclass(frozen=True)
class ReviewFindings:
    # One-based row numbers correspond to the visible preview, not CSV lines.
    duplicates: tuple
    conflicts: tuple


def inspect_cards(deck):
    """Find identical pairs and repeated questions with different answers.

    No case folding or punctuation stripping: e.g. m and M can mean
    different things. Different answers are warnings, never auto-deleted.
    """
    try:
        validate_deck(deck)
        pairs, questions = {}, {}
        duplicates, conflicts = [], []
        for index, card in enumerate(deck["cards"], 1):
            pair = (card["front"], card["back"])
            if pair in pairs:
                duplicates.append((pairs[pair], index))
            else:
                pairs[pair] = index
            first = questions.setdefault(card["front"], (index, card["back"]))
            if first[1] != card["back"]:
                conflicts.append((first[0], index))
        return ReviewFindings(tuple(duplicates), tuple(conflicts))
    except (ValueError, TypeError):
        LOG.warning("Flashcard quality check rejected invalid deck")
        raise


def without_identical_cards(deck):
    """Explicit teacher action only; keep first occurrence and original order."""
    try:
        findings = inspect_cards(deck)
        omit = {later for _, later in findings.duplicates}
        stream = io.StringIO(newline="")
        writer = csv.writer(stream, lineterminator="\n", quoting=csv.QUOTE_ALL)
        writer.writerow(("front", "back"))
        for index, card in enumerate(deck["cards"], 1):
            if index not in omit:
                writer.writerow((card["front"], card["back"]))
        text = stream.getvalue()
        if len(text.encode("utf-8")) > MAX_BYTES:
            raise ValueError("Reformatted CSV exceeds import limit")
        return text, len(omit)
    except (ValueError, TypeError, UnicodeError, csv.Error):
        LOG.warning("Explicit duplicate cleanup rejected")
        raise
