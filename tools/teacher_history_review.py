"""Read-only synthetic mathematics review; no USB, scoring edits or student writes."""
import hashlib
import logging
import math
import re
import struct
from html import escape

from tools.math_backup_codec import checkpoint, decode, MAX_RECORDS

LOG = logging.getLogger(__name__)
PAGE_SIZE = 10
# Exact V1 preview bank in firmware/include/math_practice.h, not curriculum grades.
QUESTIONS = ((2,3,(4,5,6,7)), (4,2,(6,8,5,7)), (3,4,(5,8,7,6)),
             (5,4,(8,10,7,9)), (6,4,(10,9,11,8)))


def hidden(name, value):
    return f'<input type="hidden" name="{name}" value="{escape(str(value), quote=True)}">'


def form(csrf, student, page, label):
    return ('<form method="post" action="/history">' + hidden("csrf", csrf)
            + hidden("student", student) + hidden("page", page)
            + f'<button>{escape(label)}</button></form>')


def ink_picture(record):
    # Only validated original numeric points enter the SVG. Every pen-down
    # starts a new subpath; a stationary single point is rendered as a dot.
    count = int.from_bytes(record[6:8], "little")
    if not count:
        return "<p>No handwriting captured for this attempt.</p>"
    segments = []
    for i in range(count):
        x, y, start = struct.unpack_from("<HHB", record, 24 + 5*i)
        if start:
            segments.append([])
        segments[-1].append((x,y))
    marks = []
    for segment in segments:
        if len(segment) == 1:
            x,y = segment[0]
            marks.append(f'<circle cx="{x}" cy="{y}" r="3.5" fill="#17233a"/>')
        else:
            points = " ".join(f"{x},{y}" for x,y in segment)
            marks.append(f'<polyline points="{points}" fill="none" stroke="#17233a" '
                         'stroke-width="7" stroke-linecap="round" stroke-linejoin="round"/>')
    return ('<svg role="img" aria-label="Original handwriting" viewBox="0 0 592 244" '
            'style="width:100%;max-width:592px;background:#fff;border:1px solid #aeb8c8">'
            + "".join(marks) + f'</svg><p>{count} captured ink points.</p>')


class HistoryReview:
    def __init__(self, archive):
        self.archive = archive

    def page(self, token, csrf, *, student=None, page="1"):
        try:
            assignments = self.archive.registry.list_assignments(token)
            options = "".join(
                f'<option value="{escape(row["student_id"], quote=True)}">'
                f'{escape(row["student_id"])} — {escape(row["device_id"])}</option>'
                for row in assignments if row["status"] == "PLANNED")
            chooser = ('<form method="post" action="/history">' + hidden("csrf", csrf)
                       + hidden("page", 1) + '<label>Planned student</label><select name="student" required>'
                       + options + '</select><button>Review history</button></form>') if options else "<p>No accessible planned students.</p>"
            body = ('<p class="notice">TEST MODE — synthetic mathematics history only. '
                    'Last verified computer archive; may be stale. No device is read or written.</p>'
                    f'<section>{chooser}</section><p><a href="/">Return to dashboard</a></p>')
            if student is None:
                return body
            identity = next((row for row in assignments if row["status"] == "PLANNED"
                             and row["student_id"] == student), None)
            if identity is None:
                raise PermissionError("Student outside current teacher scope")
            if not isinstance(page,str) or not re.fullmatch(r"[1-9][0-9]{0,3}",page):
                raise ValueError("Invalid history page")
            number = int(page)
            if number > math.ceil(MAX_RECORDS/PAGE_SIZE):
                raise ValueError("Invalid history page")
            try:
                raw = self.archive.load(token, identity["device_id"], student)
                snapshot = decode(raw)
            except ValueError:
                # Missing and damaged history both remain unavailable; never
                # report zero progress merely because verification failed.
                LOG.warning("Verified history unavailable")
                return body + "<section><p>No verified history available. Check synchronization or archive integrity.</p></section>"
            count = len(snapshot.records)
            pages = max(1, math.ceil(count/PAGE_SIZE))
            if number > pages:
                raise ValueError("Page outside verified history")
            base, final = checkpoint(snapshot.base), checkpoint(snapshot.final)
            body += (f'<section><h2>{escape(student)} — {escape(identity["device_id"])}</h2>'
                     f'<p>Retained attempts: {count} · Total recorded attempts: {final.attempts}</p>'
                     f'<p>Completed preview questions: {final.index}/{len(QUESTIONS)} · '
                     f'Independent answers: {final.independent}</p>'
                     '<p>Practice statistics, not mastery grades or handwriting scores.</p>'
                     f'<p>Archive SHA-256: <code style="overflow-wrap:anywhere">{hashlib.sha256(raw).hexdigest()}</code></p>')
            if base.attempts:
                body += f'<p>{base.attempts} earlier attempts have summary-only history; original evidence unavailable.</p>'
            body += f'<p>Page {number} of {pages}</p></section>'
            start = (number-1)*PAGE_SIZE
            for record in snapshot.records[start:start+PAGE_SIZE]:
                a,b,choices = QUESTIONS[record[3]]
                after = checkpoint(record[8:24])
                result = "Incorrect — retry" if record[5] == 1 else "Correct"
                body += (f'<section><h2>Attempt {after.attempts}</h2>'
                         f'<p>{a} + {b} = ? · Selected: {choices[record[4]]} · Expected: {a+b}</p>'
                         f'<p>{result}</p>{ink_picture(record)}</section>')
            if number > 1:
                body += form(csrf,student,number-1,"Previous page")
            if number < pages:
                body += form(csrf,student,number+1,"Next page")
            return body
        except Exception:
            LOG.warning("History review rejected or failed")
            raise
