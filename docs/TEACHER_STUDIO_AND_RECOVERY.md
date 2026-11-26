# Teacher Studio and SSH maintenance contract

Status: approved interface direction; implementation and physical acceptance pending. This contract extends the MVP and USB security requirements without enabling wireless on the student device.

## Interfaces and trust boundaries

- Student: standalone Tab5 touchscreen, one assigned child, no network dependency, no administrative shell.
- Teacher Studio: local browser website on the Windows teacher computer, initially bound to loopback only. Authenticated teacher accounts; review attempts, handwriting and audio, correct grades, manage content/flashcards, assign work, view progress, print certificates, back up and restore. A local backend communicates with the attached device over authenticated USB. Browser does not talk directly to a remote child device. No cloud accounts or automatic network synchronization.
- Technical maintenance: SSH into an authorized maintenance host that has the Tab5 physically attached via USB. The host owns esptool and verified board profiles. On Windows use the existing supported USB tools; remote SSH requires a separately configured trusted host and verified serial access. No SSH daemon or wireless requirement on Tab5. Prefer key-based SSH, no public internet exposure, limited service identity and command allowlist. No generic shell exposed by the teacher website.

## Maintenance modes

1. Diagnose: read device identity, build, reset reason, serial logs, display/power state; do not change flash.
2. Repair firmware: verify signed release manifest, chip/board revision, image hash, partition layout, device identity and backup. Reflash the correct application or approved recovery components; preserve student data. Reject unsupported layout, device or image. Verify readback and physical startup before marking recovered.
3. Restore clean firmware: reinstall approved known-good learning firmware and recover compatible authorized student backup, if needed. Distinct from a data wipe.
4. Factory reset / reassignment: archive/export existing records first where possible, explicitly authorize destructive reset with correct device identity and typed confirmation, securely remove student records and credentials, return to unprovisioned state. No automatic reset on firmware error. Do not erase SD contents or unrelated partitions by default.
5. Emergency recovery: when ordinary app/USB protocol is unresponsive, use board-supported USB bootloader; physical button/download mode may be required. No promise of remote recovery from dead hardware, disconnected USB or power failure. Production secure boot and flash encryption policy must preserve an authorized recovery path, validated on this exact board before deployment.

## Data and state

- Maintenance job: job_id, device_id, operation_kind, actor_id, role, release_id, manifest_hash, expected_partition_digest, backup_id, confirmation_state, status, timestamps, error_code, verification evidence. No student answers in operational logs.
- Release manifest: release_id, hardware profile, compatible revisions, partition digest, artifact hashes, signed publisher metadata, schema compatibility.
- Backup manifest: backup_id, device_id, schema/version, creation time, content hash, protected storage reference, restore authorization.
- Job state: requested -> authorized -> preflight_passed -> backed_up (when required) -> applying -> verified -> completed. Failures -> failed/recovery_required; never report success based solely on flash write. Destructive reset requires separately recorded approval. Retry must reconcile prior state and avoid repeating destructive operations.
- Teachers have educational administration rights; maintenance role has firmware privileges. Access to educational records is not implied by maintenance authorization.

## Definition of done

- Teacher website works without internet, binds to loopback, enforces authentication and role authorization, and uses the same validated USB protocol as the local manager.
- Teacher can export, review, correct and import an attempt with durable acknowledgement; no wireless.
- Maintenance host can diagnose and reflash a USB-connected prototype via SSH using a verified release without loss of acknowledged learning progress.
- Demonstrate recovery when the learner app will not start; test interrupted transfer, wrong board/release and invalid signature rejection.
- Factory reset cannot be invoked by a child or automatically by a failed boot; test explicit authorization, archive and unprovisioned state with synthetic data.
- Verify warm and cold startup, display and touch, battery-only boot, USB reconnect and progress persistence. Current intermittent display startup remains a release blocker.
- Never use actual student records until privacy/security review and authorized recovery/backup safeguards pass.

Initial implementation: use the existing teacher backend and USB transport after reviewing their source. Keep recovery tooling isolated from the web server and student app. Select frameworks and serialization only after examining existing implementation; no new network services on the Tab5.
