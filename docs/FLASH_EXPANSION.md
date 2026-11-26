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
