import unittest
from types import SimpleNamespace
from tools.teacher_usb_discovery import scan_usb, scan_result


def port(name, vid=0x303A, pid=0x1001):
    return SimpleNamespace(device=name, vid=vid, pid=pid)


class UsbDiscoveryTests(unittest.TestCase):
    def test_filters_deduplicates_and_sorts(self):
        result = scan_usb(lambda: [port("COM10"), port("com5"), port("COM5"),
                                  port("COM7", 1, 2), port("bad<script>")])
        self.assertEqual(result.status, "detected")
        self.assertEqual(result.ports, ("COM5", "COM10"))
        self.assertIn("USB identity is not authenticated", scan_result(result))

    def test_empty_and_bounded_scan(self):
        self.assertEqual(scan_usb(lambda: []).status, "empty")
        result = scan_usb(lambda: [port(f"COM{i}") for i in range(1, 100)])
        self.assertEqual(len(result.ports), 32)
        self.assertTrue(result.limited)
        result = scan_usb(lambda: [port("COM1", 1, 1)] * 257)
        self.assertTrue(result.limited)

    def test_dependency_and_error(self):
        def missing():
            raise ImportError("pyserial unavailable")
        def failure():
            raise OSError("enumeration failed")
        self.assertEqual(scan_usb(missing).status, "dependency")
        self.assertEqual(scan_usb(failure).status, "error")
