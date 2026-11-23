# Windows Tab5 USB diagnostic

**Current status:** Windows probe source and mock tests exist. The actual Tab5 firmware must be built, flashed, and verified on hardware before this utility can communicate with it.

## Before modifying the Tab5

The old dice-roller firmware can be overwritten by a new app, but **make and verify a backup before flashing**. This script only reads USB ink traces; it never flashes the device. Do not connect a real child profile or use real student data during the hardware probe.

## Developer laptop setup (PowerShell)

From Windows PowerShell, with Python 3.10+ and Git installed:

```powershell
git clone --branch feature/tab5-ink-usb-probe https://github.com/cbw29512/tab5learning.git
Set-Location tab5learning
py -m pip install -r tools/requirements-windows.txt
py -m unittest discover -s tests -v
py -m serial.tools.list_ports -v
```

The last command lists available serial ports, but **a port alone does not prove the Tab5 diagnostic firmware is installed**. Confirm the port by plugging/unplugging the Tab5 and checking which entry appears.

For firmware compilation, use VS Code with PlatformIO, open `firmware/`, and build the environment defined in `firmware/platformio.ini`. **Do not flash until the existing firmware backup is confirmed** and compilation succeeds. Follow [Tab5 hardware probe steps](TAB5_HARDWARE_PROBE.md) for the board-specific flashing process.

After flashing, draw a few digits in the on-screen canvas and run the read-only probe. Replace `COM5` with the COM port reported on *your* PC:

```powershell
py -m tools.usb_probe --port COM5 --output "$HOME\Downloads\tab5_ink_capture.json"
```

Expected result: an `INFO` log confirms the Tab5 diagnostic protocol and the count of captured touch points; the resulting JSON file contains a `TAB5_INK_V1` schema label, stroke count, and coordinate records. The file stays on the teacher/developer PC and must not be committed to GitHub.

### Troubleshooting

- **Port not found:** Use a data-capable USB-C cable and run `py -m serial.tools.list_ports -v`. Check Windows Device Manager.
- **Port access denied:** Close PlatformIO's serial monitor or any other program using the port.
- **PING timeout:** The prototype firmware might not be installed, the wrong COM port was selected, or the board's CDC transport requires debugging.
- **EXPORT rejected:** The device returned data inconsistent with the expected protocol. Do not import or grade it; retain the diagnostic log.
- **No points:** Draw inside the screen's large canvas first, then rerun the command.

## Architecture notes

The parser defines the versioned `InkPoint` / `InkReport` data schema and enforces maximum record counts before files are written. The USB reader issues read-only `PING` and `EXPORT` commands; it does not enable teacher corrections, grade manipulation, content imports, or automatic firmware updates.

The script is intentionally divided into small files. A later authenticated teacher manager will use a distinct protocol and must be designed and security-tested separately.
