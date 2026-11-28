import hashlib
import unittest
from unittest.mock import patch
from pathlib import Path
import tempfile

from tools.math_backup_archive import PreviewMathArchive
from tools.preview_usb_sync import PreviewUsbSync, PROBE_DEVICE, PROBE_STUDENT
from tools.teacher_registry import TeacherRegistry
from unittest.mock import Mock
from test_protected_snapshot_transport import Peer
from test_math_backup_archive import snapshot


class PreviewUsbSyncTests(unittest.TestCase):
    def setUp(self):
        folder = tempfile.TemporaryDirectory()
        self.addCleanup(folder.cleanup)
        self.sessions = Mock()
        self.sessions.require.return_value = "teacher1"
        self.sessions.accounts.role.return_value = "teacher"
        self.registry = TeacherRegistry(Path(folder.name) / "registry.sqlite", self.sessions)
        self.registry.create("token", PROBE_STUDENT, PROBE_DEVICE, 1)
        self.registry.create("token", "student-001", "tab5-001", 1)
        self.archive = PreviewMathArchive(
            Path(folder.name) / "math.sqlite", self.sessions, self.registry, test_mode=True)
        self.sync = PreviewUsbSync(self.archive, test_mode=True)

    def load(self):
        return self.archive.load("token", PROBE_DEVICE, PROBE_STUDENT)

    def test_commit_retry_extension(self):
        first = Peer()
        result = self.sync.receive("token", first, confirmed=True)
        self.assertTrue(first.closed)
        self.assertEqual(result.records, 3)
        self.assertEqual(result.sha256, hashlib.sha256(first.raw).hexdigest())
        self.assertFalse(result.exact_retry)
        self.assertEqual(self.load(), first.raw)
        again = Peer()
        self.assertTrue(self.sync.receive("token", again, confirmed=True).exact_retry)
        extension = Peer()
        extension.raw = snapshot(100)
        self.assertFalse(self.sync.receive("token", extension, confirmed=True).exact_retry)
        self.assertEqual(self.load(), extension.raw)
        with self.assertRaises(ValueError):
            self.archive.load("token", "tab5-001", "student-001")

    def test_denial_before_usb(self):
        with self.assertRaises(PermissionError):
            PreviewUsbSync(self.archive)
        peer = Peer()
        with self.assertRaises(PermissionError):
            self.sync.receive("token", peer)
        self.assertTrue(peer.closed)
        self.assertEqual(peer.writes, 0)
        self.sessions.require.return_value = "other"
        peer = Peer()
        with self.assertRaises(PermissionError):
            self.sync.receive("token", peer, confirmed=True)
        self.assertEqual(peer.writes, 0)
        self.assertTrue(peer.closed)

    def test_incomplete_or_conflicting_transfer_preserves_prior(self):
        initial = Peer()
        self.sync.receive("token", initial, confirmed=True)
        for mode in ("tamper", "missing_end", "disconnect"):
            peer = Peer(mode)
            with self.assertRaises((OSError, ValueError, PermissionError)):
                self.sync.receive("token", peer, confirmed=True)
            self.assertTrue(peer.closed)
            self.assertEqual(self.load(), initial.raw)
        peer = Peer()
        peer.raw = snapshot(4, alternate=True)
        with self.assertRaises(ValueError):
            self.sync.receive("token", peer, confirmed=True)
        self.assertEqual(self.load(), initial.raw)

    def test_logout_during_transfer_no_commit(self):
        peer = Peer()
        def expired(connection, *args, **kwargs):
            connection.close()
            self.sessions.require.side_effect = PermissionError("Expired")
            return peer.raw
        with patch("tools.preview_usb_sync.exchange_synthetic_snapshot", side_effect=expired):
            with self.assertRaises(PermissionError):
                self.sync.receive("token", peer, confirmed=True)
        self.sessions.require.side_effect = None
        self.assertTrue(peer.closed)
        with self.assertRaises(ValueError):
            self.load()


if __name__ == "__main__":
    unittest.main()
