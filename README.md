# Tab5 Learning

An offline-first educational device for **one child per M5Stack Tab5**, with a **Windows teacher manager connected only by USB**.

> **Status:** Planning and requirements baseline. No firmware, teacher application, USB protocol, or handwriting recognition implementation is claimed to be working yet.

## Goals

- Children learn independently without Wi-Fi, Bluetooth, cloud accounts, or a school network.
- A device boots directly into its assigned child's dashboard and preserves learning progress across restarts.
- Grade 1–3 math, reading, and spelling learning paths are the Version 1 goal; selected grade 4 skills unlock independently as prerequisite skills are mastered.
- Handwritten numerical answers can be reviewed and corrected without penalizing children for recognition mistakes.
- A teacher uses a protected Windows application and a USB data cable to review results, correct assessments, update lessons, and back up records.
- All original project materials are proprietary. See [LICENSE](LICENSE).

## MVP versus full Version 1

**MVP:** Prove one complete end-to-end learning flow: first-time single-child setup, short starting assessment, offline math practice, finger-written numeric answer capture, a recognition/confirmation workflow, local persistence, teacher review/correction over a physical USB connection, and recalculation of progress after a correction. Any uncertain handwriting recognition must allow manual confirmation; accuracy must be evaluated on the actual hardware.

**Version 1 expansion (not an MVP promise):** Curriculum coverage across grades 1–3 in math, reading, and spelling; fourth-grade skill challenges; additional learning games; richer handwriting instruction; teacher report exports for a classroom.

## Architecture and specifications

- [MVP Definition of Done and implementation phases](docs/MVP_SPEC.md)
- [Data schema and state transitions](docs/DATA_SCHEMA.md)
- [USB-only management and security model](docs/USB_SECURITY.md)

## Development guardrails

1. Agree on the Definition of Done and data schema **before writing code**.
2. Validate Tab5 USB device communication and touch coordinate capture on real hardware before relying on them.
3. Keep student records, real names, credentials, keys, and generated reports **out of GitHub**.
4. Separate curriculum content from student progress so future content updates cannot erase learning history.
5. Use small, testable modules (split source files over roughly 150 lines), error handling and meaningful logging.

## Ownership

Copyright © 2026 **[OWNER_FULL_LEGAL_NAME]**. All rights reserved.

The owner's legal name is intentionally a placeholder until confirmed. The repository is intended to remain private; this README and the proprietary notice do not make it open source.
