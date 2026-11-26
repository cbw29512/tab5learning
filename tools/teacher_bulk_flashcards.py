"""Offline bulk flashcard authoring guide and preview; no AI/network calls."""
import csv
import hashlib
import hmac
import logging
from html import escape
from uuid import uuid4

from tools.flashcard_deck import import_csv, MAX_CARDS, MAX_BYTES

LOG = logging.getLogger(__name__)
AI_PROMPT = "Create 30 age-appropriate study flashcards for Grade 3 Mathematics from the lesson notes below.\nReturn ONLY valid UTF-8 CSV, with the exact header front,back.\nUse one question and one short accurate answer per row. Quote both fields using double quotes, and escape any internal double quote by doubling it.\nNo markdown, code fences, numbering, extra columns or commentary.\nUse the supplied material only. Do not invent facts. Avoid duplicate questions. Keep both sides under 120 characters where possible (500 maximum).\nIf there is not enough material for 30 accurate cards, produce fewer.\nLesson notes:\n[Paste non-sensitive teaching material here]"

def bulk_form(csrf, title="", text="", *, deck=None, proof="", error=""):
    """Escape all generated content and require a deliberate review before save."""
    preview = ""
    if deck is not None:
        rows = "".join(
            f'<tr><td>{i}</td><td>{escape(card["front"])}</td>'
            f'<td>{escape(card["back"])}</td></tr>'
            for i, card in enumerate(deck["cards"], 1)
        )
        preview = (f'<section><h2>Review {len(deck["cards"])} cards</h2>'
                   '<p>Check accuracy, age appropriateness and duplicates before saving.</p>'
                   '<table><thead><tr><th>#</th><th>Question</th><th>Answer</th></tr>'
                   f'</thead><tbody>{rows}</tbody></table></section>')
    # Keep the original input available so teachers can correct errors.
    return f"""<section><h2>1. Generate cards using your preferred AI</h2>
    <p>Copy the prompt below, change the grade, subject and card count, and
    paste your lesson material into your chosen AI tool. Do not share student
    names, grades, work, identifiers or other private information.</p>
    <label for="ai-prompt">Copyable prompt</label>
    <textarea id="ai-prompt" readonly rows="12">{escape(AI_PROMPT)}</textarea>
    <p>AI is optional. You may also paste CSV from a spreadsheet or write it yourself.
    Teacher Studio makes no external AI requests.</p></section>
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
        if set(data) != {"csrf", "title", "csv", "proof", "action"}:
            raise ValueError("Invalid form")
        deck = import_csv(text, uuid4().hex, title)
        expected = _proof(key, token, title, text)
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
