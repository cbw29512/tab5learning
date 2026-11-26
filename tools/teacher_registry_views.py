"""Escaped registry markup; planned records are not hardware evidence."""
from html import escape


def registry_panel(assignments, csrf):
    hidden = f'<input type="hidden" name="csrf" value="{escape(csrf, quote=True)}">'
    rows = "".join(
        f'<tr><td>{escape(a["student_id"])}</td><td>{escape(a["device_id"])}</td>'
        f'<td>{a["starting_grade"]}</td><td>Planned — not synced</td></tr>'
        for a in assignments)
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
    <p><small>USB provisioning and grading are not yet connected.</small></p></section>"""
