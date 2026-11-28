"""Explicitly enabled local admin-only public USB probe workflow."""
import logging
from html import escape

from tools.preview_usb_sync import PreviewUsbSync, PROBE_DEVICE, PROBE_STUDENT
from tools.probe_serial import open_probe_stream
from tools.teacher_usb_discovery import scan_usb, PORT
from tools.teacher_history_review import hidden

LOG = logging.getLogger(__name__)
ROUTE = "/sync/probe"


class ProbeSyncCenter:
    def __init__(self, archive):
        self.archive = archive
        self.sync = PreviewUsbSync(archive, test_mode=True, public_fixture_only=True)

    def _authorized(self, token):
        self.archive.sessions.require(token, "admin")

    def page(self, token, csrf, *, candidates=None, receipt=None, error=False):
        self._authorized(token)
        rows = self.archive.registry.list_assignments(token)
        assigned = any(r["student_id"] == PROBE_STUDENT and r["device_id"] == PROBE_DEVICE
                       and r["status"] == "PLANNED" for r in rows)
        guard = hidden("csrf", csrf)
        body = ('<p class="notice">PUBLIC SYNTHETIC USB TEST ONLY. Never connect a real student device. '
                'Requires the temporary protected_usb_probe firmware. '
                'No learning firmware is installed or updated by this page.</p>'
                '<p><a href="/">Return to dashboard</a></p>')
        if error:
            body += '<p role="alert">Synchronization failed. No new receipt confirmed. Check the probe, cable, or archive.</p>'
        if not assigned:
            body += ('<p>Create the reserved planned assignment in the dashboard first: '
                     '<code>probe-student</code> / <code>probe-only</code>, grade 1.</p>')
            return body
        body += (f'<section><form method="post" action="{ROUTE}">{guard}'
                 '<button name="action" value="scan">Scan connected USB devices</button></form></section>')
        if candidates is not None:
            ports = "".join(f'<option value="{escape(p,quote=True)}">{escape(p)}</option>'
                            for p in candidates.ports if PORT.fullmatch(p))
            if candidates.status != "detected" or not ports:
                body += '<p>No usable ESP32 USB candidate detected.</p>'
            else:
                body += (f'<section><form method="post" action="{ROUTE}">{guard}'
                         f'<label>USB candidate<select name="port" required>{ports}</select></label>'
                         '<label><input type="checkbox" name="confirm" value="yes" required>'
                         'I confirm the public probe firmware is installed.</label>'
                         '<p>Keep USB connected until a result appears. The local request may take up to about two minutes.</p>'
                         '<button name="action" value="receive">Synchronize public sample</button></form></section>')
        if receipt is not None:
            body += (f'<section><h2>Verified sample saved</h2><p>Attempts: {receipt.records}</p>'
                     f'<p>SHA-256: <code>{escape(receipt.sha256)}</code></p>'
                     + ('<p>Exact retry; no duplicate attempts.</p>' if receipt.exact_retry else '')
                     + '<p><a href="/history">Review saved mathematics history</a></p></section>')
        return body

    def handle(self, token, csrf, data):
        self._authorized(token)
        action = data.get("action")
        fields = set(data)
        if action == "scan" and fields == {"csrf","action"}:
            self.sync._scope(token)
            return self.page(token, csrf, candidates=scan_usb())
        if action != "receive" or fields != {"csrf","action","port","confirm"}:
            raise ValueError("Invalid probe operation")
        if data["confirm"] != "yes":
            raise PermissionError("Explicit probe confirmation required")
        self.sync._scope(token)
        try:
            # Port is independently re-enumerated by the shared opener.
            connection = open_probe_stream(data["port"])
            receipt = self.sync.receive(token, connection, confirmed=True)
            return self.page(token, csrf, receipt=receipt)
        except PermissionError:
            raise
        except Exception:
            LOG.warning("Public probe browser synchronization failed")
            return self.page(token, csrf, error=True)
