"""Explicit physical test of the isolated public test-key USB probe only."""
import argparse
import logging
from contextlib import closing

from tools.device_auth_transport import exchange_auth
from tools.teacher_usb_discovery import scan_usb, PORT

LOG = logging.getLogger(__name__)


def run_probe(port, *, scan=scan_usb, serial_factory=None, exchange=exchange_auth):
    try:
        if not PORT.fullmatch(port) or port.upper() not in scan().ports:
            LOG.warning("Selected ESP32 interface not detected")
            return False
        if serial_factory is None:
            from serial import Serial
            serial_factory = Serial
        with closing(serial_factory(port=None, baudrate=115200, timeout=0.1,
                                    write_timeout=0.5)) as connection:
            connection.dtr = False
            connection.rts = False
            connection.port = port.upper()
            connection.open()
            # Discard preexisting boot diagnostics, never retry a failed exchange.
            connection.reset_input_buffer()
            return bool(exchange(connection, "probe-only", bytes(range(32))))
    except Exception:
        LOG.warning("Synthetic USB probe failed")
        return False


def main():
    parser = argparse.ArgumentParser(description="Synthetic auth_usb_probe only; NOT real device authentication")
    parser.add_argument("--port", required=True)
    args = parser.parse_args()
    logging.basicConfig(level=logging.INFO)
    ok = run_probe(args.port)
    print("SYNTHETIC_USB_HANDSHAKE," + ("PASS" if ok else "FAIL"))
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
