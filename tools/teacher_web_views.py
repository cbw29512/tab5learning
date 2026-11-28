"""Escaped, offline HTML for the local Teacher Studio prototype."""
from html import escape
from tools.teacher_registry_views import registry_panel
from tools.teacher_deck_editor_views import MAX_ROWS
from tools.teacher_usb_discovery import scan_panel

STYLE = """
*{box-sizing:border-box}body{margin:0;background:#f4f6fa;color:#17233a;
font:16px system-ui,sans-serif}header{background:#17233a;color:white;padding:22px}
main{max-width:920px;margin:32px auto;padding:0 20px}section{background:white;
border:1px solid #dce2ea;border-radius:12px;padding:24px;margin-bottom:20px}
h1,h2{margin-top:0}label{display:block;margin:12px 0 5px}input,textarea{
display:block;width:100%;padding:10px;border:1px solid #aeb8c8;border-radius:6px;
font:inherit}button{background:#2456a6;color:white;border:0;border-radius:6px;
padding:11px 18px;margin-top:12px;font:inherit;cursor:pointer}
small{color:#536078}.notice{padding:12px;border-left:4px solid #2456a6;background:#e9effb}
"""

def page(title, body, notice=""):
    banner = f'<p class="notice">{escape(notice)}</p>' if notice else ""
    return (f'<!doctype html><html lang="en"><head><meta charset="utf-8">'
            f'<meta name="viewport" content="width=device-width,initial-scale=1">'
            f'<title>{escape(title)} | Pocket Learning Studio</title><style>{STYLE}</style>'
            f'</head><body><header>Pocket Learning Studio · Local teacher workspace</header>'
            f'<main><h1>{escape(title)}</h1>{banner}{body}</main></body></html>')


def login(notice="", *, test_mode=False, csrf=""):
    if test_mode:
        notice = "TEST MODE — synthetic data only. " + notice
    return page("Teacher sign in", f"""
    <section><form method="post" action="/login">
    <input type="hidden" name="csrf" value="{escape(csrf, quote=True)}">
    <label for="username">Username</label>
    <input id="username" name="username" required maxlength="64" autocomplete="username">
    <label for="password">Password</label>
    <input id="password" name="password" type="password" required maxlength="256"
     autocomplete="current-password"><button>Sign in</button></form>
    <p><small>First administrator setup is available only from the local maintenance console.</small></p>
    </section>""", notice)


def workspace(username, role, decks, csrf, notice="", *, minimum=15, test_mode=False, assignments=None, enrollment=None, credential_storage=None, can_migrate=False, backup_enabled=False, probe_enabled=False):
    if test_mode:
        notice = "TEST MODE — synthetic data only. " + notice
    hidden = f'<input type="hidden" name="csrf" value="{escape(csrf, quote=True)}">'
    latest = {}
    for deck in decks:
        key = deck["deck_id"]
        if key not in latest or deck["version"] > latest[key]["version"]:
            latest[key] = deck
    rows = "".join(
        f'<li>{escape(d["title"])} — {len(d["cards"])} cards · Version {d["version"]}'
        + (f'<form method="post" action="/decks/edit">{hidden}'
           f'<input type="hidden" name="deck_id" value="{escape(d["deck_id"], quote=True)}">'
           '<button>Edit flashcards</button></form>' if len(d["cards"]) <= MAX_ROWS else
           '<p><small>Large imported deck — visual editing unavailable.</small></p>')
        + '</li>' for d in latest.values())
    admin = ""
    if role == "admin":
        admin = f"""<section><h2>Teacher accounts</h2><form method="post" action="/teachers">
        {hidden}<label for="newname">Username</label><input id="newname" name="username" required maxlength="64">
        <label for="newpass">Initial password ({minimum}+ characters)</label>
        <input id="newpass" name="password" type="password" required minlength="{minimum}" maxlength="256"
        autocomplete="new-password"><button>Create teacher</button></form></section>"""
    devices = (registry_panel(assignments, csrf, enrollment, credential_storage, can_migrate) if assignments is not None else
               "<section><h2>Device and student management</h2><p>Available in isolated test mode only.</p></section>")
    discovery = scan_panel(csrf) if test_mode and role == "admin" else ""
    backup = ('<section><h2>Backup &amp; Restore</h2><p><a href="/backups">Open teaching backup center (test)</a></p><p><a href="/history">Review saved mathematics history (test)</a></p></section>' if backup_enabled else "")
    return page("Teacher dashboard", f"""
    <p>Signed in as {escape(username)} ({escape(role)})</p>
    <section><h2>Flashcard library</h2><ul>{rows or '<li>No saved decks</li>'}</ul>
    <p><a href="/decks/new" style="display:inline-block;background:#2456a6;color:white;padding:12px 18px;border-radius:6px;text-decoration:none">Create flashcards</a></p>
    <p><a href="/decks/bulk">Generate or import bulk flashcards with AI — step-by-step guide</a></p>
    <details><summary>Import cards from CSV (optional)</summary><form method="post" action="/decks">{hidden}
    <label for="title">Deck title</label><input id="title" name="title" required maxlength="120">
    <label for="csv">Cards (CSV with front,back header)</label>
    <textarea id="csv" name="csv" rows="8" required>front,back
</textarea><button>Validate and save deck</button></form></details></section>
    {'<section><h2>Curriculum library</h2><p><a href="/curriculum">Review Grade 1 draft lessons</a></p></section>' if test_mode else ""}{backup}{admin}
    {devices}{discovery}{'<p><a href="/sync/probe">Public USB synchronization test</a></p>' if probe_enabled and role == "admin" else ""}<form method="post" action="/logout">{hidden}<button>Sign out</button></form>
    """, notice)
