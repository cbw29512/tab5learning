"""Read-only Windows/USB diagnostic. No grade changes or firmware flashing."""
import argparse
from dataclasses import asdict
import json
import logging
import os
from pathlib import Path
import time

from tools.ink_protocol import MAX_POINTS, InkReport, parse_export

LOGGER = logging.getLogger(__name__)


def read_line(connection) -> str:
    """Read one ASCII protocol line, treating silence as a failed transfer."""
    try:
        raw = connection.readline()
        if not raw:
            raise TimeoutError("USB serial response timed out")
        if len(raw) > 128:
            raise ValueError("USB serial line exceeds limit")
        return raw.decode("ascii").strip()
    except (TimeoutError, ValueError, UnicodeDecodeError) as exc:
        LOGGER.error("USB input error: %s", exc)
        raise


def ping(connection) -> None:
    """Recognize our diagnostic firmware, not an arbitrary serial device."""
    try:
        for attempt in range(3):
            connection.write(b"PING\n")
            connection.flush()
            for _ in range(3):
                reply = read_line(connection)
                if reply == "PONG,TAB5_INK_PROBE,1":
                    LOGGER.info("Tab5 diagnostic protocol v1 confirmed")
                    return
                LOGGER.debug("Skipped startup/unknown line: %s", reply)
            LOGGER.warning("No matching PONG, retry %d/3", attempt + 1)
        raise RuntimeError("Tab5 diagnostic firmware did not respond to PING")
    except (OSError, TimeoutError, RuntimeError, ValueError) as exc:
        LOGGER.error("Tab5 handshake failed: %s", exc)
        raise


def request_export(connection) -> InkReport:
    """Bound the serial read using the firmware-declared ink capacity."""
    try:
        connection.write(b"EXPORT\n")
        connection.flush()
        header = read_line(connection)
        fields = header.split(",")
        if len(fields) != 4 or fields[:2] != ["BEGIN", "INK_V1"]:
            raise ValueError(f"unexpected export response: {header!r}")
        declared = int(fields[2])
        if not 0 <= declared <= MAX_POINTS:
            raise ValueError("device declared too many points")
        lines = [header]
        for _ in range(declared + 1):  # Points, then the END marker.
            lines.append(read_line(connection))
        return parse_export(lines)
    except (OSError, TimeoutError, ValueError) as exc:
        LOGGER.error("Ink export failed: %s", exc)
        raise


def write_capture(report: InkReport, destination: Path) -> None:
    """Write one checked diagnostic capture; keep it outside the repository."""
    try:
        destination.parent.mkdir(parents=True, exist_ok=True)
        payload = {
            "schema": "TAB5_INK_V1", "strokes": report.strokes,
            "truncated": report.truncated,
            "points": [asdict(point) for point in report.points],
        }
        temporary = destination.with_name(destination.name + ".tmp")
        temporary.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
        os.replace(temporary, destination)
        LOGGER.info("Saved %d points to %s", len(report.points), destination)
    except OSError as exc:
        LOGGER.error("Failed to save capture to %s: %s", destination, exc)
        raise


def run(port: str, destination: Path) -> int:
    """Open Windows COM port only; never flash or modify device state."""
    try:
        import serial  # Optional dependency: pip install pyserial.
    except ImportError as exc:
        LOGGER.error("pyserial unavailable; run: py -m pip install pyserial")
        raise RuntimeError("pyserial is not installed") from exc
    try:
        with serial.Serial(port=port, baudrate=115200,
                           timeout=3, write_timeout=3) as connection:
            time.sleep(1)  # Allow CDC initialization after opening the port.
            connection.reset_input_buffer()
            ping(connection)
            report = request_export(connection)
        write_capture(report, destination)
        LOGGER.info("Done: %d points, %d strokes%s", len(report.points),
                    report.strokes, " (TRUNCATED)" if report.truncated else "")
        return 0
    except (OSError, RuntimeError, TimeoutError, ValueError, serial.SerialException) as exc:
        LOGGER.error("USB probe unsuccessful on %s: %s", port, exc)
        return 1


def main() -> int:
    """Only accept an explicit port and output file."""
    try:
        parser = argparse.ArgumentParser(description="Tab5 USB ink diagnostic")
        parser.add_argument("--port", required=True, help="Connected Tab5 COM port, e.g. COM5")
        parser.add_argument("--output", required=True, type=Path,
                            help="Local output file OUTSIDE the Git repository")
        args = parser.parse_args()
        logging.basicConfig(level=logging.INFO, format="%(levelname)s: %(message)s")
        return run(args.port, args.output)
    except (OSError, ValueError, RuntimeError) as exc:
        LOGGER.exception("Diagnostic startup failed: %s", exc)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
