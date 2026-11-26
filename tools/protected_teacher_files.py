"""Encrypted teaching archive file service, synthetic-only local callers."""
import hashlib
import hmac
import logging

from tools.teacher_content_files import PreviewArchiveFiles, RestorePreview, read_bounded
from tools.teacher_content_codec import decode
from tools.protected_teacher_archive import protect, unprotect, MAX_PROTECTED

LOG = logging.getLogger(__name__)


class ProtectedPreviewArchiveFiles(PreviewArchiveFiles):
    def save(self, token, directory, *, recovery_key):
        try:
            plaintext = self.archive.export(token)
            versions = len(decode(plaintext)["decks"])
            encrypted = protect(plaintext, recovery_key)

            def verify(path):
                raw = read_bounded(path, MAX_PROTECTED)
                if unprotect(raw, recovery_key) != plaintext:
                    raise ValueError("Protected archive readback mismatch")
                return raw

            # Only encrypted bytes enter the file publication path.
            return self._save_bytes(directory, encrypted, versions, ".plte", verify)
        except (OSError, ValueError, ImportError, PermissionError):
            LOG.warning("Protected teaching backup rejected")
            raise

    def preview(self, token, path, *, recovery_key):
        self.archive.decks.sessions.require(token)
        raw = read_bounded(path, MAX_PROTECTED)
        return RestorePreview(hashlib.sha256(raw).hexdigest(),
                              self.archive.preview(token, unprotect(raw, recovery_key)))

    def restore(self, token, path, *, recovery_key, expected_sha256, confirmed=False):
        try:
            self.archive.decks.sessions.require(token)
            if confirmed is not True:
                raise PermissionError("Explicit restore confirmation required")
            raw = read_bounded(path, MAX_PROTECTED)
            digest = hashlib.sha256(raw).hexdigest()
            if not isinstance(expected_sha256, str) or not hmac.compare_digest(
                    digest, expected_sha256):
                raise ValueError("Protected archive changed since preview")
            return self.archive.restore(token, unprotect(raw, recovery_key), confirmed=True)
        except (OSError, ValueError, ImportError, PermissionError):
            LOG.warning("Protected teaching restore rejected")
            raise
