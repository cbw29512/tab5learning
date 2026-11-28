"""Independent encrypted synthetic mathematics recovery files."""
import hashlib
import hmac
import logging
from dataclasses import dataclass
from pathlib import Path

from tools.math_backup_archive import PreviewMathArchive
from tools.math_backup_codec import decode
from tools.teacher_registry import normalize
from tools.teacher_content_files import read_bounded, publish_archive
from tools.protected_math_codec import StudentMathSnapshot, protect, unprotect, MAX_PROTECTED

LOG = logging.getLogger(__name__)


@dataclass(frozen=True)
class MathFileReceipt:
    path: Path
    sha256: str
    records: int


@dataclass(frozen=True)
class MathRestorePreview:
    sha256: str
    records_to_add: int
    already_present: bool


class ProtectedMathFiles:
    def __init__(self, archive):
        if not isinstance(archive, PreviewMathArchive):
            raise PermissionError("Synthetic archive required")
        self.archive = archive

    def save(self, token, device, student, directory, *, recovery_key):
        try:
            student, device = normalize(student), normalize(device)
            snapshot = StudentMathSnapshot(student, device,
                                           self.archive.load(token, device, student))
            encrypted = protect(snapshot, recovery_key)

            def verify(path):
                raw = read_bounded(path, MAX_PROTECTED)
                if unprotect(raw, recovery_key) != snapshot:
                    raise ValueError("Student archive readback mismatch")
                return raw

            path = publish_archive(directory, encrypted,
                workspace=Path(self.archive.path).resolve().parent,
                prefix="pocket-student-test-", suffix=".plse", verify=verify)
            return MathFileReceipt(path, hashlib.sha256(encrypted).hexdigest(),
                                   len(decode(snapshot.history).records))
        except (ValueError, PermissionError, OSError, ImportError):
            LOG.warning("Protected mathematics backup rejected")
            raise

    def _read(self, token, device, student, path, recovery_key):
        _, device, student = self.archive._scope(token, device, student)
        raw = read_bounded(path, MAX_PROTECTED)
        snapshot = unprotect(raw, recovery_key)
        if (snapshot.student, snapshot.device) != (student, device):
            raise PermissionError("Student archive identity mismatch")
        return raw, snapshot

    def preview(self, token, device, student, path, *, recovery_key):
        raw, snapshot = self._read(token, device, student, path, recovery_key)
        count, present = self.archive.preview_restore(token, snapshot.device,
                                                       snapshot.student, snapshot.history)
        return MathRestorePreview(hashlib.sha256(raw).hexdigest(), count, present)

    def restore(self, token, device, student, path, *, recovery_key,
                expected_sha256, confirmed=False):
        try:
            if confirmed is not True:
                raise PermissionError("Explicit restore confirmation required")
            raw, snapshot = self._read(token, device, student, path, recovery_key)
            if not isinstance(expected_sha256, str) or not hmac.compare_digest(
                    hashlib.sha256(raw).hexdigest(), expected_sha256):
                raise ValueError("Archive changed since preview")
            # Existing archive transaction checks scope, history and readback.
            return self.archive.save(token, snapshot.device, snapshot.student, snapshot.history)
        except (ValueError, PermissionError, OSError, ImportError):
            LOG.warning("Protected mathematics restore rejected")
            raise
