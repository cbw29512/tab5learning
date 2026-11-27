"""Physical public-fixture test; never access or save real student data."""
import argparse
import hashlib
import logging

from tools.teacher_usb_discovery import scan_usb, PORT
from tools.protected_snapshot_transport import exchange_synthetic_snapshot
from tools.math_backup_codec import decode, checkpoint

LOG = logging.getLogger(__name__)


def verify_fixture(raw):
    """Independently verify the expected two answers and original ink bytes."""
    backup = decode(raw)
    if len(backup.records) != 2:
        raise ValueError("Expected exactly two public sample attempts")
    first, second = backup.records
    base = checkpoint(backup.base)
    final = checkpoint(backup.final)
    if (base.attempts != 0 or base.index != 0 or final.attempts != 2
            or final.index != 1 or final.independent != 0
            or first[3:8] != bytes([0,0,1,1,0])
            or first[24:29] != bytes([0,0,0,0,1])
            or second[3:8] != bytes([0,1,2,0,0])):
        raise ValueError("Public fixture differs from expected sample")
    return hashlib.sha256(raw).hexdigest()


def run_probe(port, *, scan=scan_usb, serial_factory=None):
    connection = None
    try:
        if not isinstance(port,str) or not PORT.fullmatch(port):
            raise ValueError("Invalid COM port")
        port = port.upper()
        if port not in scan().ports:
            raise ValueError("Selected ESP32 USB port not detected")
        if serial_factory is None:
            from serial import Serial
            serial_factory = Serial
        connection = serial_factory(port=None,baudrate=115200,timeout=0.1,
                                    write_timeout=0.5)
        connection.dtr = False
        connection.rts = False
        connection.port = port
        connection.open()
        # Discard stale boot diagnostic bytes before initiating a fresh handshake.
        connection.reset_input_buffer()
        raw = exchange_synthetic_snapshot(connection,"probe-only",bytes(range(32)),
                                           test_mode=True)
        return verify_fixture(raw)
    except Exception:
        LOG.warning("Public synthetic USB verification failed")
        raise
    finally:
        if connection is not None:
            try:
                connection.close()
            except Exception:
                LOG.warning("Serial close failed")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port",required=True)
    parser.add_argument("--synthetic-only",action="store_true",required=True)
    args = parser.parse_args()
    logging.basicConfig(level=logging.INFO)
    try:
        digest = run_probe(args.port)
        print("PROTECTED USB SNAPSHOT VERIFIED")
        print("Public attempts: 2; original ink points: 1")
        print("PLMB SHA256:",digest)
        print("No student data saved. Restore learner firmware next.")
        return 0
    except Exception:
        LOG.error("Probe failed; check firmware, USB connection and exclusive COM access.")
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
