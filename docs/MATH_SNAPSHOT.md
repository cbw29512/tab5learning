# Internal mathematics snapshot contract — synthetic preview

A recovered journal can produce a bounded read-only snapshot for a future
authorized backup implementation. Schema: base checkpoint (V1, 16 bytes),
final checkpoint (V1, 16 bytes), count of retained V1 attempt records, ordered
one-based physical slots with exact encoded bytes and lengths. The base
checkpoint is zero-progress unless it represents a migrated legacy summary.
A nonzero base explicitly means prior attempts lack individual evidence.

Flow: reconcile journal -> validate manifest -> stage manifest -> reread and
replay each record -> validate terminal checkpoint -> commit staged snapshot.
A missing, corrupt or out-of-order record or mismatched terminal state prevents
publication. A sink failure aborts staging. The sink must not publish partial
results and must implement commit/abort transactionally. No callbacks may
reenter journal writes or recovery. No whole-journal RAM allocation is required
by the exporter. An ambiguous or failed append requires recovery before export.

This is an internal C++ interface, not a file or wire format, authenticated
USB command, encrypted storage, restore operation or released teacher feature.
No external access is added. A future host importer must independently validate
the exact bytes, identity, versions, authorization and integrity; CRC is not an
authentication mechanism. Never erase device records to free space until a
separately verified backup and retention policy authorize it. No flash
partition changes, learner UI changes or real student data in this slice.

## Host preview container (V1)

The local candidate format uses magic bytes PLMB followed by version byte 1,
base and final 16-byte V1 checkpoints, a little-endian uint32 record count,
then ordered records each prefixed by a little-endian uint16 byte length,
and a trailing SHA-256 of all preceding bytes. Order implies physical slot.
The host implementation independently validates original V1 record CRCs,
ink bounds, question/choice scoring, sequential checkpoints and final state.
Nonzero base remains summary-only progress. Host preview limit 4096 records,
each 28–2588 bytes; this is a parser resource bound, not a retention policy.
No unbounded file read is implemented.

The container is intentionally not an authenticated or production backup:
no identity, encryption, transport authorization or persistent file storage is
implemented here. SHA-256 does not establish provenance. Never expose this
codec as a teacher import or restore route without the protected identity and
authorization layer. No device or teacher database changes.

## Durable host preview archive

Separate synthetic SQLite database, explicit test-mode construction only.
Current TeacherRegistry planned assignment and active TeacherSessions gate
every save/load; a planned assignment is not proof of device identity.
Tables: device/student/base checkpoint (one-to-one), immutable ordered
attempt bytes (device, slot), immutable receipt (device, count, final,
container SHA-256, actor, timestamp). Historical receipts reconstruct exact
prefix snapshots without duplicating attempt BLOBs. Begin immediate -> verify
existing head and row count -> verify exact prefix extension or exact retry
-> append new records and receipt -> re-encode/readback -> commit. Reject
unknown older prefixes, divergent ink, base changes, wrong scope or damaged
records; no overwrite or delete operations. Reads use a transaction and
revalidate digest and replay. No browser route, device restore, encrypted
production storage or authenticated device origin; do not use real student
data. This is an internal reference for a future authorized import.

## Non-destructive device restore candidate

Internal synthetic restore image: existing snapshot manifest plus immutable
staged length-prefixed V1 record bytes, maximum 4096 records. Caller must
retain exclusive ownership of the staged buffer for the synchronous call,
and authenticate identity and the complete input before invoking it.
Preflight validates all bytes, exact terminal checkpoint, identical legacy
base and exact persisted prefix. Older history and conflicts are rejected;
no writes on invalid preflight. Only missing suffix records can be appended.
Each record uses the existing NVS commit and exact readback. Full storage or
ambiguous commit leaves a recoverable prefix; the caller must rerun journal
recovery before retrying, and may not acknowledge full restore until Complete.
An exact replay is a no-op. The multi-record operation is resumable, not atomic
as a whole. No automatic legacy base installation on a blank device. The
candidate does not allocate staging memory, implement a USB command, verify
device identity, provision a student or authorize restoration. Not for real
student data. Physical capacity and power loss remain release gates.

## Authenticated synthetic frame transfer reference
One snapshot per session. Request 0x31 + 16-byte request ID; manifest 0x32 + ID + base/final 16-byte checkpoints + uint32 LE count; record 0x33 + ID + uint32 LE one-based slot + original V1 bytes; end 0x34 + ID + uint32 LE count. All carried inside existing direction/sequence/HMAC frames. Device uses existing journal snapshot verifier and bounded member buffers; no end if source verification or writer fails. Host stages bounded records and independently replays, publishes PLMB only after exact terminal marker and verified checkpoint. No actual serial route or real student operation authorization; synchronous writer is a test reference only, not a production nonblocking transport. No encryption of frames yet. C++ fake-NVS test exports original ink and emits a fixture consumed by Python in CI, with interrupt and wrong-frame tests. Reconnect/restart after failure; never remove device data. CI pending.

## Incremental maintenance export candidate
MathJournalExportCursor uses the existing MathSnapshotSink and verifier, locks the recovered journal during the operation, emits one manifest/record/terminal per ready poll, does not write storage and releases the lock on completion, rejection, timeout, explicit cancellation or scope destruction. No reentrant callbacks; journal and sink must outlive cursor. Poll even under backpressure to enforce 110-second deadline. No commit until full replay verified; on disconnect discard and restart under a new authenticated session, not an across-connection resume. Exclusive maintenance mode is mandatory before actual integration: must disable answer submission without losing scratchpad and show cancellation/progress; never silently reject a learner save. No production dispatch, authorization or large SD export yet. Host CI regression covers 100 attempts, exact bytes, blocked concurrent writes, backpressure, timeout, failure, corruption, empty history and release. Native compile/runtime pending.

## Incremental protected frame integration (synthetic tests only)
SyntheticIncrementalTransfer binds the existing protected-frame sink to the incremental cursor after the shared authenticated request decoder accepts one request. No unencrypted variant, no new protocol messages, no firmware dispatch. One frame per ready poll; verify each record before queuing; terminal queued is not delivered or durably imported. Caller owns session/writer/journal lifetime and must drain or discard pending ciphertext on disconnect. Timeout/cancellation releases journal lock, closes session and invalidates incomplete snapshots. Tests use fake NVS and OpenSSL GCM for 100 records, exact bytes, backpressure, denial, tamper, cancellation and timeout. Production maintenance UI, per-operation authorization, disconnected output cleanup, diagnostics isolation and physical long-transfer measurement still required. CI pending.
