# Flash expansion candidate — no installation

Factory baseline: 16 MiB, existing NVS/PHY/application/model addresses
unchanged. Candidate reserves 2 MiB dedicated learning NVS, 12 KiB
credential NVS and 4 KiB encrypted NVS keys. Remaining tail is reserved.
The candidate CSV is not referenced by any firmware/upload environment.
NVS capacity is not a record-count guarantee: metadata, garbage collection,
ink length, RAM and endurance require measurement. Existing snapshot format
bounds also remain applicable.

Read-only host preflight: exact 16 MiB raw backup, independently recorded
SHA-256, original partition entries and MD5, unchanged candidate layout,
all newly allocated bytes erased. It refuses unknown tables, occupied
candidate ranges and corrupt/truncated backups. This checks a file, not
the connected board, security state, backup recency or restore ability.

Run from repository root on a previously verified backup:
python -m tools.flash_expansion_preflight <backup.bin> --sha256 <recorded-hash>

Do not upload the candidate CSV, erase flash, generate keys, burn eFuses or
move the model partition. The next engineering gate is a dedicated learning
journal backend and resumable exact-copy migration with a durable cutover
receipt. No automatic fallback to an older journal after migration.
The physical gate requires fresh device-specific backup and live table/ID
comparison, RAM/latency/capacity measurements, unplug/reboot and recovery.
Production encryption/security provisioning is separate; this CSV alone
does not enable encryption. No real student data until approved.

References: ESP-IDF 5.4 partition table and NVS documentation:
https://docs.espressif.com/projects/esp-idf/en/v5.4/esp32p4/api-guides/partition-tables.html
https://docs.espressif.com/projects/esp-idf/en/v5.4.4/esp32p4/api-reference/storage/nvs_flash.html

## Internal journal target
`MathJournalTarget::DedicatedStaging` opens only an already initialized `pllearning` partition. It does not initialize or erase NVS, copy legacy progress, or authorize activation. The default learner continues to use the factory journal. Both targets share the 4096-record PLMB V1 ceiling; the staging namespace enumerates keys before replay to reject unexpected/out-of-range records. This is an upper bound, not promised usable capacity. Exact-copy migration and durable cutover policy are still required. No installation instructions are authorized by this candidate.

## Exact-copy staging migration
`MathJournalMigration::copy` accepts exclusively owned, recovered factory and dedicated staging journals. It preflights the entire source, compares exact existing record bytes including ink, adopts a legacy base only on an empty destination, then commits/readbacks the missing suffix. It verifies the complete destination again before reporting success. An interrupted copy requires destination recovery with `begin()`; a matching prefix resumes without rewriting. Uses existing journal buffers, not an in-memory image or large stack arrays. No source erase, boot selection, durable activation receipt or new partition initialization. Caller must prevent other NVS writers. Trailing record loss after reboot remains undetectable without a separate durable high-water marker. A successful staging copy is not authorization to activate storage.

## Sealed staging copy receipt
Explicit `copy_and_seal` records `copyv1` only after verified exact copy. V1 contains physical record count, base and terminal checkpoints, rolling CRC over length-framed record bodies excluding individual CRCs, and receipt CRC. Recovery requires exact receipt match against replayed history. A sealed destination is immutable to normal append/restore; a changed source is rejected. It detects missing tail while the receipt exists. A lost entire receipt cannot prove earlier completion. No authentication, active selector, activation, erase, or installation; the factory journal remains the learner default. An independent durable cutover policy remains required.
