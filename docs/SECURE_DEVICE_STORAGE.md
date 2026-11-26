# Protected device credential storage candidate

Selected candidate: ESP-IDF flash-encryption-backed encrypted NVS, with
release-mode flash encryption and Secure Boot already enabled, dedicated
NVS partition plsecure (minimum 12 KiB) and encrypted NVS key partition
plseckeys (minimum 4 KiB). This is a conservative candidate, not approval
to change the existing physical layout or burn fuses. The factory layout
does not contain these partitions. HMAC/eFuse-backed NVS is an alternative
to evaluate if the pinned Arduino SDK cannot support the selected scheme.

State: unavailable -> prerequisites detected -> existing key config loaded
-> dedicated encrypted NVS initialized -> empty/valid/corrupt/unavailable.
No automatic generation, fallback, erase, eFuse changes or default NVS
access. An authorized maintenance caller may install a valid record exactly
once; existing different records conflict. Ambiguous writes use the existing
readback reconciliation. Key configuration scratch is cleared. Dedicated
single-task ownership is required; no other writer may access this namespace.

The isolated existing auth compile probe checks prerequisites only, without
initializing storage or writing keys. No learner integration or actual
credential installation. Hardware release boot chain, debug/download
restrictions, key custody, exact partition map, encrypted backup recovery,
signed application updates, and approved reversible test plan remain
mandatory. Do not use on the currently installed learner or flash without
separate authorization. Production student data remains prohibited.

SDK references (ESP-IDF 5.4.x):
- https://docs.espressif.com/projects/esp-idf/en/v5.4.2/esp32p4/api-reference/storage/nvs_encryption.html
- https://docs.espressif.com/projects/esp-idf/en/v5.4.2/esp32p4/security/flash-encryption.html
- https://docs.espressif.com/projects/esp-idf/en/release-v5.4/esp32p4/api-reference/storage/nvs_flash.html
