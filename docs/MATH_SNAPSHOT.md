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
