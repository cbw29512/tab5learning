"""Bounded authentication exchange over an already-open, timeout-configured stream."""
import logging
import time

from tools.device_auth_wire import HostAuthWire, MAX_FRAME

LOG = logging.getLogger(__name__)


class AuthStream:
    def __init__(self, connection, clock):
        self.connection = connection
        self.clock = clock
        self.deadline = clock() + 10
        self.read_budget = 4096
        self.byte_budget = 512

    def check_time(self):
        if self.clock() >= self.deadline:
            raise TimeoutError("Authentication exchange expired")

    def send(self, frame):
        self.check_time()
        data = frame + b"\n"
        if self.connection.write(data) != len(data):
            raise OSError("Incomplete authentication write")
        self.check_time()

    def receive(self):
        line = bytearray()
        cr = False
        while self.read_budget > 0 and self.byte_budget > 0:
            self.check_time()
            self.read_budget -= 1
            chunk = self.connection.read(1)
            self.check_time()
            if not isinstance(chunk, bytes) or len(chunk) > 1:
                raise OSError("Invalid transport response")
            if not chunk:
                continue
            self.byte_budget -= 1
            c = chunk[0]
            if c == 10:
                if not line:
                    raise ValueError("Empty authentication frame")
                return bytes(line)
            if cr:
                raise ValueError("Invalid CR placement")
            if c == 13:
                cr = True
                continue
            if not 32 <= c <= 126 or len(line) >= MAX_FRAME:
                raise ValueError("Invalid authentication frame")
            line.append(c)
        raise TimeoutError("Authentication receive budget exhausted")


def exchange_auth(connection, device_id, key, *, clock=time.monotonic):
    """Requires a caller-owned stream with short finite read/write timeouts.

    Success proves only the peer handshake, never authorization for data writes.
    The caller must close/reopen the stream after every attempt; no implicit retry.
    """
    wire = None
    try:
        transport = AuthStream(connection, clock)
        wire = HostAuthWire(device_id, key, clock=clock)
        transport.send(wire.start())
        transport.send(wire.receive_device(transport.receive()))
        result = wire.receive_ack(transport.receive())
        transport.check_time()
        return result
    except (OSError, ValueError, PermissionError):
        LOG.warning("Authentication exchange failed")
        return False
    except Exception:
        # Do not log arbitrary driver exceptions, which could include frame data.
        LOG.warning("Authentication transport error")
        return False
    finally:
        if wire is not None:
            wire.cancel()
