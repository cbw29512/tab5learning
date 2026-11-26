"""Stage-level diagnostics for the installed synthetic USB probe; no flashing."""
import argparse
import logging
import time
from contextlib import closing

from tools.device_auth_transport import AuthStream
from tools.device_auth_wire import HostAuthWire
from tools.teacher_usb_discovery import PORT, scan_usb

LOG = logging.getLogger(__name__)


class MeteredStream:
    def __init__(self, connection):
        self.connection = connection
        self.received = 0

    def write(self, data):
        return self.connection.write(data)

    def read(self, count):
        data = self.connection.read(count)
        self.received += len(data)
        return data


def diagnose(port):
    stage = "SCAN"
    stream = wire = None
    try:
        if not PORT.fullmatch(port) or port.upper() not in scan_usb().ports:
            print("SCAN,NOT_DETECTED")
            return False
        from serial import Serial
        stage = "OPEN"
        with closing(Serial(port=None, baudrate=115200, timeout=0.1,
                            write_timeout=0.5)) as connection:
            connection.dtr = False
            connection.rts = False
            connection.port = port.upper()
            connection.open()
            print("OPEN,PASS", flush=True)
            # Give the USB driver time to settle. Inspect bounded startup bytes
            # without printing arbitrary device output or cryptographic fields.
            stage = "STARTUP"
            startup = bytearray()
            until = time.monotonic() + 1.5
            while time.monotonic() < until and len(startup) < 512:
                startup.extend(connection.read(min(64, 512-len(startup))))
            banner = ("FAIL" if b"SYNTHETIC_AUTH_PROBE,FAIL" in startup else
                      "READY" if b"SYNTHETIC_AUTH_PROBE,READY" in startup else "NOT_SEEN")
            print(f"STARTUP,{banner},bytes={len(startup)}", flush=True)
            if banner == "FAIL":
                return False
            connection.reset_input_buffer()
            stage = "RESYNC"
            # Empty line cancels any prior partial challenge, not a device reset.
            if connection.write(b"\n") != 1:
                raise OSError("Incomplete resync")
            time.sleep(0.6)
            connection.reset_input_buffer()
            stream = MeteredStream(connection)
            transport = AuthStream(stream, time.monotonic)
            wire = HostAuthWire("probe-only", bytes(range(32)))
            stage = "SEND_AUTH1"
            transport.send(wire.start())
            print("AUTH1,SENT", flush=True)
            stage = "RECEIVE_AUTH2"
            response = transport.receive()
            print(f"AUTH2,RECEIVED,bytes={len(response)},prefix_ok={response.startswith(b'AUTH2,')}", flush=True)
            stage = "VERIFY_AUTH2"
            reply = wire.receive_device(response)
            print("AUTH2,VERIFIED", flush=True)
            stage = "SEND_AUTH3"
            transport.send(reply)
            print("AUTH3,SENT", flush=True)
            stage = "RECEIVE_AUTH4"
            response = transport.receive()
            print(f"AUTH4,RECEIVED,bytes={len(response)},prefix_ok={response.startswith(b'AUTH4,')}", flush=True)
            stage = "VERIFY_AUTH4"
            wire.receive_ack(response)
            transport.check_time()
            print("SYNTHETIC_USB_HANDSHAKE,PASS", flush=True)
            return True
    except Exception as exc:
        # Only status, phase and byte count; never echo frames or key material.
        count = stream.received if stream else 0
        LOG.warning("Synthetic probe failed at %s (%s)", stage, type(exc).__name__)
        print(f"DIAGNOSTIC,FAIL,stage={stage},error={type(exc).__name__},rx_bytes={count}", flush=True)
        return False
    finally:
        if wire is not None:
            wire.cancel()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", required=True)
    args = parser.parse_args()
    logging.basicConfig(level=logging.INFO)
    return 0 if diagnose(args.port) else 1


if __name__ == "__main__":
    raise SystemExit(main())
