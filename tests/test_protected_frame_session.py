import os
import struct
import unittest
from pathlib import Path

from tools.device_frame_session import FrameSession
from tools.protected_frame_session import ProtectedFrameSession
from tools.math_snapshot_receiver import MathSnapshotReceiver
from tools.math_backup_codec import decode
from tools.device_auth_wire import HostAuthWire
from tools.device_auth_reference import proof


class ProtectedFrameTests(unittest.TestCase):
    def test_handshake_and_no_downgrade(self):
        key=bytes(range(32))
        nonce=bytes(range(64,96))
        wire=HostAuthWire("tab5-001",key,clock=lambda:0)
        host=bytes.fromhex(wire.start().split(b",")[2].decode())
        p=proof(key,"device","tab5-001",host,nonce)
        wire.receive_device(b"AUTH2,"+nonce.hex().encode()+b","+p.hex().encode())
        ack=proof(key,"ack","tab5-001",host,nonce)
        client=wire.receive_ack(b"AUTH4,"+ack.hex().encode(),open_session=True,protected=True)
        self.assertIsInstance(client,ProtectedFrameSession)
        plain=FrameSession(key,host=False,clock=lambda:0)
        with self.assertRaises(ValueError):
            client.receive(plain.send(b"plaintext"))

    def test_tamper_replay_reflection_expiration(self):
        key=bytes(range(32))
        now=[0]
        for failure in ("tamper","replay","reflection","expiry","wrong_key"):
            now[0]=0
            host=ProtectedFrameSession(key,host=True,clock=lambda:now[0])
            device=ProtectedFrameSession(bytes(32) if failure=="wrong_key" else key,
                                        host=False,clock=lambda:now[0])
            raw=host.send(b"private handwriting")
            self.assertNotIn(b"private handwriting",raw)
            if failure=="tamper":
                changed=bytearray(raw)
                changed[7]^=1
                with self.assertRaises(PermissionError):device.receive(bytes(changed))
            elif failure=="replay":
                self.assertEqual(device.receive(raw),b"private handwriting")
                with self.assertRaises(ValueError):device.receive(raw)
            elif failure=="reflection":
                with self.assertRaises(ValueError):host.receive(raw)
                continue
            elif failure=="expiry":
                now[0]=120
                with self.assertRaises(PermissionError):device.receive(raw)
            else:
                with self.assertRaises(PermissionError):device.receive(raw)
            with self.assertRaises(PermissionError):device.send(b"closed")

    def test_cross_language_encrypted_snapshot(self):
        path=os.environ.get("TAB5_PROTECTED_MATH_FIXTURE")
        if not path:self.skipTest("C++ encrypted fixture generated in CI")
        session=ProtectedFrameSession(bytes(range(32)),host=True,clock=lambda:0)
        receiver=MathSnapshotReceiver(session,request_id=bytes(range(16)))
        receiver.request()
        raw=Path(path).read_bytes()
        pos=0
        while pos<len(raw):
            n=struct.unpack_from("<H",raw,pos)[0]
            pos+=2
            self.assertTrue(0<n<=len(raw)-pos)
            receiver.receive(raw[pos:pos+n])
            pos+=n
        result=decode(receiver.result())
        self.assertEqual(len(result.records),2)
        self.assertEqual(result.records[0][24:29],b"\x00\x00\x00\x00\x01")
        self.assertEqual(result.records[1][4],1)
