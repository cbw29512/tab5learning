"""Explicit AES-GCM USB session reference; no silent plaintext downgrade."""
import hmac
import logging
import struct
import time

from tools.device_frame_session import FrameSession, MAX_PAYLOAD

LOG = logging.getLogger(__name__)
DOMAIN = b"TAB5-GCM-V1\0"
OVERHEAD = 23


class ProtectedFrameSession(FrameSession):
    """Requires a fresh, unique transcript key per connection."""
    def __init__(self, key, *, host, clock=time.monotonic):
        from cryptography.hazmat.primitives.ciphers.aead import AESGCM
        super().__init__(key, host=host, clock=clock)
        self._key = hmac.digest(key, DOMAIN, "sha256")
        self._aead = AESGCM(self._key)
        self._tx |= 0x80
        self._rx |= 0x80

    def close(self):
        super().close()
        self._aead = None

    @staticmethod
    def _nonce(direction, sequence):
        return struct.pack("<BI", direction, sequence) + bytes(7)

    def send(self, payload):
        try:
            self._check(self._sent)
            if type(payload) is not bytes or not 0 < len(payload) <= MAX_PAYLOAD:
                raise ValueError("Invalid protected payload")
            header = struct.pack("<BIH",self._tx,self._sent,len(payload))
            encrypted = self._aead.encrypt(self._nonce(self._tx,self._sent),payload,header)
            self._sent += 1
            return header+encrypted
        except Exception:
            self.close()
            LOG.warning("Protected USB frame send rejected")
            raise

    def receive(self, frame):
        from cryptography.exceptions import InvalidTag
        try:
            self._check(self._received)
            if type(frame) is not bytes or not OVERHEAD < len(frame) <= OVERHEAD+MAX_PAYLOAD:
                raise ValueError("Invalid protected frame size")
            direction, sequence, length = struct.unpack_from("<BIH",frame)
            if direction != self._rx or sequence != self._received or length != len(frame)-OVERHEAD:
                raise ValueError("Invalid protected frame order or direction")
            try:
                plaintext = self._aead.decrypt(self._nonce(direction,sequence),frame[7:],frame[:7])
            except InvalidTag:
                raise PermissionError("Protected frame authentication failed") from None
            self._received += 1
            return plaintext
        except Exception:
            self.close()
            LOG.warning("Protected USB frame receive rejected")
            raise
