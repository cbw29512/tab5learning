"""Read-only educator curriculum preview, never a learner installer."""
import logging
from pathlib import Path
from html import escape

from tools.curriculum_draft import decode_draft, MAX_BYTES

LOG = logging.getLogger(__name__)
SOURCE = Path(__file__).resolve().parents[1] / "curriculum" / "grade-1" / "starter.v16.json"
SUBJECTS = (("mathematics","Mathematics"),("english_language_arts","English language arts"),
            ("science","Science"),("social_studies","Social studies"))


def curriculum_page(lesson_id=None):
    try:
        # The browser cannot supply a filesystem path or arbitrary package.
        with SOURCE.open("rb") as stream:
            catalog, digest = decode_draft(stream.read(MAX_BYTES+1))
        lessons = catalog["lessons"]
        body = ('<p class="notice">Educator preview only — draft material, not approved coverage. '
                'No learner installation or student data.</p>'
                '<p><a href="/">Return to dashboard</a></p>'
                f'<p>Grade 1 · Revision {catalog["revision"]} · {len(lessons)} draft lessons · '
                f'{sum(len(x["activities"]) for x in lessons)} activities · 0 approved subjects</p>')
        if lesson_id is None:
            for key, title in SUBJECTS:
                selected = [x for x in lessons if x["subject"] == key]
                body += f'<section><h2>{title}</h2><ul>'
                for item in selected:
                    body += (f'<li><a href="/curriculum/{escape(item["lesson_id"],quote=True)}">'
                             f'{escape(item["title"])}</a> — {item["minutes"]} min</li>')
                body += '</ul></section>'
            return body
        item = next((x for x in lessons if x["lesson_id"] == lesson_id), None)
        if item is None:
            raise ValueError("Unknown draft lesson")
        body += (f'<p><a href="/curriculum">All draft lessons</a></p><section><h2>{escape(item["title"])}</h2>'
                 f'<p>{escape(item["objective"])}</p><p>Lesson revision {item["revision"]}</p><ul>')
        for ref in item["standards"]:
            body += f'<li>{escape(ref["framework"])} · {escape(ref["edition"])} · {escape(ref["id"])} (partial draft)</li>'
        body += '</ul></section>'
        for step in item["activities"]:
            body += f'<section><h2>{escape(step["phase"].replace("_"," ").title())}</h2><p>{escape(step["prompt"])}</p>'
            if step["kind"] == "choice":
                body += '<ol>' + "".join(f'<li>{escape(x)}</li>' for x in step["choices"]) + '</ol>'
                body += f'<p>Answer key: {escape(step["choices"][step["answer_index"]])}</p>'
            if step["feedback"]:
                body += f'<p>Teacher guidance: {escape(step["feedback"])}</p>'
            body += '</section>'
        return body + f'<p>Catalog SHA-256: <code>{digest}</code></p>'
    except Exception:
        LOG.warning("Curriculum preview failed")
        raise
