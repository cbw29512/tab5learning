# Local teacher device registry

Prototype scope: synthetic data only. A teacher account can register a planned assignment of one anonymous student to one device. This is not device discovery, hardware authentication or provisioning.

## Schema and state
- Student ID and device ID: 1–64 ASCII alphanumeric, underscore or hyphen; normalized lowercase. No student names.
- Assignment: student_id (unique), device_id (unique), starting_grade (1–3), owner (authenticated teacher username), created_at (UTC), status (PLANNED).
- Every insertion is atomic. Unique constraints prevent both double assignment of a device and assignment of a student to multiple devices. Reject duplicates even across teachers without disclosing other records.
- Teacher sees only their own records; admin sees all. Owner comes from authenticated session, not form input.
- No reassignment or deletion in this slice. Future state transition PLANNED -> PROVISIONED requires authenticated device acknowledgement, and archival/reassignment requires explicit authorization and data protection.
- Test mode only, separate existing test database directory. No real student records or serial writes. Device IDs are manually entered labels, not verified hardware identities.

## Acceptance
Create and list a plan, preserve it across process restart, reject invalid grade/ID and duplicates, enforce role visibility, reject missing authorization and production-mode writes. Browser form must enforce existing CSRF checks and show planned status clearly. No firmware modification required.
