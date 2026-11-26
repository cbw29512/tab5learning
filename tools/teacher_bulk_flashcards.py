"""Offline bulk flashcard authoring guide and preview; no AI/network calls."""
import csv
import hashlib
import hmac
import logging
from html import escape
from uuid import uuid4

from tools.flashcard_deck import import_csv, MAX_CARDS, MAX_BYTES
from tools.teacher_flashcard_prompt import build_prompt, prompt_panel
from tools.flashcard_quality import inspect_cards, without_identical_cards

LOG = logging.getLogger(__name__)
AI_PROMPT = build_prompt()

def bulk_form(csrf, title="", text="", *, deck=None, proof="", error="", prompt_options=None):
    """Escape all generated content and require a deliberate review before save."""
    preview = ""
    if deck is not None:
        findings = inspect_cards(deck)
        warnings = []
        if findings.duplicates:
            pairs = ", ".join(f"{first}/{later}" for first, later in findings.duplicates)
            warnings.append(f"<p>{len(findings.duplicates)} identical duplicate cards. "
                            f"Matching preview rows: {pairs}.</p>"
                            '<button name="action" value="deduplicate">Remove identical duplicates and preview</button>')
        if findings.conflicts:
            pairs = ", ".join(f"{first}/{later}" for first, later in findings.conflicts)
            warnings.append(f"<p>Repeated questions with different answers: {pairs}. "
                            "Review and edit the CSV if needed. These cards are never removed automatically.</p>")
        rows = "".join(
            f'<tr><td>{i}</td><td>{escape(card["front"])}</td>'
            f'<td>{escape(card["back"])}</td></tr>'
            for i, card in enumerate(deck["cards"], 1)
        )
        preview = (f'<section><h2>Review {len(deck["cards"])} cards</h2>'
                   '<p>Check accuracy, age appropriateness and duplicates before saving.</p>'
                   + ''.join(warnings) +
                   '<table><thead><tr><th>#</th><th>Question</th><th>Answer</th></tr>'
                   f'</thead><tbody>{rows}</tbody></table></section>')
    # Keep the original input available so teachers can correct errors.
    panel = prompt_panel(csrf, **(prompt_options or {}))
    return panel + f"""
    <section><h2>2. Paste and preview</h2>
    <p>Use the exact front,back header. Maximum {MAX_CARDS} cards,
    {MAX_BYTES // 1024} KiB, and 500 characters per side.</p>
    <p>Suggested deck title: Grade 3 · Mathematics · Multiplication</p>
    <p role="alert">{escape(error)}</p>
    <form method="post" action="/decks/bulk">
      <input type="hidden" name="csrf" value="{escape(csrf, quote=True)}">
      <input type="hidden" name="proof" value="{escape(proof, quote=True)}">
      <label for="title">Deck title</label>
      <input id="title" name="title" required maxlength="120" value="{escape(title, quote=True)}">
      <label for="csv">Paste CSV output</label>
      <textarea id="csv" name="csv" required rows="12">{escape(text)}</textarea>
      <button name="action" value="preview">Validate and preview</button>
      {preview}
      {'<button name="action" value="save">Save reviewed deck</button>' if deck is not None else ''}
    </form><p><a href="/">Return to dashboard</a></p></section>"""

def _proof(key, token, title, text):
    # Bind preview to exact input and authenticated session. No student data
    # or generated content is sent outside the local server.
    digest = hashlib.sha256()
    for field in (token, title, text):
        encoded = field.encode("utf-8")
        digest.update(len(encoded).to_bytes(8, "big"))
        digest.update(encoded)
    return hmac.new(key, b"bulk-flashcards-v1:" + digest.digest(), hashlib.sha256).hexdigest()

def handle_bulk(data, *, csrf, key, token, decks):
    """Validate and preview; persist only an explicitly reviewed exact input."""
    title, text = data.get("title", ""), data.get("csv", "")
    try:
        if data.get("action") == "generate":
            if set(data) != {"csrf", "grade", "subject", "count", "notes", "action"}:
                raise ValueError("Invalid prompt form")
            options = {k: data[k] for k in ("grade", "subject", "count", "notes")}
            return bulk_form(csrf, prompt_options=options), False
        # HTML textareas normalize line endings on submit. Canonicalize both
        # preview and save so reviewed content survives the browser round trip.
        text = text.replace("\r\n", "\n").replace("\r", "\n")
        if set(data) != {"csrf", "title", "csv", "proof", "action"}:
            raise ValueError("Invalid form")
        deck = import_csv(text, uuid4().hex, title)
        expected = _proof(key, token, title, text)
        if data["action"] == "deduplicate":
            if not hmac.compare_digest(data["proof"], expected):
                return bulk_form(csrf, title, text, deck=deck, proof=expected,
                                 error="Content changed. Review again before removing duplicates."), False
            cleaned, removed = without_identical_cards(deck)
            deck = import_csv(cleaned, deck["deck_id"], title)
            return bulk_form(csrf, title, cleaned, deck=deck,
                             proof=_proof(key, token, title, cleaned),
                             error=f"Removed {removed} identical duplicate cards. Review the revised deck before saving."), False
        if data["action"] == "preview":
            return bulk_form(csrf, title, text, deck=deck, proof=expected), False
        if data["action"] != "save" or not hmac.compare_digest(data["proof"], expected):
            return bulk_form(csrf, title, text, deck=deck, proof=expected,
                             error="Content changed or was not previewed. Review again before saving."), False
        decks.save(token, deck)
        return "", True
    except (ValueError, TypeError, UnicodeError, csv.Error):
        LOG.warning("Bulk flashcard input rejected")
        return bulk_form(csrf, title[:120], text[:MAX_BYTES],
                         error="Invalid CSV or deck. Check the header, quoting, text and card limits."), False
