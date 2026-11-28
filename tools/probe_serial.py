"""Open only a freshly enumerated ESP32 USB candidate for the public probe."""
import logging
from tools.teacher_usb_discovery import scan_usb, PORT

LOG = logging.getLogger(__name__)


def open_probe_stream(port, *, scan=scan_usb, serial_factory=None):
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
        return connection
    except Exception:
        LOG.warning("Public probe serial open failed")
        if connection is not None:
            try:
                connection.close()
            except Exception:
                LOG.warning("Serial close failed")
        raise
