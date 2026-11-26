import unittest
from tools.device_auth_reference import proof
from tools.device_auth_transport import exchange_auth


class Peer:
    def __init__(self, mode="ok"):
        self.mode = mode
        self.key = b"k"*32
        self.nonce = b"n"*32
        self.rx = bytearray()
        self.writes = 0

    def write(self, data):
        self.writes += 1
        if self.mode == "short":
            return len(data)-1
        if self.writes == 1:
            _, ident, host = data.rstrip(b"\n").split(b",")
            self.ident = ident.decode()
            self.host = bytes.fromhex(host.decode())
            p = proof(self.key, "device", self.ident, self.host, self.nonce)
            frame = b"AUTH2,"+self.nonce.hex().encode()+b","+p.hex().encode()
            if self.mode == "oversize":
                frame = b"x"*137
            if self.mode == "bad_cr":
                frame = frame[:10]+b"\r"+frame[10:]
            if self.mode != "silent":
                self.rx.extend(frame+b"\r\n")
        else:
            expected = proof(self.key, "host", self.ident, self.host, self.nonce)
            assert data == b"AUTH3,"+expected.hex().encode()+b"\n"
            ack = proof(self.key, "ack", self.ident, self.host, self.nonce)
            if self.mode == "bad_ack":
                ack = bytes(32)
            self.rx.extend(b"AUTH4,"+ack.hex().encode()+b"\n")
        return len(data)

    def read(self, n):
        if self.mode == "disconnect":
            raise OSError("disconnected")
        out = bytes(self.rx[:n])
        del self.rx[:n]
        return out


class AuthTransportTests(unittest.TestCase):
    def test_success(self):
        peer = Peer()
        self.assertTrue(exchange_auth(peer,"tab5-001",peer.key))
        self.assertEqual(peer.writes,2)

    def test_failure_modes(self):
        for mode in ("short","oversize","bad_cr","disconnect","silent","bad_ack"):
            with self.subTest(mode=mode):
                peer = Peer(mode)
                self.assertFalse(exchange_auth(peer,"tab5-001",peer.key))
                self.assertEqual(peer.writes,2 if mode=="bad_ack" else 1)

    def test_wrong_key(self):
        peer = Peer()
        self.assertFalse(exchange_auth(peer,"tab5-001",b"x"*32))
        self.assertEqual(peer.writes,1)

    def test_deadline(self):
        peer = Peer("silent")
        now = iter(range(100))
        self.assertFalse(exchange_auth(peer,"tab5-001",peer.key,clock=lambda: next(now)))
        self.assertEqual(peer.writes,1)
