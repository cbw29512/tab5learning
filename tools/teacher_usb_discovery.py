"""Read-only host USB enumeration, not device authentication."""
import logging
import re
from dataclasses import dataclass
from html import escape
from itertools import islice

LOG = logging.getLogger(__name__)
VID, PID = 0x303A, 0x1001
MAX_SCAN = 256
MAX_CANDIDATES = 32
PORT = re.compile(r"COM[1-9][0-9]{0,5}\Z", re.IGNORECASE)


@dataclass(frozen=True)
class UsbSnapshot:
    status: str
    ports: tuple[str, ...] = ()
    limited: bool = False


def scan_usb(enumerate_ports=None):
    try:
        if enumerate_ports is None:
            from serial.tools.list_ports import comports
            enumerate_ports = comports
        seen = set()
        limited = False
        for index, item in enumerate(islice(enumerate_ports(), MAX_SCAN + 1)):
            if index == MAX_SCAN:
                limited = True
                break
            if (item.vid, item.pid) != (VID, PID):
                continue
            if not isinstance(item.device, str) or not PORT.fullmatch(item.device):
                continue
            if len(seen) >= MAX_CANDIDATES and item.device.upper() not in seen:
                limited = True
                break
            seen.add(item.device.upper())
        ports = tuple(sorted(seen, key=lambda name: int(name[3:])))
        return UsbSnapshot("detected" if ports else "empty", ports, limited)
    except ImportError:
        LOG.warning("Optional pyserial dependency unavailable")
        return UsbSnapshot("dependency")
    except Exception:
        LOG.exception("USB enumeration failed")
        return UsbSnapshot("error")


def scan_panel(csrf):
    return f"""<section><h2>USB device discovery</h2>
    <p>Find connected ESP32 USB interfaces without opening a serial port.</p>
    <form method="post" action="/usb-scan">
    <input type="hidden" name="csrf" value="{escape(csrf, quote=True)}">
    <button>Scan connected devices</button></form></section>"""


def scan_result(snapshot, csrf=None):
    messages = {
        "detected": "ESP32 USB candidates detected",
        "empty": "No matching USB interfaces found",
        "dependency": "USB discovery dependency not installed. Install tools/requirements-windows.txt.",
        "error": "USB enumeration failed. Check the local server log.",
    }
    rows = "".join(f"<li>{escape(port)} — USB 303A:1001</li>" for port in snapshot.ports)
    limit = "<p>Scan limit reached; results may be incomplete.</p>" if snapshot.limited else ""
    actions = ""
    if csrf is not None:
        from tools.teacher_usb_identity import identity_form
        actions = "".join(identity_form(port, csrf) for port in snapshot.ports)
    return f"""<section><h2>{messages[snapshot.status]}</h2><ul>{rows}</ul>{limit}
    <p>USB identity is not authenticated. No assignment has been synced.
    No serial port was opened during discovery.</p>{actions}<p><a href="/">Return to dashboard</a></p></section>"""
