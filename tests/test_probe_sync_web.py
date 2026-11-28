"""Browser admission and verified receipt tests with a fake public probe."""
import struct
import unittest
from pathlib import Path
from unittest.mock import patch

import test_teacher_web as harness
from test_math_backup_archive import snapshot, crc, checkpoint
from test_protected_snapshot_transport import Peer
from tools.math_backup_codec import MathBackup, encode
from tools.teacher_web import TeacherWeb
from tools.teacher_usb_discovery import UsbSnapshot


def public_sample():
    base = checkpoint(0)
    first_after = checkpoint(1)
    final = crc(b"PL\x01" + bytes((1,0,0,0,0)) + struct.pack("<I",2))
    first = crc(b"PA\x01" + bytes((0,0,1,1,0)) + first_after + b"\0\0\0\0\1")
    second = crc(b"PA\x01" + bytes((0,1,2,0,0)) + final)
    return encode(MathBackup(base, final, (first,second)))


class ProbeSyncWebTests(unittest.TestCase):
    def setUp(self):
        self.web = harness.TeacherWebTests()
        self.web.setUp()
        self.addCleanup(self.web.doCleanups)
        self.root = Path(self.web.temp.name)
        self.backup = self.root / "external"
        self.backup.mkdir()
        self.web.app = TeacherWeb(self.root / "probe", test_mode=True,
                                  backup_dir=self.backup, usb_probe=True)
        self.web.app.accounts.create("admin","1234","admin",bootstrap=True)
        self.csrf = self.web.login("admin","1234")
        self.token = self.web.cookie.split("=",1)[1]
        self.route = "/sync/probe"

    def assign(self):
        self.assertEqual(self.web.call("/assignments",dict(csrf=self.csrf,
            student_id="probe-student",device_id="probe-only",grade="1"))[0],303)

    def post(self, **args):
        return self.web.call(self.route,dict(csrf=self.csrf,**args))

    def test_admission_and_success(self):
        self.assertIn("Create the reserved",self.web.call(self.route)[2])
        with patch("tools.probe_sync_center.open_probe_stream") as op:
            self.assertEqual(self.post(action="receive",port="COM5",confirm="yes")[0],403)
            op.assert_not_called()
        self.assign()
        with patch("tools.probe_sync_center.scan_usb",return_value=UsbSnapshot("detected",("COM5",))):
            self.assertIn("COM5",self.post(action="scan")[2])
        peer = Peer()
        peer.raw = public_sample()
        with patch("tools.probe_sync_center.open_probe_stream",return_value=peer) as op:
            self.assertEqual(self.post(action="receive",port="COM5",confirm="no")[0],403)
            op.assert_not_called()
            result = self.post(action="receive",port="COM5",confirm="yes")
            self.assertEqual(result[0],200)
            self.assertIn("Verified sample saved",result[2])
            self.assertTrue(peer.closed)
        self.assertEqual(self.web.app.math_archive.load(self.token,"probe-only","probe-student"),peer.raw)
        again = Peer()
        again.raw = public_sample()
        with patch("tools.probe_sync_center.open_probe_stream",return_value=again):
            self.assertIn("Exact retry",self.post(action="receive",port="COM5",confirm="yes")[2])

    def test_not_public_fixture_or_cross_origin(self):
        self.assign()
        peer = Peer()
        with patch("tools.probe_sync_center.open_probe_stream",return_value=peer) as op:
            self.assertEqual(self.web.call(self.route,dict(csrf=self.csrf,action="receive",
                port="COM5",confirm="yes"),HTTP_ORIGIN="https://evil.example")[0],403)
            op.assert_not_called()
            self.assertIn("Synchronization failed",self.post(action="receive",port="COM5",confirm="yes")[2])
            self.assertTrue(peer.closed)
        with self.assertRaises(ValueError):
            self.web.app.math_archive.load(self.token,"probe-only","probe-student")

    def test_disabled_and_teacher_denied(self):
        with self.assertRaises(PermissionError):
            TeacherWeb(self.root/"normal",usb_probe=True)
        self.web.app.probe_sync = None
        self.assertEqual(self.web.call(self.route)[0],403)
        self.assertEqual(self.post(action="scan")[0],403)
