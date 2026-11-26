"""Synthetic authenticated snapshot receiver; no serial or real student access."""
import logging
import secrets
import struct

from tools.device_frame_session import FrameSession
from tools.math_backup_codec import (MathBackup, checkpoint, replay, encode,
                                     MAX_RECORDS, MAX_RECORD_SIZE)

LOG = logging.getLogger(__name__)
REQUEST, BEGIN, RECORD, END = 0x31, 0x32, 0x33, 0x34


class MathSnapshotReceiver:
    """NEW -> WAIT_BEGIN -> RECORDS -> COMPLETE, or permanently FAILED."""
    def __init__(self, session, *, request_id=None):
        if not isinstance(session, FrameSession):
            raise TypeError("Authenticated frame session required")
        self._session = session
        self._id = secrets.token_bytes(16) if request_id is None else request_id
        if type(self._id) is not bytes or len(self._id) != 16:
            raise ValueError("Invalid snapshot request ID")
        self._state = "NEW"
        self._rows = []
        self._base = self._final = self._progress = None
        self._count = 0

    def abort(self):
        self._state = "FAILED"
        self._rows.clear()
        self._base = self._final = self._progress = None
        self._session.close()

    def request(self):
        try:
            if self._state != "NEW":
                raise PermissionError("Snapshot request already used")
            frame = self._session.send(bytes([REQUEST]) + self._id)
            self._state = "WAIT_BEGIN"
            return frame
        except (ValueError, PermissionError, TypeError):
            self.abort()
            LOG.warning("Snapshot request rejected")
            raise

    def receive(self, frame):
        try:
            if self._state not in ("WAIT_BEGIN", "RECORDS"):
                raise PermissionError("Snapshot receiver not accepting frames")
            payload = self._session.receive(frame)
            if len(payload) < 17 or payload[1:17] != self._id:
                raise ValueError("Snapshot request mismatch")
            kind = payload[0]
            if self._state == "WAIT_BEGIN":
                if kind != BEGIN or len(payload) != 53:
                    raise ValueError("Expected snapshot manifest")
                self._base, self._final = payload[17:33], payload[33:49]
                self._count = struct.unpack_from("<I", payload, 49)[0]
                self._progress = checkpoint(self._base)
                terminal = checkpoint(self._final)
                if self._count > MAX_RECORDS or terminal.attempts-self._progress.attempts != self._count:
                    raise ValueError("Snapshot manifest count invalid")
                self._state = "RECORDS"
            elif kind == RECORD:
                if not 21+28 <= len(payload) <= 21+MAX_RECORD_SIZE:
                    raise ValueError("Invalid snapshot record size")
                slot = struct.unpack_from("<I", payload,17)[0]
                if slot != len(self._rows)+1 or slot > self._count:
                    raise ValueError("Unexpected snapshot record")
                body = payload[21:]
                self._progress = replay(body,self._progress)
                self._rows.append(body)
            elif kind == END:
                if len(payload) != 21 or struct.unpack_from("<I",payload,17)[0] != self._count:
                    raise ValueError("Invalid snapshot end marker")
                if len(self._rows) != self._count or self._progress != checkpoint(self._final):
                    raise ValueError("Incomplete or inconsistent snapshot")
                self._state = "COMPLETE"
                self._session.close()
            else:
                raise ValueError("Unexpected snapshot message")
        except (ValueError, PermissionError, TypeError, struct.error):
            self.abort()
            LOG.warning("Snapshot receive rejected")
            raise

    @property
    def complete(self):
        return self._state == "COMPLETE"

    def result(self):
        try:
            if self._state != "COMPLETE":
                raise PermissionError("Snapshot not complete")
            return encode(MathBackup(self._base,self._final,tuple(self._rows)))
        except (ValueError, PermissionError, TypeError):
            LOG.warning("Incomplete snapshot cannot be published")
            raise
