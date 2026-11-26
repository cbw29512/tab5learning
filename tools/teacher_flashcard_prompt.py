"""Bounded offline prompt builder; never contacts an AI service."""
import logging
from html import escape

LOG = logging.getLogger(__name__)
SUBJECTS = ("Mathematics", "Reading", "Writing", "Spelling", "Science",
            "Social Studies", "Custom")
GRADES = ("1", "2", "3", "4")
MAX_NOTES_BYTES = 12000

def build_prompt(grade="3", subject="Mathematics", count="30", notes=""):
    try:
        if grade not in GRADES or subject not in SUBJECTS:
            raise ValueError("Select a valid grade and subject")
        if not isinstance(count, str) or not count.isascii() or not count.isdecimal() or len(count) > 3:
            raise ValueError("Select 1–100 cards per AI batch")
        number = int(count)
        if not 1 <= number <= 100:
            raise ValueError("Select 1–100 cards per AI batch")
        if not isinstance(notes, str) or len(notes.encode("utf-8")) > MAX_NOTES_BYTES:
            raise ValueError("Lesson notes exceed 12,000 UTF-8 bytes")
        return (
            f"Create {number} age-appropriate study flashcards for Grade {grade} {subject} "
            "from the lesson notes below.\n"
            "Return ONLY valid UTF-8 CSV, with the exact header front,back.\n"
            "Use one question and one short accurate answer per row. Quote both fields "
            "using double quotes, and escape any internal double quote by doubling it.\n"
            "No markdown, code fences, numbering, extra columns or commentary.\n"
            "Use the supplied material only. Do not invent facts. Avoid duplicate questions. "
            "Keep both sides under 120 characters where possible (500 maximum).\n"
            f"If there is not enough material for {number} accurate cards, produce fewer.\n"
            "Lesson notes:\n" + (notes.strip() or "[Paste non-sensitive teaching material here]")
        )
    except (ValueError, TypeError, UnicodeError):
        LOG.warning("Flashcard prompt input rejected")
        raise

def prompt_panel(csrf, *, grade="3", subject="Mathematics", count="30", notes="", error=""):
    def options(values, current):
        return "".join(
            f'<option value="{escape(v, quote=True)}"{" selected" if v == current else ""}>'
            f'{escape(v)}</option>' for v in values
        )
    try:
        generated = build_prompt(grade, subject, count, notes)
    except (ValueError, TypeError, UnicodeError):
        grade, subject, count, notes = "3", "Mathematics", "30", ""
        generated = build_prompt()
        error = error or "Invalid grade, subject, count or lesson notes. Check the input limits."
    return f"""<section><h2>1. Generate cards using your preferred AI</h2>
    <p>Do not share student names, grades, work, identifiers or other private information.
    Teacher Studio makes no external AI requests.</p>
    <p role="alert">{escape(error)}</p>
    <form method="post" action="/decks/bulk">
    <input type="hidden" name="csrf" value="{escape(csrf, quote=True)}">
    <label for="grade">Grade level</label><select id="grade" name="grade">{options(GRADES, grade)}</select>
    <label for="subject">Subject</label><select id="subject" name="subject">{options(SUBJECTS, subject)}</select>
    <label for="count">Cards per AI batch (1–100)</label>
    <input id="count" name="count" type="number" min="1" max="100" required value="{escape(count, quote=True)}">
    <label for="notes">Non-sensitive lesson material (optional)</label>
    <textarea id="notes" name="notes" maxlength="{MAX_NOTES_BYTES}" rows="6">{escape(notes)}</textarea>
    <button name="action" value="generate">Generate AI prompt</button></form>
    <label for="ai-prompt">Copyable prompt</label>
    <textarea id="ai-prompt" readonly rows="12">{escape(generated)}</textarea>
    <p>Select and copy the prompt into your chosen AI, then paste its CSV below.
    AI is optional; spreadsheet or manually written CSV also works.</p></section>"""
