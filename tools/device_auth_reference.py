"""Host reference for a future mutual USB authentication protocol."""
import hashlib
import hmac
import logging
import secrets
import time
from dataclasses import dataclass, field

from tools.teacher_registry import normalize

LOG = logging.getLogger(__name__)
DOMAIN = b"TAB5-AUTH-V1\0"


def proof(key, role, device_id, host_nonce, device_nonce):
    if not isinstance(key, bytes) or len(key) != 32:
        raise ValueError("Expected a 32-byte device credential")
    if role not in ("device", "host", "ack"):
        raise ValueError("Invalid proof role")
    if any(not isinstance(n, bytes) or len(n) != 32 for n in (host_nonce, device_nonce)):
        raise ValueError("Invalid challenge nonce")
    transcript = (DOMAIN + role.encode("ascii") + b"\0" +
                  normalize(device_id).encode("ascii") + b"\0" + host_nonce + device_nonce)
    return hmac.new(key, transcript, hashlib.sha256).digest()


@dataclass
class HostChallenge:
    device_id: str
    host_nonce: bytes = field(repr=False)
    started: float
    consumed: bool = False

    @classmethod
    def begin(cls, device_id, *, clock=time.monotonic):
        return cls(normalize(device_id), secrets.token_bytes(32), clock())

    def complete(self, key, device_nonce, device_proof, *, clock=time.monotonic):
        # Consume even a failed attempt; require a new challenge for retries.
        if self.consumed:
            raise PermissionError("Challenge already consumed")
        self.consumed = True
        try:
            if not 0 <= clock() - self.started <= 10:
                raise PermissionError("Challenge expired")
            expected = proof(key, "device", self.device_id, self.host_nonce, device_nonce)
            if not isinstance(device_proof, bytes) or not hmac.compare_digest(expected, device_proof):
                raise PermissionError("Device proof rejected")
            # This only authenticates the peer to the host. The peer must still
            # verify this distinct proof before accepting protected operations.
            return proof(key, "host", self.device_id, self.host_nonce, device_nonce)
        except (ValueError, PermissionError):
            LOG.warning("USB authentication reference rejected challenge")
            raise
