"""Test-only bridge from verified public USB probe to a scoped durable archive.

A public probe must NEVER impersonate an enrolled learner. Its only permitted
destination is the reserved synthetic assignment below. No serial discovery,
real student import, production credentials or device writes are enabled.
"""
import hashlib
import logging
from dataclasses import dataclass

from tools.math_backup_archive import PreviewMathArchive
from tools.math_backup_codec import decode
from tools.protected_snapshot_transport import exchange_synthetic_snapshot
from tools.run_protected_usb_probe import verify_fixture

LOG = logging.getLogger(__name__)
PROBE_DEVICE = "probe-only"
PROBE_STUDENT = "probe-student"


@dataclass(frozen=True)
class PreviewSyncReceipt:
    records: int
    sha256: str
    exact_retry: bool


class PreviewUsbSync:
    def __init__(self, archive, *, test_mode=False, public_fixture_only=False):
        if test_mode is not True or not isinstance(archive, PreviewMathArchive):
            raise PermissionError("Synthetic USB archive only")
        self.archive = archive
        self.public_fixture_only = public_fixture_only

    def _scope(self, token):
        # Reuse the archive's teacher/session/assignment checks. The reserved
        # identity cannot be selected through a caller-supplied student ID.
        return self.archive._scope(token, PROBE_DEVICE, PROBE_STUDENT)

    def receive(self, token, connection, *, confirmed=False):
        """Own the stream; commit only after complete verification and recheck."""
        handed_off = False
        try:
            if confirmed is not True:
                raise PermissionError("Explicit synthetic transfer approval required")
            actor = self._scope(token)
            handed_off = True
            raw = exchange_synthetic_snapshot(
                connection, PROBE_DEVICE, bytes(range(32)), test_mode=True)
            if self.public_fixture_only:
                verify_fixture(raw)
            candidate = decode(raw)
            digest = hashlib.sha256(raw).hexdigest()
            # A teacher may have logged out while USB was transferring.
            if self._scope(token) != actor:
                raise PermissionError("Teacher assignment changed during transfer")
            _, retry = self.archive.preview_restore(
                token, PROBE_DEVICE, PROBE_STUDENT, raw)
            saved = self.archive.save(token, PROBE_DEVICE, PROBE_STUDENT, raw)
            if saved != digest:
                raise ValueError("Unexpected durable receipt")
            return PreviewSyncReceipt(len(candidate.records), digest, retry)
        except Exception:
            LOG.warning("Synthetic USB synchronization rejected or failed")
            raise
        finally:
            # The exchange owns and closes its stream once called; close on
            # admission rejection too. Never log the credential or payload.
            if not handed_off:
                try:
                    connection.close()
                except Exception:
                    LOG.warning("Rejected USB connection close failed")
