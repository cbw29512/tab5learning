"""Server-rendered flashcard editor markup."""
from html import escape

MAX_ROWS = 25

def editor(draft, csrf, *, preview=None, error=""):
    """All user-controlled strings escaped; no scripts or external resources."""
    def attr(s):
        return escape(str(s), quote=True)
    hidden = "".join(f'<input type="hidden" name="{name}" value="{attr(value)}">'
                     for name, value in (("csrf", csrf), ("deck_id", draft.deck_id),
                                         ("count", len(draft.rows))))
    if draft.base_version:
        hidden += f'<input type="hidden" name="base_version" value="{draft.base_version}">'
    rows = []
    for i, (front, back) in enumerate(draft.rows):
        remove = (f'<button name="action" value="remove:{i}" type="submit" '
                  f'aria-label="Remove card {i+1}">Remove card {i+1}</button>'
                  if len(draft.rows) > 1 else "")
        rows.append(f'<fieldset style="margin:16px 0;border:1px solid #dce2ea;border-radius:8px;padding:12px">'
                    f'<legend>Card {i+1}</legend><label for="front_{i}">Question / front</label>'
                    f'<textarea id="front_{i}" name="front_{i}" rows="2" maxlength="500">{escape(front)}</textarea>'
                    f'<label for="back_{i}">Answer / back</label>'
                    f'<textarea id="back_{i}" name="back_{i}" rows="2" maxlength="500">{escape(back)}</textarea>'
                    f'{remove}</fieldset>')
    preview_html = ""
    if preview is not None:
        items = "".join(f'<li><strong>{escape(c["front"])}</strong><p>{escape(c["back"])}</p></li>'
                        for c in preview["cards"])
        preview_html = (f'<section><h2>Preview — {len(preview["cards"])} cards</h2>'
                        f'<p>{escape(preview["title"])}</p><ol>{items}</ol>'
                        '<p>Saved decks are not yet assigned to a student.</p></section>')
    add = ('<button name="action" value="add">Add another card</button>'
           if len(draft.rows) < MAX_ROWS else '<p>25-card editor limit reached. CSV supports larger decks.</p>')
    save_label = "Save new version" if draft.base_version else "Save deck"
    save = f'<button name="action" value="save">{save_label}</button>' if preview is not None else ""
    notice = f'<p role="alert" class="notice">{escape(error)}</p>' if error else ""
    context = (f"<p>Editing version {draft.base_version}. Earlier versions remain saved.</p>"
               if draft.base_version else "")
    return (f'{notice}{preview_html}<section>{context}<p>Type a question and its answer. Empty cards are ignored.</p>'
            f'<form method="post" action="/decks/new">{hidden}'
            f'<label for="deck-title">Deck title</label><input id="deck-title" name="title" '
            f'maxlength="120" value="{attr(draft.title)}">{"".join(rows)}'
            f'{add}<button name="action" value="preview">Preview cards</button>{save}'
            '</form><p><a href="/">Return to dashboard (unsaved work will be lost)</a></p></section>')


