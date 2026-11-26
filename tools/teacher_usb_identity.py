"""Read-only development probe; not a device authentication mechanism."""
import logging
import secrets
import time
from contextlib import closing
from dataclasses import dataclass
from html import escape
from tools.teacher_usb_discovery import scan_usb, PORT

LOG = logging.getLogger(__name__)


@dataclass(frozen=True)
class IdentityResult:
    status: str
    port: str


def exchange(connection, nonce, *, clock=time.monotonic):
    request = f"HELLO,1,{nonce}\n".encode("ascii")
    expected = f"LEARNER,1,{nonce},TAB5_LEARNER".encode("ascii")
    deadline = clock() + 3
    if connection.write(request) != len(request):
        return "error"
    pending = bytearray()
    discard = False
    for _ in range(128):
        if clock() >= deadline:
            break
        chunk = connection.read(64)
        for c in chunk:
            if c == 10:
                if not discard and bytes(pending).rstrip(b"\r") == expected:
                    return "responding"
                pending.clear()
                discard = False
            elif not discard:
                if len(pending) >= 128:
                    discard = True
                    pending.clear()
                else:
                    pending.append(c)
    return "timeout"


def identify(port, *, scan=scan_usb, serial_factory=None):
    if not isinstance(port, str) or not PORT.fullmatch(port):
        raise ValueError("Invalid COM port")
    port = port.upper()
    try:
        if port not in scan().ports:
            return IdentityResult("not_detected", port)
        if serial_factory is None:
            from serial import Serial
            serial_factory = Serial
        # Configure modem control before opening. Never invoke bootloader/reset.
        with closing(serial_factory(port=None, baudrate=115200, timeout=0.1,
                                    write_timeout=0.5)) as connection:
            connection.dtr = False
            connection.rts = False
            connection.port = port
            connection.open()
            status = exchange(connection, secrets.token_hex(16))
            return IdentityResult(status, port)
    except ImportError:
        LOG.warning("Serial dependency unavailable")
        return IdentityResult("dependency", port)
    except Exception:
        LOG.exception("Learner identification failed")
        return IdentityResult("error", port)


def identity_form(port, csrf):
    return f"""<form method="post" action="/usb-identify">
    <input type="hidden" name="csrf" value="{escape(csrf, quote=True)}">
    <input type="hidden" name="port" value="{escape(port, quote=True)}">
    <button>Identify learner on {escape(port)}</button></form>"""


def identity_panel(result):
    messages = {"responding": "Learner protocol v1 responding",
                "timeout": "No matching learner reply. Firmware may not support identification.",
                "not_detected": "Selected USB port is no longer detected.",
                "dependency": "Serial dependency unavailable.",
                "error": "Could not complete serial probe. Check the local server log."}
    return f"""<section><h2>{messages[result.status]}</h2>
    <p>{escape(result.port)} — development identification only, NOT authenticated.</p>
    <p>No student data transferred. Assignment remains planned.</p>
    <p><a href="/">Return to dashboard</a></p></section>"""
