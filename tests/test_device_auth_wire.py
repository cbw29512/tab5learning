import unittest
from tools.device_auth_reference import proof
from tools.device_auth_wire import HostAuthWire


class WireTests(unittest.TestCase):
    def setUp(self):
        self.now = 0
        self.key = bytes(range(32))
        self.nonce = bytes(range(64, 96))
        self.wire = HostAuthWire("tab5-001", self.key, clock=lambda: self.now)
        self.host = bytes.fromhex(self.wire.start().split(b",")[2].decode())

    def device_frame(self, role="device"):
        p = proof(self.key, role, "tab5-001", self.host, self.nonce)
        return b"AUTH2," + self.nonce.hex().encode() + b"," + p.hex().encode()

    def ack_frame(self):
        return b"AUTH4," + proof(self.key, "ack", "tab5-001", self.host, self.nonce).hex().encode()

    def test_full_exchange_and_replay(self):
        host = self.wire.receive_device(self.device_frame())
        self.assertEqual(host, b"AUTH3," + proof(self.key, "host", "tab5-001", self.host, self.nonce).hex().encode())
        self.assertTrue(self.wire.receive_ack(self.ack_frame()))
        with self.assertRaises(PermissionError):
            self.wire.receive_ack(self.ack_frame())

    def test_false_receipt_consumes_attempt(self):
        host = self.wire.receive_device(self.device_frame())
        with self.assertRaises(PermissionError):
            self.wire.receive_ack(b"AUTH4," + host[6:])
        with self.assertRaises(PermissionError):
            self.wire.receive_ack(self.ack_frame())

    def test_wrong_role(self):
        with self.assertRaises(PermissionError):
            self.wire.receive_device(self.device_frame("host"))

    def test_malformed_consumes_attempt(self):
        with self.assertRaises(ValueError):
            self.wire.receive_device(self.device_frame()+b"x")
        with self.assertRaises(PermissionError):
            self.wire.receive_device(self.device_frame())

    def test_expired_ack(self):
        self.wire.receive_device(self.device_frame())
        self.now = 11
        with self.assertRaises(PermissionError):
            self.wire.receive_ack(self.ack_frame())

    def test_ack_before_device(self):
        with self.assertRaises(PermissionError):
            self.wire.receive_ack(self.ack_frame())

    def test_maximum_id(self):
        wire = HostAuthWire("a"*64,self.key)
        self.assertEqual(len(wire.start()),135)
