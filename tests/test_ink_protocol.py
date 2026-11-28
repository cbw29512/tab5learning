"""Host-only tests; no Tab5 hardware or serial adapter needed."""
import tempfile
from pathlib import Path
import unittest

from tools.ink_protocol import parse_export
from tools.usb_probe import ping, request_export, write_capture


class FakeSerial:
    def __init__(self, lines):
        self.lines = iter(lines)
        self.sent = []

    def write(self, message):
        self.sent.append(message)

    def flush(self):
        pass

    def readline(self):
        return next(self.lines, b"")


class InkProtocolTests(unittest.TestCase):
    def test_valid_two_stroke_export(self):
        lines = ["BEGIN,INK_V1,3,2", "PT,0,10,20,1", "PT,1,11,21,0",
                 "PT,2,15,30,1", "END,OK"]
        report = parse_export(lines)
        self.assertEqual((len(report.points), report.strokes), (3, 2))
        self.assertFalse(report.truncated)

    def test_empty_export(self):
        self.assertEqual(parse_export(["BEGIN,INK_V1,0,0", "END,OK"]).points, ())

    def test_wrong_index_rejected(self):
        with self.assertRaises(ValueError):
            parse_export(["BEGIN,INK_V1,1,1", "PT,5,10,20,1", "END,OK"])

    def test_stroke_mismatch_rejected(self):
        with self.assertRaises(ValueError):
            parse_export(["BEGIN,INK_V1,1,0", "PT,0,10,20,1", "END,OK"])

    def test_oversized_export_rejected(self):
        with self.assertRaises(ValueError):
            parse_export(["BEGIN,INK_V1,513,1", "END,OK"])

    def test_missing_footer_rejected(self):
        with self.assertRaises(ValueError):
            parse_export(["BEGIN,INK_V1,1,1", "PT,0,10,20,1"])

    def test_handshake_and_export(self):
        device = FakeSerial([b"PONG,TAB5_INK_PROBE,1\n", b"BEGIN,INK_V1,1,1\n",
                             b"PT,0,25,42,1\n", b"END,OK\n"])
        ping(device)
        report = request_export(device)
        self.assertEqual(device.sent, [b"PING\n", b"EXPORT\n"])
        self.assertEqual(report.points[0].x, 25)
        with tempfile.TemporaryDirectory() as folder:
            output = Path(folder) / "capture.json"
            write_capture(report, output)
            self.assertIn('"schema": "TAB5_INK_V1"', output.read_text())

    def test_invalid_declared_count_fails_early(self):
        device = FakeSerial([b"BEGIN,INK_V1,999,2\n"])
        with self.assertRaises(ValueError):
            request_export(device)


if __name__ == "__main__":
    unittest.main()
