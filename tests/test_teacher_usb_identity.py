import unittest
from tools.teacher_usb_discovery import UsbSnapshot
from tools.teacher_usb_identity import exchange, identify


class FakeSerial:
    def __init__(self, **kwargs):
        self.port = kwargs.get("port")
        self.dtr = True
        self.rts = True
        self.opened = False
        self.closed = False
        self.reply = bytearray()

    def open(self):
        assert self.port == "COM5"
        assert self.dtr is False and self.rts is False
        self.opened = True

    def close(self):
        self.closed = True

    def write(self, request):
        nonce = request.decode().strip().split(",")[2]
        self.reply.extend(f"boot log\nLEARNER,1,{nonce},TAB5_LEARNER\n".encode())
        return len(request)

    def read(self, count):
        result = bytes(self.reply[:7])
        del self.reply[:7]
        return result


class IdentityTests(unittest.TestCase):
    def test_matching_nonce_and_control_lines(self):
        serial = FakeSerial()
        result = identify("COM5", scan=lambda: UsbSnapshot("detected", ("COM5",)),
                          serial_factory=lambda **kwargs: serial)
        self.assertEqual(result.status, "responding")
        self.assertTrue(serial.opened and serial.closed)

    def test_disconnected_port_not_opened(self):
        def forbidden(**kwargs):
            self.fail("Should not open an absent device")
        self.assertEqual(identify("COM5", scan=lambda: UsbSnapshot("empty"),
                                  serial_factory=forbidden).status, "not_detected")
        with self.assertRaises(ValueError):
            identify("COM5;RESET")

    def test_wrong_nonce_and_overlong_lines(self):
        class Wrong(FakeSerial):
            def write(self, request):
                self.reply.extend(b"x"*300+b"\nLEARNER,1,"+b"0"*32+b",TAB5_LEARNER\n")
                return len(request)
        self.assertEqual(exchange(Wrong(), "1"*32), "timeout")
