"""Protected synthetic backup UI; keys are never persisted by the server."""
import hashlib
import hmac
import logging
import re
from html import escape
from itertools import islice

from tools.teacher_backup_center import BackupCenter, MAX_FILES
from tools.protected_teacher_archive import new_recovery_key, MAX_PROTECTED
from tools.protected_teacher_files import ProtectedPreviewArchiveFiles

LOG = logging.getLogger(__name__)
NAME = re.compile(r"pocket-teaching-test-[a-f0-9]{32}\.plte\Z")
HEX = re.compile(r"[a-f0-9]{64}\Z")
ROUTE = "/backups/protected"


def hidden(name, value):
    return f'<input type="hidden" name="{name}" value="{escape(value, quote=True)}">'


def key_field(name="recovery_key", label="Recovery key"):
    return (f'<label>{label}<input type="password" name="{name}" '
            'required minlength="64" maxlength="64" autocomplete="off" spellcheck="false"></label>')


class ProtectedBackupCenter(BackupCenter):
    def __init__(self, decks, directory, *, test_mode=False):
        super().__init__(decks, directory, test_mode=test_mode)
        self.files = ProtectedPreviewArchiveFiles(self.files.archive)

    def _selected(self, token, filename):
        owner, folder = self._folder(token)
        if not isinstance(filename, str) or not NAME.fullmatch(filename):
            raise ValueError("Select a protected archive")
        path = folder / filename
        if not path.is_file() or path.is_symlink():
            raise ValueError("Protected archive unavailable")
        return owner, path

    @staticmethod
    def _bound(key, token, purpose, *values):
        message = "\0".join(("protected-backup-v1", token, purpose, *values))
        return hmac.new(key, message.encode("utf-8"), hashlib.sha256).hexdigest()

    def page(self, token, csrf, *, notice="", generated=None, proof="", selected=None,
             digest="", summary=None):
        _, folder = self._folder(token)
        guard = hidden("csrf", csrf)
        rows = []
        for path in islice(folder.glob("pocket-teaching-test-*.plte"), MAX_FILES):
            if not NAME.fullmatch(path.name) or path.is_symlink():
                continue
            try:
                if not path.is_file() or not 0 < path.stat().st_size <= MAX_PROTECTED:
                    continue
            except OSError:
                continue
            rows.append(f'<li>{escape(path.name)} — locked; integrity not yet checked'
                        f'<form method="post" action="{ROUTE}">{guard}'
                        f'{hidden("filename", path.name)}{key_field()}'
                        '<button name="action" value="preview">Unlock and preview restore</button></form></li>')
        new = ""
        if generated is not None:
            new = (f'<section><h2>Save your recovery key separately</h2><p><code>{escape(generated)}</code></p>'
                   '<p>This key is shown only in this response. Store it securely away from the backup. '
                   'If lost, the archive cannot be recovered. Do not use real records in this preview.</p>'
                   f'<form method="post" action="{ROUTE}">{guard}{hidden("proof", proof)}'
                   f'{key_field()}{key_field("repeat_key", "Re-enter saved key")}'
                   '<label><input type="checkbox" name="saved_key" value="yes" required> '
                   'I have retained the key separately.</label>'
                   '<button name="action" value="save">Create encrypted backup</button></form></section>')
        review = ""
        if summary is not None:
            review = (f'<section><h2>Review protected restore</h2><p>{escape(selected)}</p>'
                      f'<p>{summary.existing} existing versions; {summary.new} versions to add.</p>'
                      f'<form method="post" action="{ROUTE}">{guard}{hidden("filename", selected)}'
                      f'{hidden("digest", digest)}{hidden("proof", proof)}{key_field()}'
                      '<button name="action" value="restore">Confirm protected restore</button></form></section>')
        return ('<p class="notice">TEST MODE — synthetic flashcards only. Not approved for student records.</p>'
                f'<p role="status">{escape(notice)}</p>'
                f'<section><h2>Encrypted teaching backup</h2><p>Creates a new archive with a separate recovery key.</p>'
                f'<form method="post" action="{ROUTE}">{guard}'
                '<button name="action" value="generate">Generate recovery key</button></form></section>'
                f'{new}{review}<section><h2>Protected archive history (up to {MAX_FILES})</h2>'
                f'<ul>{"".join(rows) or "<li>No protected archives found</li>"}</ul></section>'
                '<p><a href="/backups">Return to backup center</a></p>')

    def handle(self, token, csrf, key, data):
        try:
            action = data.get("action")
            fields = set(data)
            if action == "generate" and fields == {"csrf", "action"}:
                value = new_recovery_key()
                return self.page(token, csrf, generated=value,
                                 proof=self._bound(key, token, "generate", value))
            if action == "save" and fields == {"csrf", "action", "proof", "recovery_key", "repeat_key", "saved_key"}:
                value = data["recovery_key"]
                if not HEX.fullmatch(value) or not HEX.fullmatch(data["repeat_key"]) or not HEX.fullmatch(data["proof"]):
                    raise ValueError("Invalid recovery key or proof")
                if data["saved_key"] != "yes" or not hmac.compare_digest(value, data["repeat_key"]):
                    raise ValueError("Retain and confirm recovery key")
                if not hmac.compare_digest(self._bound(key, token, "generate", value), data["proof"]):
                    raise PermissionError("Generate a recovery key in this session")
                _, folder = self._folder(token)
                receipt = self.files.save(token, folder, recovery_key=value)
                return self.page(token, csrf, notice=f"Encrypted backup verified: {receipt.path.name}")
            if action == "preview" and fields == {"csrf", "action", "filename", "recovery_key"}:
                _, path = self._selected(token, data["filename"])
                preview = self.files.preview(token, path, recovery_key=data["recovery_key"])
                proof = self._bound(key, token, "restore", path.name, preview.sha256)
                return self.page(token, csrf, selected=path.name, digest=preview.sha256,
                                 proof=proof, summary=preview.summary)
            if action == "restore" and fields == {"csrf", "action", "filename", "digest", "proof", "recovery_key"}:
                _, path = self._selected(token, data["filename"])
                if not HEX.fullmatch(data["digest"]) or not HEX.fullmatch(data["proof"]):
                    raise ValueError("Invalid restore proof")
                expected = self._bound(key, token, "restore", path.name, data["digest"])
                if not hmac.compare_digest(expected, data["proof"]):
                    raise PermissionError("Protected restore preview required")
                result = self.files.restore(token, path, recovery_key=data["recovery_key"],
                    expected_sha256=data["digest"], confirmed=True)
                return self.page(token, csrf, notice=f"Protected restore verified: {result.new} added; {result.existing} existing.")
            raise ValueError("Invalid protected backup action")
        except PermissionError:
            raise
        except (OSError, ValueError, ImportError):
            LOG.warning("Protected backup operation failed")
            return self.page(token, csrf, notice="Protected backup or restore failed. Check the key, archive and storage.")
