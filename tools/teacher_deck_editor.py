"""Server-rendered visual deck editor. Uses the existing immutable deck contract."""
import logging
import sqlite3
from dataclasses import dataclass
from uuid import uuid4

from tools.flashcard_deck import ID, validate_deck
from tools.teacher_decks import StaleRevision
from tools.teacher_deck_editor_views import editor, MAX_ROWS

LOG = logging.getLogger(__name__)
FORM_FIELDS = 6 + 2 * MAX_ROWS


@dataclass
class Draft:
    deck_id: str
    title: str
    rows: list
    base_version: int = 0

    @classmethod
    def new(cls):
        return cls(uuid4().hex, "", [("", "") for _ in range(5)])

    @classmethod
    def existing(cls, deck):
        validate_deck(deck)
        if len(deck["cards"]) > MAX_ROWS:
            raise ValueError("This deck is too large for the visual editor. It has not been changed.")
        if deck["version"] >= 2147483647:
            raise ValueError("Deck version limit reached.")
        return cls(deck["deck_id"], deck["title"],
                   [(c["front"], c["back"]) for c in deck["cards"]], deck["version"])

    @classmethod
    def read(cls, data):
        try:
            count = int(data["count"])
            ident = data["deck_id"]
            if not 1 <= count <= MAX_ROWS or not ID.fullmatch(ident):
                raise ValueError("Invalid editor state")
            expected = {"csrf", "count", "deck_id", "title", "action"}
            base = 0
            if "base_version" in data:
                base = int(data["base_version"])
                if not 1 <= base < 2147483647:
                    raise ValueError("Invalid revision base")
                expected.add("base_version")
            for i in range(count):
                expected.update((f"front_{i}", f"back_{i}"))
            if set(data) != expected:
                raise ValueError("Invalid editor fields")
            rows = [(data[f"front_{i}"], data[f"back_{i}"]) for i in range(count)]
            # Keep error redisplay bounded even for hostile submissions.
            if len(data["title"]) > 120 or any(len(s) > 500 for row in rows for s in row):
                raise ValueError("Card or title exceeds the editor limit")
            return cls(ident, data["title"], rows, base)
        except (ValueError, TypeError, KeyError):
            LOG.warning("Invalid visual editor state")
            raise

    def deck(self):
        try:
            if not self.title.strip():
                raise ValueError("Enter a deck title.")
            cards = []
            for i, (front, back) in enumerate(self.rows, 1):
                if not front.strip() and not back.strip():
                    continue
                if not front.strip() or not back.strip():
                    raise ValueError(f"Complete both sides of card {i}, or remove it.")
                cards.append({"card_id": f"card-{len(cards)+1}", "front": front, "back": back})
            if not cards:
                raise ValueError("Enter at least one complete flashcard.")
            return validate_deck({"deck_id": self.deck_id, "version": self.base_version + 1,
                                  "title": self.title.strip(), "cards": cards})
        except ValueError:
            LOG.warning("Visual deck validation rejected")
            raise


def handle_editor(data, csrf, decks, token):
    """Return (draft, preview, error, saved); no hidden state or session draft."""
    draft = Draft.read(data)
    if draft.base_version:
        # Authorize every edit submission, not just the initial open.
        Draft.existing(decks.latest(token, draft.deck_id))
    action = data["action"]
    if action == "add":
        if len(draft.rows) >= MAX_ROWS:
            return draft, None, "The editor supports up to 25 cards.", False
        draft.rows.append(("", ""))
    elif action.startswith("remove:"):
        try:
            index = int(action.split(":", 1)[1])
            if not 0 <= index < len(draft.rows) or len(draft.rows) == 1:
                raise ValueError("Invalid card selection")
            draft.rows.pop(index)
        except ValueError:
            LOG.warning("Invalid card removal")
            return draft, None, "Select a valid card to remove.", False
    elif action in ("preview", "save"):
        try:
            deck = draft.deck()
        except ValueError as exc:
            return draft, None, str(exc), False
        if action == "preview":
            return draft, deck, "", False
        try:
            if draft.base_version:
                decks.revise(token, deck, draft.base_version)
            else:
                decks.save(token, deck)
        except StaleRevision as exc:
            return draft, deck, str(exc), False
        except sqlite3.IntegrityError:
            LOG.warning("Duplicate visual deck save rejected")
            return draft, deck, "This deck ID was already saved. Check the flashcard library before creating another copy.", False
        return draft, None, "", True
    else:
        raise ValueError("Unknown editor action")
    return draft, None, "", False
