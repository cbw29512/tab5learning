# Tab5 Learning — USB-Only Management and Security

## Operating model

- **Learner device:** One child's offline lessons, touch input and locally stored progress.
- **Teacher PC:** Windows laptop with a protected local management application; no cloud account or school Wi-Fi required.
- **Transport:** One physically connected USB data cable. The supported Tab5 USB device interface and protocol will be **verified on hardware** before choosing CDC serial, USB class, or a vendor-specific interface.
- **Wireless:** Wi-Fi and Bluetooth are not used by the learning application or teacher manager. Their disabled state must be checked in the production firmware, not assumed merely because the UI has no network controls.
- **Normal updates:** Lessons, corrections, configuration and reports move via authenticated wired sessions only. Recovery/programming interfaces are a separate security consideration.

## Proposed safe correction transaction

1. Connect and identify the device; verify the device ID and negotiated schema/protocol versions.
2. Authenticate the teacher. Do not rely only on an easily guessed PIN to authenticate a USB host.
3. Export the relevant original attempt, stored trace and current effective grade.
4. Display the correction for teacher review and confirmation.
5. Create an operation with unique ID, target attempt ID, desired correction, version and integrity metadata.
6. Validate authorization, integrity, supported format and target state on the device.
7. Atomically append the correction and recompute mastery. Acknowledge only after durable commit.
8. Reconnecting and resending the same operation must return the prior result without creating duplicate changes.
9. Make an independent local backup to the teacher's PC before destructive or large updates.

## Threats and mitigations to validate

| Threat | Planned control / limitation |
| --- | --- |
| Student attempts wireless access | No wireless operation in released learner firmware; verify and test |
| Unauthorized teacher menu access | Authenticated teacher mode, lockout/rate limiting, limited permissions |
| Malicious USB computer | Device-side authorization, bounded parser, integrity checks, no trust based on cable alone |
| Tampered lesson or firmware image | Authenticated/signed packages; validate signatures in firmware, manage keys outside repository |
| Lost or stolen device | Data minimization and privacy-focused local storage; study encryption support and threat model |
| Edited USB backup | Backup integrity validation; distinguish restoring an approved backup from trusting arbitrary imported files |
| Direct flashing / physical access | Evaluate device secure boot, flash encryption, recovery modes and hardware constraints before making tamper-resistance claims |
| Partial USB transfer or unplugged cable | Atomic state writes, explicit acknowledgments, idempotent operation IDs and recovery tests |

## Repository safety

No actual student data, answer traces, identifiers tied to real people, teacher PINs, passwords, private signing keys, real backup files or embedded secrets in version control. Audit the licenses of every third-party firmware component, recognition model and curriculum resource before distribution.

## Open verification tasks

- Confirm exact Tab5 firmware, chip revision, touch driver and USB device capability with official documentation **and** a physical spike.
- Measure offline handwriting-recognition accuracy with children's digit samples and provide an explicit correction path.
- Test power-loss persistence, USB disconnects, and corrupted package rejection.
- Choose a practical teacher authentication model and approved school data-handling/retention requirements before pilot deployment.

## Authenticated frame reference
A distinct HMAC-SHA256 transcript domain TAB5-SESSION-V1 (NUL terminated) derives a key from the existing credential, canonical device ID (NUL terminated), host nonce and device nonce. Optional key handoff only after successful AUTH3 on device and verified AUTH4 on host. Existing handshake-only transport still discards key; no actual serial data route is enabled. Frame body: direction byte (1 host, 2 device), little-endian uint32 sequence from zero per direction, uint16 payload length, 1–4096 payload bytes; append HMAC-SHA256 over TAB5-FRAME-V1 NUL and body. Strict order, direction, length, authentication, 120-second session lifetime, max 65536 frames per direction, close on error/disconnect (caller must enforce disconnect), poll expiry to avoid timer wrap. This is integrity/authentication, NOT confidentiality or operation permission. No real student data until encrypted transfer, authorization, bounded transport and physical verification. Tests pending.

