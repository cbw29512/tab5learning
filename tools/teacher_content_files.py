"""Local synthetic-only independent teaching archive files; no web paths."""
import hashlib
import hmac
import logging
import os
import stat
import tempfile
from dataclasses import dataclass
from pathlib import Path
from uuid import uuid4

from tools.teacher_content_codec import decode, MAX_BYTES
from tools.teacher_content_archive import PreviewTeacherArchive, RestoreSummary

LOG = logging.getLogger(__name__)


@dataclass(frozen=True)
class FileReceipt:
    path: Path
    sha256: str
    bytes_written: int
    versions: int


@dataclass(frozen=True)
class RestorePreview:
    sha256: str
    summary: RestoreSummary


def read_archive(path):
    """Read a bounded regular file, never follow archive-embedded paths."""
    try:
        path = Path(path)
        if not path.is_file() or path.is_symlink():
            raise ValueError("Select a regular archive file")
        with path.open("rb") as stream:
            info = os.fstat(stream.fileno())
            if not stat.S_ISREG(info.st_mode) or info.st_size > MAX_BYTES:
                raise ValueError("Invalid archive file size or type")
            raw = stream.read(MAX_BYTES + 1)
        decode(raw)
        return raw
    except (OSError, ValueError, TypeError):
        LOG.warning("Teaching archive file read failed")
        raise


def _publish_new(staged, final):
    # Neither path is supplied by an archive. Never replace any prior backup.
    # Windows rename rejects existing destinations; POSIX rename does not.
    if os.name == "nt":
        os.rename(staged, final)
    else:
        os.link(staged, final)
        staged.unlink()


def _sync_directory(directory):
    if os.name != "nt":
        # Flush the directory entry where the filesystem supports it.
        fd = os.open(directory, os.O_RDONLY | os.O_DIRECTORY)
        try:
            os.fsync(fd)
        finally:
            os.close(fd)


class PreviewArchiveFiles:
    """Trusted local caller selects an existing directory outside the workspace."""
    def __init__(self, archive):
        if not isinstance(archive, PreviewTeacherArchive):
            raise PermissionError("Synthetic archive service required")
        self.archive = archive

    def save(self, token, directory):
        staged = None
        try:
            raw = self.archive.export(token)
            parsed = decode(raw)
            root = Path(directory).resolve(strict=True)
            workspace = Path(self.archive.decks.path).resolve().parent
            if not root.is_dir() or root == workspace or workspace in root.parents:
                raise ValueError("Choose a directory outside the teacher workspace")
            final = root / f"pocket-teaching-test-{uuid4().hex}.plta"
            # Same-directory staging avoids a cross-filesystem publication.
            with tempfile.NamedTemporaryFile(dir=root, prefix=".pocket-",
                                             suffix=".tmp", delete=False) as stream:
                staged = Path(stream.name)
                if stream.write(raw) != len(raw):
                    raise OSError("Incomplete archive write")
                stream.flush()
                os.fsync(stream.fileno())
            if read_archive(staged) != raw:
                raise ValueError("Staged archive readback mismatch")
            _publish_new(staged, final)
            _sync_directory(root)
            if read_archive(final) != raw:
                raise ValueError("Published archive readback mismatch")
            return FileReceipt(final, hashlib.sha256(raw).hexdigest(),
                               len(raw), len(parsed["decks"]))
        except (OSError, ValueError, PermissionError, TypeError):
            # A published file with an uncertain result is retained for review.
            # It never replaces a previous archive.
            LOG.warning("Independent teaching archive save failed")
            raise
        finally:
            if staged is not None:
                try:
                    staged.unlink(missing_ok=True)
                except OSError:
                    LOG.warning("Temporary archive cleanup failed")

    def preview(self, token, path):
        self.archive.decks.sessions.require(token)
        raw = read_archive(path)
        return RestorePreview(hashlib.sha256(raw).hexdigest(),
                              self.archive.preview(token, raw))

    def restore(self, token, path, *, expected_sha256, confirmed=False):
        try:
            self.archive.decks.sessions.require(token)
            if confirmed is not True:
                raise PermissionError("Explicit restore confirmation required")
            raw = read_archive(path)
            digest = hashlib.sha256(raw).hexdigest()
            if not isinstance(expected_sha256, str) or not hmac.compare_digest(
                    digest, expected_sha256):
                raise ValueError("Archive changed since preview; review again")
            # Restore the exact verified bytes, never reopen a mutable path.
            return self.archive.restore(token, raw, confirmed=True)
        except (OSError, ValueError, PermissionError, TypeError):
            LOG.warning("Independent teaching archive restore rejected")
            raise
