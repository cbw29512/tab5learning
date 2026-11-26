"""Escaped synthetic student archive forms; no sensitive values in URLs."""
from html import escape
from itertools import islice

from tools.protected_backup_center import hidden, key_field
from tools.protected_math_codec import MAX_PROTECTED
from tools.teacher_backup_center import MAX_FILES

ROUTE = "/backups/students"


def form(guard, body):
    return f'<form method="post" action="{ROUTE}">{guard}{body}</form>'


def student_page(assignments, csrf, *, folder=None, selected=None, name_pattern=None,
                 notice="", generated=None, proof="", filename="", digest="", summary=None):
    guard = hidden("csrf", csrf)
    options = "".join(
        f'<option value="{escape(row["student_id"], quote=True)}">{escape(row["student_id"])} '
        f'— {escape(row["device_id"])}</option>' for row in assignments)
    chooser = form(guard, '<label>Planned student<select name="student" required>'
                   + options + '</select></label><button name="action" value="select">Open student backup</button>') if options else "<p>No accessible planned students.</p>"
    body = ('<p class="notice">TEST MODE — synthetic mathematics history only.</p>'
            '<p>Uses the last validated history stored on this computer. It may be stale. '
            'No device is read or written, and other subjects are not included.</p>'
            f'<p role="status">{escape(notice)}</p><section>{chooser}</section>')
    if selected is None:
        return body + '<p><a href="/backups">Return to backup center</a></p>'
    student, device = selected
    scope = hidden("student", student)
    body += (f'<section><h2>{escape(student)} — {escape(device)}</h2>'
             + form(guard+scope, '<button name="action" value="generate">Generate recovery key for backup</button>') + '</section>')
    if generated is not None:
        body += (f'<section><h2>Retain recovery key separately</h2><p><code>{escape(generated)}</code></p>'
                 '<p>Store this key securely away from the archive. Losing it prevents recovery.</p>'
                 + form(guard+scope+hidden("proof",proof), key_field()+key_field("repeat_key","Re-enter saved key")
                        + '<label><input type="checkbox" name="saved_key" value="yes" required> I retained the key.</label>'
                        '<button name="action" value="save">Save encrypted mathematics history</button>') + '</section>')
    if summary is not None:
        body += (f'<section><h2>Review student restore</h2><p>{escape(filename)}</p>'
                 f'<p>{summary.records_to_add} attempts to add. '
                 + ("Exact prior snapshot already present. " if summary.already_present else "")
                 + 'Newer history is preserved. This restores the computer archive only.</p>'
                 + form(guard+scope+hidden("filename",filename)+hidden("digest",digest)+hidden("proof",proof),
                        key_field()+'<button name="action" value="restore">Confirm student archive restore</button>')
                 + '</section>')
    rows = []
    # Filenames do not reveal student identity; decrypt and verify before selection is accepted.
    for path in islice(folder.glob("pocket-student-test-*.plse"), MAX_FILES):
        if not name_pattern.fullmatch(path.name) or path.is_symlink():
            continue
        try:
            if not path.is_file() or not 0 < path.stat().st_size <= MAX_PROTECTED:
                continue
        except OSError:
            continue
        rows.append(f'<li>{escape(path.name)} — locked; student identity not yet checked'
                    + form(guard+scope+hidden("filename",path.name),key_field()
                           + '<button name="action" value="preview">Unlock and preview restore</button>') + '</li>')
    return (body+f'<section><h2>Locked student archives (up to {MAX_FILES})</h2>'
            f'<ul>{"".join(rows) or "<li>No archives found</li>"}</ul></section>'
            '<p><a href="/backups">Return to backup center</a></p>')
