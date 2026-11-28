"""Read-only Grade 1 educator coverage inventory; never certify draft evidence."""
import json
import logging
from pathlib import Path
from html import escape

LOG = logging.getLogger(__name__)
SOURCE = Path(__file__).resolve().parents[1] / "curriculum" / "grade-1" / "coverage.v8.json"
MAX_LEDGER_BYTES = 1024 * 1024
SUBJECTS = (
    ("mathematics", "Mathematics"),
    ("english_language_arts", "English language arts"),
    ("science", "Science"),
    ("social_studies", "Social studies"),
)


def validate_ledger(raw, catalog, catalog_name):
    """Validate revision-pinned references before showing any coverage counts."""
    try:
        if len(raw) > MAX_LEDGER_BYTES:
            raise ValueError("Coverage ledger exceeds size limit")
        ledger = json.loads(raw)
        if (ledger["schema_version"] != 1 or ledger["grade"] != "1"
                or ledger["catalog"] != catalog_name
                or ledger["catalog_revision"] != catalog["revision"]):
            raise ValueError("Coverage ledger/catalog mismatch")
        subjects = dict(SUBJECTS)
        lessons = {x["lesson_id"]: x for x in catalog["lessons"]}
        rows = ledger["rows"]
        if not isinstance(rows, list) or not 0 < len(rows) <= 500:
            raise ValueError("Invalid coverage inventory")
        if set(ledger["frameworks"]) != set(subjects):
            raise ValueError("Unexpected coverage frameworks")
        seen = set()
        for row in rows:
            key = (row["subject"], row["indicator"])
            if key[0] not in subjects or not key[1] or key in seen:
                raise ValueError("Invalid or duplicate coverage indicator")
            seen.add(key)
            if row["status"] not in {"partial_draft", "missing"}:
                raise ValueError("Unsupported coverage state")
            # This provisional viewer cannot recognize or grant approval.
            if row["educator_review"] is not None or row["hardware_acceptance"] is not None:
                raise ValueError("Review records require an approved review schema")
            if not all(isinstance(row[k], str) and row[k].strip()
                       for k in ("indicator", "objective", "remaining")):
                raise ValueError("Invalid objective or gap description")
            evidence = row["evidence"]
            if not isinstance(evidence, list) or bool(evidence) != (row["status"] == "partial_draft"):
                raise ValueError("Coverage state/evidence mismatch")
            for ref in evidence:
                lesson = lessons.get(ref["lesson_id"])
                if (lesson is None or lesson["subject"] != key[0]
                        or ref["revision"] != lesson["revision"]):
                    raise ValueError("Invalid lesson reference")
                ids = ref["activity_ids"]
                if (not isinstance(ids, list) or not ids or len(ids) != len(set(ids))
                        or not set(ids).issubset({a["activity_id"] for a in lesson["activities"]})):
                    raise ValueError("Invalid activity references")
        return ledger
    except (ValueError, KeyError, TypeError, AttributeError, OverflowError) as exc:
        LOG.warning("Coverage ledger validation failed: %s", exc)
        raise ValueError("Invalid Grade 1 coverage inventory") from exc


def coverage_page(catalog, catalog_name):
    """Return escaped HTML only; all data stays local and no review is written."""
    try:
        with SOURCE.open("rb") as stream:
            ledger = validate_ledger(stream.read(MAX_LEDGER_BYTES + 1), catalog, catalog_name)
        rows = ledger["rows"]
        lessons = {x["lesson_id"]: x for x in catalog["lessons"]}
        partial = sum(r["status"] == "partial_draft" for r in rows)
        body = [
            '<p><a href="/curriculum">All draft lessons</a></p>',
            '<h2>Grade 1 standards review</h2>',
            '<p class="notice">Provisional objective inventory. Candidate material is not '
            'verified coverage, educator approval, hardware acceptance or learner mastery.</p>',
            f'<p>{partial} of {len(rows)} provisional objectives have draft candidates; '
            f'{len(rows)-partial} missing; 0 approved.</p>',
            '<p>Full official indicator reconciliation and educator review remain required.</p>',
        ]
        for key, title in SUBJECTS:
            framework = ledger["frameworks"][key]
            items = [r for r in rows if r["subject"] == key]
            count = sum(r["status"] == "partial_draft" for r in items)
            body.append(f'<section><h2>{escape(title)}</h2><p>{count}/{len(items)} '
                        'candidate objectives; 0 approved.</p>')
            body.append(f'<p>{escape(framework["edition"])}; {escape(framework["pages"])}</p>')
            body.append('<p>Official reconciliation: ' +
                        ('recorded in inventory' if framework["reconciled"] is True
                         else 'pending') + '</p>')
            for row in items:
                label = "Draft candidate" if row["status"] == "partial_draft" else "Missing"
                body.append(f'<details><summary>{escape(row["indicator"])} — '
                            f'{escape(row["objective"])} ({label})</summary>')
                body.append(f'<p><strong>Remaining:</strong> {escape(row["remaining"])}</p>')
                if row["evidence"]:
                    body.append('<ul>')
                    for ref in row["evidence"]:
                        lesson = lessons[ref["lesson_id"]]
                        url = escape(lesson["lesson_id"], quote=True)
                        body.append(f'<li><a href="/curriculum/{url}">'
                                    f'{escape(lesson["title"])}</a> · revision {ref["revision"]}'
                                    f' · {len(ref["activity_ids"])} candidate activities</li>')
                    body.append('</ul>')
                body.append('<p>Educator review: pending. Device acceptance: pending.</p></details>')
            body.append('</section>')
        return "".join(body)
    except Exception:
        LOG.exception("Unable to render Grade 1 standards review")
        raise
