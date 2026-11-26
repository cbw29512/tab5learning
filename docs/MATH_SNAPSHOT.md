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