## Protected data-frame candidate
Explicit AES-256-GCM mode, distinct from synthetic HMAC-only mode; no fallback. Derive separate AEAD key as HMAC-SHA256(transcript session key, TAB5-GCM-V1 NUL). Header = 0x81 host / 0x82 device, uint32 LE directional sequence, uint16 LE plaintext length; authenticated as associated data. 12-byte nonce = direction, sequence LE, seven zero bytes. Ciphertext plus 16-byte tag. Nonce uniqueness requires fresh mutually authenticated transcript key per connection, no counter restart under same key, bounded sequence and lifetime. Metadata length/direction remains visible; no forward secrecy. Python AESGCM and ESP-IDF Mbed TLS adapter (documented API https://mbed-tls.readthedocs.io/projects/api/en/v3.6.7/api/file/gcm_8h/) with OpenSSL native-host fixture. Existing snapshot sender works with either explicit mode, host receiver reuses replay. Actual serial adapter, teacher operation authorization, connection/nonce lifecycle enforcement, hardware crypto build and physical tests still required. Isolated compile probe only, no learner release or actual records. CI pending.

## Host protected snapshot serial reference
Exclusive finite-timeout stream, explicit synthetic test mode and probe-only identity. AUTH1–4 ASCII line exchange then one encrypted binary snapshot request; read exactly 7-byte header and bounded length+16-byte tag without scanning/resync, validate AEAD and replay, accept only complete terminal snapshot. Deadline 115 seconds after handshake, per-read and frame-count budgets, no plaintext fallback or automatic retry. Close stream/session on every result, including failures. No persistent import, real student authority or device-side binary serial integration. Fake fragmented serial peer exercises real mutual authentication, encryption and exact PLMB replay; wrong ACK, truncated/missing end, tamper, oversized header, plaintext, disconnect and short writes rejected. CI pending.

## Isolated device USB snapshot probe
New protected_usb_probe build uses HWCDC, existing AUTH1–4 and optional one-use session handoff only after AUTH4 bytes are accepted by stream. Public probe-only credential and two generated mathematics attempts in RAM (one ink point), no learner NVS/SD access. Fixed 40-byte protected request, bounded 512-byte ciphertext response queue, partial reads/writes in 32-byte poll budgets, timeout/invalid frame fails closed, no plaintext fallback, one snapshot per observed USB connection. A disconnect must be observed before retry; hardware USB disconnect detection still requires validation. No diagnostic text mixed into binary protocol. Host fake-stream test checks partial writes, valid complete response, tamper, expiry and disconnect. Native CI build only; never upload without separate verified backup and explicit physical approval. Actual journal, larger nonblocking export, student permission and secure production credential handling remain pending.

## Windows physical probe runbook
Use repair_tab5_preview.ps1 -ProtectedUsbProbe without -Install first, pinned revision, expected MAC and fresh dual-read matching full backup. Compare exact existing partition table and image checksum; no write in preflight. Only after review add -Install to write application alone, not partition table or data. Then python -m tools.run_protected_usb_probe --port COM5 --synthetic-only, close serial monitor first. Verify exactly two sample attempts and one original ink point, independent replay, digest. Restore known learner application before resuming learning. No real student records or credentials; public synthetic probe key. Physical evidence pending.

## Physical encrypted synthetic USB acceptance
Operator-reported Windows COM5 test at a32177a9 succeeded: guarded app-only flash verified, then `python -m tools.run_protected_usb_probe --port COM5 --synthetic-only` reported PROTECTED USB SNAPSHOT VERIFIED; two public attempts, one original ink point, PLMB SHA-256 `73fc9787985ee1950abf097bd87fcfa43cc12c4a9b0efa636c9fa77aabde78f8`. Native ESP32-P4 build and host tests passed locally (14 pass, one CI-only fixture skipped). Fresh dual-read recovery backup verified before temporary probe installation. This is positive-path physical interoperability only: production keys, live student journal authorization and export, hardware interruption/replay tests, full CI and exact learner application restoration/retention still pending. Do not mark production synchronization ready. Restore known learner app from verified pre-probe backup without changing other partitions; verify original learner progress before resuming.

## Scoped synthetic archive ingestion
PreviewUsbSync owns an exclusive stream, admits only explicit test-mode confirmation and current teacher scope over reserved `probe-only` / `probe-student` planned assignment, verifies the complete encrypted PLMB exchange, rechecks the active teacher/assignment before preview and transactional save, and returns a receipt only after durable archive verification. Public probe key cannot establish a real student identity; destination is never caller selectable. Prior history preserved on partial, conflicting or denied transfer. Exact retry supported. Host fake serial/SQLite regressions added; no real student route or browser-triggered USB, no device writes. Production enrollment and authorization remain mandatory.
