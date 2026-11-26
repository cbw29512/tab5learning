"""Server-rendered visual deck editor. Uses the existing immutable deck contract."""
import logging
import sqlite3
from dataclasses import dataclass
from html import escape
from uuid import uuid4

from tools.flashcard_deck import ID, validate_deck

LOG = logging.getLogger(__name__)
MAX_ROWS = 25
FORM_FIELDS = 5 + 2 * MAX_ROWS


@dataclass
class Draft:
    deck_id: str
    title: str
    rows: list

    @classmethod
    def new(cls):
        return cls(uuid4().hex, "", [("", "") for _ in range(5)])

    @classmethod
    def read(cls, data):
        try:
            count = int(data["count"])
            ident = data["deck_id"]
            if not 1 <= count <= MAX_ROWS or not ID.fullmatch(ident):
                raise ValueError("Invalid editor state")
            expected = {"csrf", "count", "deck_id", "title", "action"}
            for i in range(count):
                expected.update((f"front_{i}", f"back_{i}"))
            if set(data) != expected:
                raise ValueError("Invalid editor fields")
            rows = [(data[f"front_{i}"], data[f"back_{i}"]) for i in range(count)]
            # Keep error redisplay bounded even for hostile submissions.
            if len(data["title"]) > 120 or any(len(s) > 500 for row in rows for s in row):
                raise ValueError("Card or title exceeds the editor limit")
            return cls(ident, data["title"], rows)
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
            return validate_deck({"deck_id": self.deck_id, "version": 1,
                                  "title": self.title.strip(), "cards": cards})
        except ValueError:
            LOG.warning("Visual deck validation rejected")
            raise


def editor(draft, csrf, *, preview=None, error=""):
    """All user-controlled strings escaped; no scripts or external resources."""
    def attr(s):
        return escape(str(s), quote=True)
    hidden = "".join(f'<input type="hidden" name="{name}" value="{attr(value)}">'
                     for name, value in (("csrf", csrf), ("deck_id", draft.deck_id),
                                         ("count", len(draft.rows))))
    rows = []
    for i, (front, back) in enumerate(draft.rows):
        remove = (f'<button name="action" value="remove:{i}" type="submit" '
                  f'aria-label="Remove card {i+1}">Remove card {i+1}</button>'
                  if len(draft.rows) > 1 else "")
        rows.append(f'<fieldset style="margin:16px 0;border:1px solid #dce2ea;border-radius:8px;padding:12px">'
                    f'<legend>Card {i+1}</legend><label for="front_{i}">Question / front</label>'
                    f'<textarea id="front_{i}" name="front_{i}" rows="2" maxlength="500">{escape(front)}</textarea>'
                    f'<label for="back_{i}">Answer / back</label>'
                    f'<textarea id="back_{i}" name="back_{i}" rows="2" maxlength="500">{escape(back)}</textarea>'
                    f'{remove}</fieldset>')
    preview_html = ""
    if preview is not None:
        items = "".join(f'<li><strong>{escape(c["front"])}</strong><p>{escape(c["back"])}</p></li>'
                        for c in preview["cards"])
        preview_html = (f'<section><h2>Preview — {len(preview["cards"])} cards</h2>'
                        f'<p>{escape(preview["title"])}</p><ol>{items}</ol>'
                        '<p>Saved decks are not yet assigned to a student.</p></section>')
    add = ('<button name="action" value="add">Add another card</button>'
           if len(draft.rows) < MAX_ROWS else '<p>25-card editor limit reached. CSV supports larger decks.</p>')
    save = '<button name="action" value="save">Save deck</button>' if preview is not None else ""
    notice = f'<p role="alert" class="notice">{escape(error)}</p>' if error else ""
    return (f'{notice}{preview_html}<section><p>Type a question and its answer. Empty cards are ignored.</p>'
            f'<form method="post" action="/decks/new">{hidden}'
            f'<label for="deck-title">Deck title</label><input id="deck-title" name="title" '
            f'maxlength="120" value="{attr(draft.title)}">{"".join(rows)}'
            f'{add}<button name="action" value="preview">Preview cards</button>{save}'
            '</form><p><a href="/">Return to dashboard (unsaved work will be lost)</a></p></section>')


def handle_editor(data, csrf, decks, token):
    """Return (draft, preview, error, saved); no hidden state or session draft."""
    draft = Draft.read(data)
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
            decks.save(token, deck)
        except sqlite3.IntegrityError:
            LOG.warning("Duplicate visual deck save rejected")
            return draft, deck, "This deck ID was already saved. Check the flashcard library before creating another copy.", False
        return draft, None, "", True
    else:
        raise ValueError("Unknown editor action")
    return draft, None, "", False
