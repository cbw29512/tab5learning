import unittest
from tools.run_auth_probe import run_probe
from tools.teacher_usb_discovery import UsbSnapshot


class Port:
    def __init__(self):
        self.dtr = self.rts = True
        self.port = None
        self.opened = self.closed = self.drained = False

    def open(self):
        assert self.port == "COM5"
        assert not self.dtr and not self.rts
        self.opened = True

    def reset_input_buffer(self):
        assert self.opened
        self.drained = True

    def close(self):
        self.closed = True


class NativeProbeTests(unittest.TestCase):
    def test_explicit_synthetic_probe(self):
        port = Port()

        def exchange(connection, ident, key):
            self.assertIs(connection, port)
            self.assertTrue(port.drained)
            self.assertEqual(ident, "probe-only")
            self.assertEqual(key, bytes(range(32)))
            return True

        self.assertTrue(run_probe("com5", scan=lambda: UsbSnapshot("detected", ("COM5",)),
                                  serial_factory=lambda **kw: port, exchange=exchange))
        self.assertTrue(port.closed)

    def test_absent_or_invalid_port_never_opened(self):
        def factory(**kw):
            self.fail("Unexpected serial open")
        for name in ("COM5", "COM5;RESET"):
            self.assertFalse(run_probe(name, scan=lambda: UsbSnapshot("empty"),
                                       serial_factory=factory))

    def test_failure_closes_port(self):
        port = Port()
        self.assertFalse(run_probe("COM5", scan=lambda: UsbSnapshot("detected", ("COM5",)),
                                   serial_factory=lambda **kw: port, exchange=lambda *a: False))
        self.assertTrue(port.closed)
