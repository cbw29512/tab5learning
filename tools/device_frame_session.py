"""Bounded authenticated USB data frame reference; not encrypted transport."""
import hashlib
import hmac
import logging
import struct
import time

from tools.teacher_registry import normalize

LOG = logging.getLogger(__name__)
MAX_PAYLOAD = 4096
MAX_SEQUENCE = 65536
DOMAIN = b"TAB5-SESSION-V1\0"
FRAME_DOMAIN = b"TAB5-FRAME-V1\0"


def session_key(key, device_id, host_nonce, device_nonce):
    if any(type(x) is not bytes or len(x) != 32 for x in (key, host_nonce, device_nonce)):
        raise ValueError("Invalid session transcript")
    transcript = DOMAIN + normalize(device_id).encode("ascii") + b"\0" + host_nonce + device_nonce
    return hmac.digest(key, transcript, "sha256")


class FrameSession:
    """One connection; independent directional sequences, fail closed on errors."""
    def __init__(self, key, *, host, clock=time.monotonic):
        if type(key) is not bytes or len(key) != 32 or type(host) is not bool:
            raise ValueError("Invalid frame session")
        self._key = key
        self._tx = 1 if host else 2
        self._rx = 2 if host else 1
        self._sent = self._received = 0
        self._clock = clock
        self._started = clock()

    def close(self):
        self._key = None

    def _check(self, sequence):
        if self._key is None or not 0 <= self._clock()-self._started < 120 or sequence >= MAX_SEQUENCE:
            raise PermissionError("Frame session unavailable")

    def send(self, payload):
        try:
            self._check(self._sent)
            if type(payload) is not bytes or not 0 < len(payload) <= MAX_PAYLOAD:
                raise ValueError("Invalid frame payload")
            body = struct.pack("<BIH", self._tx, self._sent, len(payload)) + payload
            result = body + hmac.digest(self._key, FRAME_DOMAIN + body, "sha256")
            self._sent += 1
            return result
        except (ValueError, PermissionError, TypeError, struct.error):
            self.close()
            LOG.warning("USB frame send rejected")
            raise

    def receive(self, frame):
        try:
            self._check(self._received)
            if type(frame) is not bytes or not 39 < len(frame) <= 39 + MAX_PAYLOAD:
                raise ValueError("Invalid frame size")
            direction, sequence, length = struct.unpack_from("<BIH", frame)
            if direction != self._rx or sequence != self._received or length != len(frame)-39:
                raise ValueError("Unexpected frame order or direction")
            if not hmac.compare_digest(frame[-32:], hmac.digest(self._key, FRAME_DOMAIN + frame[:-32], "sha256")):
                raise PermissionError("Invalid frame authentication")
            self._received += 1
            return frame[7:-32]
        except (ValueError, PermissionError, TypeError, struct.error):
            self.close()
            LOG.warning("USB frame receive rejected")
            raise
