"""Synthetic backup center; only configured local folders, never browser paths."""
import hashlib
import hmac
import logging
import re
from html import escape
from pathlib import Path

from tools.flashcard_deck import ID
from tools.teacher_content_archive import PreviewTeacherArchive
from tools.teacher_content_files import PreviewArchiveFiles, read_archive
from tools.teacher_content_codec import decode

LOG = logging.getLogger(__name__)
NAME = re.compile(r"pocket-teaching-test-[a-f0-9]{32}\.plta\Z")
MAX_FILES = 200


class BackupCenter:
    def __init__(self, decks, directory, *, test_mode=False):
        self.files = PreviewArchiveFiles(PreviewTeacherArchive(decks, test_mode=test_mode))
        self.sessions = decks.sessions
        self.root = Path(directory).resolve(strict=True)
        workspace = Path(decks.path).resolve().parent
        if not self.root.is_dir() or self.root == workspace or workspace in self.root.parents:
            raise ValueError("Choose an existing folder outside the workspace")

    def _folder(self, token):
        owner = self.sessions.require(token)
        if not ID.fullmatch(owner):
            raise PermissionError("Invalid owner")
        folder = self.root / owner
        # The configured root is trusted by the local operator, not an HTTP field.
        if folder.is_symlink():
            raise PermissionError("Linked backup folder rejected")
        folder.mkdir(exist_ok=True)
        if not folder.is_dir() or folder.resolve() != folder:
            raise PermissionError("Invalid backup folder")
        return owner, folder

    def _selected(self, token, filename):
        owner, folder = self._folder(token)
        if not isinstance(filename, str) or not NAME.fullmatch(filename):
            raise ValueError("Select a listed archive")
        path = folder / filename
        if not path.is_file() or path.is_symlink():
            raise ValueError("Archive unavailable")
        return owner, path

    @staticmethod
    def _proof(key, token, name, digest):
        message = "\0".join(("teaching-restore-v1", token, name, digest))
        return hmac.new(key, message.encode("utf-8"), hashlib.sha256).hexdigest()

    def page(self, token, csrf, *, notice="", selected=None, digest="", proof="", summary=None):
        owner, folder = self._folder(token)
        hidden = f'<input type="hidden" name="csrf" value="{escape(csrf, quote=True)}">'
        rows = []
        # Bound history rendering. Do not report an unverified file as healthy.
        for path in sorted(folder.glob("pocket-teaching-test-*.plta"))[:MAX_FILES]:
            if not NAME.fullmatch(path.name) or path.is_symlink():
                continue
            try:
                raw = read_archive(path)
                if decode(raw)["owner"] != owner:
                    continue
                label = f"{len(decode(raw)['decks'])} deck versions · {len(raw)} bytes"
                control = (f'<form method="post" action="/backups">{hidden}'
                           f'<input type="hidden" name="filename" value="{escape(path.name)}">'
                           '<button name="action" value="preview">Preview restore</button></form>')
            except (OSError, ValueError):
                label, control = "Unreadable or damaged archive", ""
            rows.append(f'<li>{escape(path.name)} — {label}{control}</li>')
        review = ""
        if summary is not None:
            review = (f'<section><h2>Review restore</h2><p>{escape(selected)}</p>'
                      f'<p>{summary.existing} existing versions; {summary.new} versions to add. '
                      'Newer versions will not be deleted.</p>'
                      f'<form method="post" action="/backups">{hidden}'
                      f'<input type="hidden" name="filename" value="{escape(selected)}">'
                      f'<input type="hidden" name="digest" value="{escape(digest)}">'
                      f'<input type="hidden" name="proof" value="{escape(proof)}">'
                      '<button name="action" value="restore">Confirm restore</button></form></section>')
        return (f'<p class="notice">TEST MODE — synthetic data only. Legacy .plta archives are NOT encrypted.</p>'
                f'<p role="status">{escape(notice)}</p>'
                '<section><h2>Teaching materials</h2><p>Includes all versions of your saved text flashcards.</p>'
                '<p><a href="/backups/protected">Create or restore encrypted backups (test)</a></p>'
                f'<p>Configured location: {escape(str(folder))}</p>'
                f'<form method="post" action="/backups">{hidden}'
                '<button name="action" value="save">Back up my flashcards</button></form></section>'
                f'{review}<section><h2>Archive history (up to {MAX_FILES})</h2>'
                f'<ul>{"".join(rows) or "<li>No readable archives found</li>"}</ul></section>'
                '<section><h2>Student progress</h2><p>Not yet available. Authenticated USB '
                'and protected student archives must pass verification first.</p></section>'
                '<p><a href="/">Return to dashboard</a></p>')

    def handle(self, token, csrf, key, data):
        try:
            action = data.get("action")
            if action == "save" and set(data) == {"csrf", "action"}:
                _, folder = self._folder(token)
                receipt = self.files.save(token, folder)
                return self.page(token, csrf, notice=f"Verified backup saved: {receipt.path.name}")
            if action == "preview" and set(data) == {"csrf", "action", "filename"}:
                _, path = self._selected(token, data["filename"])
                preview = self.files.preview(token, path)
                proof = self._proof(key, token, path.name, preview.sha256)
                return self.page(token, csrf, selected=path.name, digest=preview.sha256,
                                 proof=proof, summary=preview.summary)
            if action == "restore" and set(data) == {"csrf", "action", "filename", "digest", "proof"}:
                _, path = self._selected(token, data["filename"])
                if not re.fullmatch(r"[a-f0-9]{64}", data["digest"]) or not re.fullmatch(r"[a-f0-9]{64}", data["proof"]):
                    raise ValueError("Invalid restore proof")
                expected = self._proof(key, token, path.name, data["digest"])
                if not hmac.compare_digest(expected, data["proof"]):
                    raise PermissionError("Restore preview required")
                result = self.files.restore(token, path, expected_sha256=data["digest"], confirmed=True)
                return self.page(token, csrf, notice=f"Restore verified: {result.new} versions added; {result.existing} already present.")
            raise ValueError("Unknown backup action")
        except PermissionError:
            raise
        except (OSError, ValueError) as exc:
            LOG.warning("Teaching backup operation rejected: %s", type(exc).__name__)
            return self.page(token, csrf, notice="Backup or restore failed. Check the archive, storage and conflicts; review again.")
