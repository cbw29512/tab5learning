# Tab5 Learning

An offline-first educational device for **one child per M5Stack Tab5**, with a **Windows teacher manager connected only by USB**.

> **Status:** Pre-pilot development. See the specification and master tracker for release gates. Portable ink capture and read-only USB diagnostics are described in [PR #1](https://github.com/cbw29512/tab5learning/pull/1); native firmware and physical acceptance are not certified.

## Goals

- Children learn independently without Wi-Fi, Bluetooth, cloud accounts, or a school network.
- A device boots directly into its assigned child's dashboard and preserves learning progress across restarts.
- Version 1 ships complete reviewed grades 1–4 learning paths in mathematics, English language arts, science and social studies; skills unlock independently as prerequisites are mastered.
- Handwritten numerical answers can be reviewed and corrected without penalizing children for recognition mistakes.
- A teacher uses a protected Windows application and a USB data cable to review results, correct assessments, update lessons, and back up records.
- All original project materials are proprietary. See [LICENSE](LICENSE).

## Initial integration milestone versus launch MVP

**Initial integration milestone:** Prove one complete end-to-end learning flow: first-time single-child setup, short starting assessment, offline math practice, finger-written numeric answer capture, a recognition/confirmation workflow, local persistence, teacher review/correction over a physical USB connection, and recalculation of progress after a correction. Any uncertain handwriting recognition must allow manual confirmation; accuracy must be evaluated on the actual hardware.

**Launch MVP:** Complete the integration milestone plus four full offline grade-level curricula (grades 1–4) in mathematics, English language arts, science and social studies, offline read-along, pronunciation and reading assessment with teacher-review fallback, custom lessons, flashcards, activity history, achievements and major milestone certificates. See the [Master Development Tracker](https://chatgpt.com/space/page_02d1380881d08191a124e3becdbf3073) and MVP specification for acceptance gates.

**Later expansion:** Additional learning games, grades 5 and above, richer handwriting instruction and classroom-wide reporting.

## Architecture and specifications

- [MVP Definition of Done and implementation phases](docs/MVP_SPEC.md)
- [Four-year curriculum release requirements](docs/CURRICULUM_RELEASE.md)
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

## Next release milestone
The first controlled beta targets a complete Grade 1 curriculum and integrated teacher/student workflows. See [Grade 1 beta acceptance checklist](docs/BETA_RELEASE.md). Later grade content is not a beta blocker; commercial packaging and pricing remain undecided.
