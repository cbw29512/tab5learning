import hashlib
import unittest
from unittest.mock import patch

from tools.run_protected_usb_probe import verify_fixture, run_probe
from tools.teacher_usb_discovery import UsbSnapshot
from tools.math_backup_codec import decode, encode, MathBackup
from test_math_backup_archive import snapshot


class ProbeCliTests(unittest.TestCase):
    def test_nonfixture_rejected(self):
        with self.assertRaises(ValueError):
            verify_fixture(snapshot(3))
        with self.assertRaises(ValueError):
            verify_fixture(snapshot(2))

    def test_port_rejected_before_open(self):
        with self.assertRaises(ValueError):
            run_probe("COM5",scan=lambda:UsbSnapshot("empty"),
                      serial_factory=lambda **kw:self.fail("Unexpected open"))
        with self.assertRaises(ValueError):
            run_probe("invalid",scan=lambda:self.fail("Unexpected scan"))

    def test_connection_closed_after_failed_exchange(self):
        class Connection:
            closed=False
            def open(self):pass
            def reset_input_buffer(self):pass
            def close(self):self.closed=True
        peer=Connection()
        with patch("tools.run_protected_usb_probe.exchange_synthetic_snapshot",
                   side_effect=OSError("synthetic failure")):
            with self.assertRaises(OSError):
                run_probe("COM5",scan=lambda:UsbSnapshot("detected",("COM5",)),
                          serial_factory=lambda **kw:peer)
        self.assertTrue(peer.closed)
        self.assertFalse(peer.dtr)
        self.assertFalse(peer.rts)
