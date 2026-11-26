import unittest
from tools.device_auth_wire import HostAuthWire
from tools.device_auth_reference import proof
from tools.device_frame_session import FrameSession, session_key


class FrameSessionTests(unittest.TestCase):
    def test_handshake_then_directional_frames(self):
        credential = bytes(range(32))
        nonce = bytes(range(64,96))
        wire = HostAuthWire("tab5-001", credential, clock=lambda: 0)
        host = bytes.fromhex(wire.start().split(b",")[2].decode())
        device_proof = proof(credential,"device","tab5-001",host,nonce)
        wire.receive_device(b"AUTH2,"+nonce.hex().encode()+b","+device_proof.hex().encode())
        ack = proof(credential,"ack","tab5-001",host,nonce)
        client = wire.receive_ack(b"AUTH4,"+ack.hex().encode(),open_session=True)
        peer = FrameSession(session_key(credential,"tab5-001",host,nonce),host=False,clock=lambda:0)
        frame = client.send(b"request")
        self.assertEqual(peer.receive(frame),b"request")
        self.assertEqual(client.receive(peer.send(b"response")),b"response")
        with self.assertRaises(ValueError):
            peer.receive(frame)
        with self.assertRaises(PermissionError):
            peer.send(b"after replay")

    def test_tamper_reflection_expiration_and_order(self):
        now = [0]
        key = bytes(range(32))
        for failure in ("tamper","reflection","order","expiration"):
            now[0]=0
            host = FrameSession(key,host=True,clock=lambda:now[0])
            device = FrameSession(key,host=False,clock=lambda:now[0])
            first = host.send(b"one")
            second = host.send(b"two")
            if failure=="tamper":
                changed=bytearray(first)
                changed[7]^=1
                with self.assertRaises(PermissionError):
                    device.receive(bytes(changed))
            elif failure=="reflection":
                with self.assertRaises(ValueError):
                    host.receive(first)
            elif failure=="order":
                with self.assertRaises(ValueError):
                    device.receive(second)
            else:
                now[0]=120
                with self.assertRaises(PermissionError):
                    device.receive(first)

    def test_distinct_transcripts(self):
        key=bytes(range(32))
        host=bytes(32)
        device=bytes([1])*32
        self.assertNotEqual(session_key(key,"tab5-001",host,device),
                            session_key(key,"tab5-002",host,device))
