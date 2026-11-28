# First Tab5 Hardware Prototype (not yet flashed)

## What this branch actually provides

A **hardware bring-up** firmware experiment. It draws a sample math problem (`7 + 5 = ?`), records a child's finger strokes inside a large canvas, and allows CLEAR or read-only EXPORT via USB serial. It **does not** yet recognize numerals, grade the answer, persist attempts, synchronize a student profile, or authenticate a teacher. It must not be used for actual student records.

## Verified hardware facts

- M5Stack documents a USB-C data port, 1280×720 display, and a PlatformIO ESP32-P4 Arduino configuration.
- **Multiple screen/touch revisions exist**: early ILI9881C + GT911; later ST7123; newest ST7121. Check the rear sticker before debugging any display issues.
- M5Unified and M5GFX provide revision-aware display/touch access. This firmware intentionally uses their public display/touch APIs rather than hard-coded controller registers.
- Wireless is not initialized by this program, but **absence of a Wi-Fi API call is not security certification**. Radio-power state, recovery mode, and hardening remain unverified.

## Windows build steps (not executed here)

1. Install **VS Code + PlatformIO IDE**, then open the `firmware` directory from the repository.
2. Connect the Tab5 using a **USB-C data cable** and identify its COM port in Windows Device Manager.
3. Run `pio run` from the `firmware` folder to build. The initial dependency download requires internet access on the developer laptop, not on the student device.
4. Do not flash until the current firmware backup is confirmed. The previous dice-roller can be intentionally replaced once a backup is saved.
5. When ready, run `pio run -t upload --upload-port COMx` (replace `COMx` with the detected port). Follow M5Stack's **Download Mode** procedure if the board is not discovered.
6. Connect a serial monitor at **115200** on the same device port. Send each command followed by newline: `PING`, `STATUS`, `EXPORT`. Stop the serial monitor before uploading again.

## Expected diagnostic protocol

- `PING` -> `PONG,TAB5_INK_PROBE,1`
- `STATUS` -> `STATUS,1,<point_count>,<stroke_count>,<overflow_flag>`
- `EXPORT` -> `BEGIN,INK_V1,<point_count>,<stroke_count>`; `PT,<index>,<x>,<y>,<new_stroke>` for each sample; `END,OK` or `END,TRUNCATED`
- Unknown or oversized commands are rejected; the host cannot use this diagnostic protocol to alter grades or install content.

## Hardware acceptance checklist

- [ ] Builds with the documented PlatformIO environment; record exact resolved library versions.
- [ ] Boots on the actual Tab5 display revision in the intended stand orientation.
- [ ] Entire touchscreen tracks finger moves, with no unexplained coordinate reversal.
- [ ] Drawing a digit such as `1` or `2` shows a continuous stroke; lifting the finger breaks the line.
- [ ] CLEAR removes strokes but does not restart the device.
- [ ] `PING`, `STATUS`, and `EXPORT` return valid records over USB-C; malformed input cannot freeze the UI.
- [ ] Power-cycle and USB disconnect/reconnect behavior are observed and logged.
- [ ] Non-network operation and ESP32-C6 radio-power state are explicitly tested.

## Next increments

1. Add versioned, durable attempt storage and a locally confirmed numeral interpretation.
2. Build the Windows teacher USB manager with authentication and backups.
3. Add teacher corrections, recomputed mastery, and safe update transactions.
4. Only then extend to a validated classroom-ready learning curriculum.

## Sources

- M5Stack Tab5 hardware and PlatformIO setup: https://docs.m5stack.com/en/core/Tab5
- M5Unified touch example: https://github.com/m5stack/M5Unified/blob/master/examples/Basic/Touch/DragDrop/DragDrop.ino
