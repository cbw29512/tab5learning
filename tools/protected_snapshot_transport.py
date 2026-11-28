"""Synthetic protected snapshot exchange on an exclusively owned USB stream."""
import logging
import time
import struct

from tools.device_auth_transport import AuthStream
from tools.device_auth_wire import HostAuthWire
from tools.device_frame_session import MAX_PAYLOAD
from tools.math_backup_codec import MAX_RECORDS
from tools.math_snapshot_receiver import MathSnapshotReceiver
from tools.protected_frame_session import ProtectedFrameSession

LOG = logging.getLogger(__name__)
HEADER = 7
TAG = 16


class ProtectedStream:
    """Fixed binary header and bounded ciphertext; never scan for magic bytes."""
    def __init__(self, connection, clock):
        self.connection = connection
        self.clock = clock
        self.deadline = clock() + 115

    def check(self):
        if self.clock() >= self.deadline:
            raise TimeoutError("Protected transfer expired")

    def read_exact(self, size):
        if not 0 < size <= MAX_PAYLOAD + TAG:
            raise ValueError("Invalid transfer read size")
        data = bytearray()
        # Budget also handles a broken or frozen clock in development tests.
        budget = size + 1024
        while len(data) < size and budget:
            self.check()
            budget -= 1
            chunk = self.connection.read(size-len(data))
            self.check()
            if type(chunk) is not bytes or len(chunk) > size-len(data):
                raise OSError("Invalid serial read")
            data.extend(chunk)
        if len(data) != size:
            raise TimeoutError("Incomplete protected transfer")
        return bytes(data)

    def receive(self):
        header = self.read_exact(HEADER)
        direction, _, length = struct.unpack("<BIH",header)
        if direction != 0x82 or not 0 < length <= MAX_PAYLOAD:
            raise ValueError("Invalid protected response header")
        # Header fields are untrusted until AEAD authentication succeeds.
        return header + self.read_exact(length + TAG)

    def send(self, frame):
        self.check()
        if self.connection.write(frame) != len(frame):
            raise OSError("Incomplete protected request write")
        self.check()


def exchange_synthetic_snapshot(connection, device_id, key, *, test_mode=False,
                                clock=time.monotonic):
    """Owns and closes a finite-timeout stream. Returns only verified PLMB bytes.

    This reference is intentionally restricted to the public synthetic probe
    identity. It does not grant teacher permission or access learner storage.
    """
    wire = session = receiver = None
    try:
        if test_mode is not True or device_id != "probe-only":
            raise PermissionError("Synthetic snapshot probe only")
        auth = AuthStream(connection, clock)
        wire = HostAuthWire(device_id,key,clock=clock)
        auth.send(wire.start())
        auth.send(wire.receive_device(auth.receive()))
        session = wire.receive_ack(auth.receive(),open_session=True,protected=True)
        auth.check_time()
        if not isinstance(session,ProtectedFrameSession):
            raise PermissionError("Encrypted session required")
        binary = ProtectedStream(connection,clock)
        receiver = MathSnapshotReceiver(session)
        binary.send(receiver.request())
        # One manifest, at most MAX_RECORDS attempts, one end marker.
        for _ in range(MAX_RECORDS+2):
            receiver.receive(binary.receive())
            if receiver.complete:
                result = receiver.result()
                binary.check()
                return result
        raise ValueError("Snapshot frame limit exhausted")
    except Exception:
        if receiver is not None:
            receiver.abort()
        LOG.warning("Synthetic protected snapshot exchange failed")
        raise
    finally:
        if wire is not None:
            wire.cancel()
        if session is not None:
            session.close()
        try:
            connection.close()
        except Exception:
            LOG.warning("USB connection close failed")
