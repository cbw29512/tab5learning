"""Host mutual authentication wire reference, with no transport or write grants."""
import hmac
import logging
import re
import time

from tools.device_auth_reference import HostChallenge, proof
from tools.device_frame_session import session_key, FrameSession

LOG = logging.getLogger(__name__)
HEX = re.compile(rb"[0-9a-f]{64}\Z")
MAX_FRAME = 136


def fields(frame, prefix, count):
    if not isinstance(frame, bytes) or not 0 < len(frame) <= MAX_FRAME:
        raise ValueError("Invalid authentication frame length")
    parts = frame.split(b",")
    if len(parts) != count + 1 or parts[0] != prefix:
        raise ValueError("Unexpected authentication frame")
    if any(not HEX.fullmatch(part) for part in parts[1:]):
        raise ValueError("Invalid proof or nonce encoding")
    return tuple(bytes.fromhex(part.decode("ascii")) for part in parts[1:])


class HostAuthWire:
    """Single attempt: NEW -> WAIT_DEVICE -> WAIT_ACK -> CLOSED."""

    def __init__(self, device_id, key, *, clock=time.monotonic):
        if not isinstance(key, bytes) or len(key) != 32:
            raise ValueError("Invalid device credential")
        self._clock = clock
        self._key = key
        self._challenge = HostChallenge.begin(device_id, clock=clock)
        self._state = "NEW"
        self._ack = None
        self._session_key = None

    def cancel(self):
        self._state = "CLOSED"
        self._key = None
        self._ack = None
        self._session_key = None
        self._challenge.consumed = True

    def _require(self, state):
        if self._state != state or not 0 <= self._clock() - self._challenge.started <= 10:
            raise PermissionError("Authentication attempt invalid or expired")

    def start(self):
        try:
            self._require("NEW")
            self._state = "WAIT_DEVICE"
            return (b"AUTH1," + self._challenge.device_id.encode("ascii") + b"," +
                    self._challenge.host_nonce.hex().encode("ascii"))
        except Exception:
            self.cancel()
            LOG.warning("Authentication start rejected")
            raise

    def receive_device(self, frame):
        try:
            self._require("WAIT_DEVICE")
            nonce, device_proof = fields(frame, b"AUTH2", 2)
            host_proof = self._challenge.complete(self._key, nonce, device_proof, clock=self._clock)
            self._ack = proof(self._key, "ack", self._challenge.device_id,
                              self._challenge.host_nonce, nonce)
            self._session_key = session_key(self._key, self._challenge.device_id,
                                            self._challenge.host_nonce, nonce)
            self._key = None
            self._state = "WAIT_ACK"
            return b"AUTH3," + host_proof.hex().encode("ascii")
        except Exception:
            self.cancel()
            LOG.warning("Device authentication frame rejected")
            raise

    def receive_ack(self, frame, *, open_session=False):
        try:
            self._require("WAIT_ACK")
            (receipt,) = fields(frame, b"AUTH4", 1)
            if not hmac.compare_digest(self._ack, receipt):
                raise PermissionError("Authenticated receipt rejected")
            return FrameSession(self._session_key, host=True, clock=self._clock) if open_session else True
        except Exception:
            LOG.warning("Authentication receipt rejected")
            raise
        finally:
            self.cancel()
