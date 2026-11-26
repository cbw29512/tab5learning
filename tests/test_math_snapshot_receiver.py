import os
import struct
import unittest
from pathlib import Path

from tools.device_frame_session import FrameSession
from tools.math_snapshot_receiver import MathSnapshotReceiver, BEGIN, RECORD, END
from tools.math_backup_codec import decode
from test_math_backup_archive import snapshot


class SnapshotReceiverTests(unittest.TestCase):
    def setUp(self):
        self.key=bytes(range(32))
        self.ident=bytes(range(16))
        self.client=FrameSession(self.key,host=True,clock=lambda:0)
        self.device=FrameSession(self.key,host=False,clock=lambda:0)
        self.receiver=MathSnapshotReceiver(self.client,request_id=self.ident)
        self.assertEqual(self.device.receive(self.receiver.request()),b"1"+self.ident)
        self.backup=decode(snapshot(2))

    def send(self,kind,body,ident=None):
        self.receiver.receive(self.device.send(bytes([kind])+(self.ident if ident is None else ident)+body))

    def begin(self,count=2):
        self.send(BEGIN,self.backup.base+self.backup.final+struct.pack("<I",count))

    def record(self,i):
        self.send(RECORD,struct.pack("<I",i)+self.backup.records[i-1])

    def test_full_snapshot(self):
        self.begin()
        self.record(1)
        with self.assertRaises(PermissionError):
            self.receiver.result()
        self.record(2)
        self.send(END,struct.pack("<I",2))
        self.assertEqual(self.receiver.result(),snapshot(2))

    def test_interruption_and_bad_order(self):
        self.begin()
        with self.assertRaises(ValueError):
            self.record(2)
        with self.assertRaises(PermissionError):
            self.receiver.result()

    def test_missing_end_and_wrong_count(self):
        self.begin()
        self.record(1)
        with self.assertRaises(ValueError):
            self.send(END,struct.pack("<I",2))
        with self.assertRaises(PermissionError):
            self.receiver.result()

    def test_wrong_id(self):
        with self.assertRaises(ValueError):
            self.send(BEGIN,self.backup.base+self.backup.final+struct.pack("<I",2),bytes(16))

    def test_cross_language_fixture(self):
        path=os.environ.get("TAB5_MATH_FRAME_FIXTURE")
        if not path:
            self.skipTest("C++ fixture generated in CI")
        raw=Path(path).read_bytes()
        pos=0
        while pos<len(raw):
            n=struct.unpack_from("<H",raw,pos)[0]
            pos+=2
            self.assertTrue(0<n<=len(raw)-pos)
            self.receiver.receive(raw[pos:pos+n])
            pos+=n
        result=decode(self.receiver.result())
        self.assertEqual(len(result.records),2)
        self.assertEqual(result.records[0][6:8],b"\x01\x00")
        self.assertEqual(result.records[0][24:29],b"\x00\x00\x00\x00\x01")
        self.assertEqual(result.records[1][4],1)
