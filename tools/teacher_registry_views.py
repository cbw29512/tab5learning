"""Escaped registry markup; planned records are not hardware evidence."""
from html import escape


def registry_panel(assignments, csrf, enrollment=None, credential_storage=None, can_migrate=False):
    hidden = f'<input type="hidden" name="csrf" value="{escape(csrf, quote=True)}">'
    rows = "".join(
        f'<tr><td>{escape(a["student_id"])}</td><td>{escape(a["device_id"])}</td>'
        f'<td>{a["starting_grade"]}</td><td>Planned — not synced</td></tr>'
        for a in assignments)
    enrollment_rows = ""
    if enrollment is not None:
        for a in assignments:
            device = escape(a["device_id"], quote=True)
            if a["device_id"] in enrollment:
                storage = (credential_storage or {}).get(a["device_id"])
                detail = {"legacy": "Legacy test key (previously unencrypted)",
                          "protected": "Protected host record (test only)",
                          "conflict": "Conflicting records — maintenance required"}.get(storage, "")
                action = ""
                if storage == "legacy" and can_migrate:
                    action = (f'<form method="post" action="/protect-credential">{hidden}'
                              f'<input type="hidden" name="device_id" value="{device}">'
                              '<label><input type="checkbox" name="confirm" value="yes" required '
                              'style="display:inline;width:auto"> Protect the existing test key locally '
                              '(does not erase older copies or install on Tab5).</label>'
                              '<button>Protect existing key</button></form>')
                enrollment_rows += f"<li>{device} — KEY_PENDING (not installed)<p>{detail}</p>{action}</li>"
            else:
                enrollment_rows += f'<li><form method="post" action="/enrollment">{hidden}<input type="hidden" name="device_id" value="{device}"><button>Prepare key for {device}</button></form></li>'
        enrollment_rows = "<h3>Device enrollment</h3><p>Prepare a local test credential only. No hardware changes.</p><ul>" + enrollment_rows + "</ul>"
    return f"""<section><h2>Student and device management</h2>
    <p>Test assignments only. Enter anonymous IDs, not children's names.
    A planned assignment does not change the Tab5.</p>
    <table style="width:100%;text-align:left"><thead><tr>
    <th>Student</th><th>Device</th><th>Grade</th><th>Status</th>
    </tr></thead><tbody>{rows or '<tr><td colspan="4">No assignments yet</td></tr>'}</tbody></table>
    <form method="post" action="/assignments">{hidden}
    <label for="student">Student ID</label>
    <input id="student" name="student_id" required maxlength="64" pattern="[A-Za-z0-9_-]+">
    <label for="device">Device ID</label>
    <input id="device" name="device_id" required maxlength="64" pattern="[A-Za-z0-9_-]+">
    <label for="grade">Starting grade</label>
    <input id="grade" name="grade" type="number" required min="1" max="3" step="1" value="1">
    <button>Create planned assignment</button></form>
    {enrollment_rows}
    <p><small>USB provisioning and grading are not yet connected.</small></p></section>"""
