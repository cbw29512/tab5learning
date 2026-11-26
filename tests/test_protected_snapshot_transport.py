"""Fake serial peer exercises the real handshake and encrypted transfer path."""
import struct
import unittest

from tools.device_auth_reference import proof
from tools.device_frame_session import session_key
from tools.protected_frame_session import ProtectedFrameSession
from tools.math_snapshot_receiver import BEGIN, RECORD, END, REQUEST
from tools.math_backup_codec import decode
from tools.protected_snapshot_transport import exchange_synthetic_snapshot
from test_math_backup_archive import snapshot


class Peer:
    def __init__(self, mode="ok"):
        self.mode=mode
        self.key=bytes(range(32))
        self.nonce=bytes(range(64,96))
        self.raw=snapshot(3)
        self.rx=bytearray()
        self.writes=0
        self.closed=False

    def write(self,data):
        self.writes+=1
        if self.writes==1:
            _, ident, host=data.rstrip(b"\n").split(b",")
            self.ident=ident.decode()
            self.host=bytes.fromhex(host.decode())
            p=proof(self.key,"device",self.ident,self.host,self.nonce)
            self.rx.extend(b"AUTH2,"+self.nonce.hex().encode()+b","+p.hex().encode()+b"\n")
        elif self.writes==2:
            expected=proof(self.key,"host",self.ident,self.host,self.nonce)
            if data!=b"AUTH3,"+expected.hex().encode()+b"\n":
                raise ValueError("Unexpected proof")
            ack=proof(self.key,"ack",self.ident,self.host,self.nonce)
            if self.mode=="bad_ack":
                ack=bytes(32)
            self.rx.extend(b"AUTH4,"+ack.hex().encode()+b"\n")
        elif self.writes==3:
            if self.mode=="short_write":
                return len(data)-1
            key=session_key(self.key,self.ident,self.host,self.nonce)
            session=ProtectedFrameSession(key,host=False,clock=lambda:0)
            request=session.receive(data)
            if len(request)!=17 or request[0]!=REQUEST:
                raise ValueError("Invalid request")
            ident=request[1:]
            b=decode(self.raw)
            frames=[session.send(bytes([BEGIN])+ident+b.base+b.final+struct.pack("<I",len(b.records)))]
            for i,body in enumerate(b.records,1):
                frames.append(session.send(bytes([RECORD])+ident+struct.pack("<I",i)+body))
            frames.append(session.send(bytes([END])+ident+struct.pack("<I",len(b.records))))
            session.close()
            if self.mode=="missing_end":
                frames.pop()
            elif self.mode=="tamper":
                frame=bytearray(frames[1])
                frame[-1]^=1
                frames[1]=bytes(frame)
            elif self.mode=="oversize":
                frames[0]=b"\x82\x00\x00\x00\x00\xff\xff"
            elif self.mode=="truncated":
                frames[-1]=frames[-1][:-1]
            elif self.mode=="plaintext":
                frames[0]=b"\x02"+frames[0][1:]
            self.rx.extend(b"".join(frames))
        else:
            raise ValueError("Unexpected extra write")
        return len(data)

    def read(self,size):
        if self.mode=="disconnect" and self.writes==3:
            raise OSError("Disconnected")
        # Deliberately fragment both line and binary responses.
        n=min(size,3)
        result=bytes(self.rx[:n])
        del self.rx[:n]
        return result

    def close(self):
        self.closed=True


class ProtectedSnapshotTransportTests(unittest.TestCase):
    def exchange(self,peer,**kwargs):
        return exchange_synthetic_snapshot(peer,"probe-only",peer.key,test_mode=True,
                                           clock=lambda:0,**kwargs)

    def test_verified_complete_exchange(self):
        peer=Peer()
        self.assertEqual(self.exchange(peer),peer.raw)
        self.assertEqual(peer.writes,3)
        self.assertTrue(peer.closed)

    def test_failures_do_not_return_partial_data(self):
        for mode in ("bad_ack","short_write","missing_end","tamper","oversize",
                     "truncated","plaintext","disconnect"):
            with self.subTest(mode=mode):
                peer=Peer(mode)
                with self.assertRaises((ValueError,PermissionError,OSError)):
                    self.exchange(peer)
                self.assertTrue(peer.closed)
                self.assertEqual(peer.writes,2 if mode=="bad_ack" else 3)

    def test_no_normal_device_or_implicit_test_mode(self):
        for ident,mode in (("probe-only",False),("tab5-001",True)):
            peer=Peer()
            with self.assertRaises(PermissionError):
                exchange_synthetic_snapshot(peer,ident,peer.key,test_mode=mode)
            self.assertEqual(peer.writes,0)
            self.assertTrue(peer.closed)
