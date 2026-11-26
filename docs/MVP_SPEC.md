# Tab5 Learning — MVP Specification

**Document status:** Proposed engineering baseline; educational thresholds and curriculum will be reviewed with a teacher before implementation.

## Objective

Prove that a single-child M5Stack Tab5 can deliver learning activities completely offline and that a teacher can manage, correct, and back up student work using only a USB-connected Windows laptop. Keep the working prototype independent of the prior dice-roller firmware.

## MVP Definition of Done

A build passes when all of the following can be demonstrated on the real device:

1. **First-use setup:** A teacher assigns one anonymous student ID and a starting grade. On later boots, the child resumes their own dashboard without selecting an account.
2. **Offline lessons:** At least one small math skill set runs without Wi-Fi, Bluetooth, or internet access, including a short initial assessment and repeated practice.
3. **Finger entry:** The child can draw digits on the touchscreen; the device records and displays strokes. Recognition may return a candidate answer or an explicit uncertain result.
4. **Fair scoring:** The child can confirm/correct an uncertain transcription before grading. The app assesses the math answer independently from penmanship.
5. **Local persistence:** The assigned student ID, question attempts, raw stroke traces, and derived mastery survive a reboot and ordinary power loss using recoverable storage.
6. **Adaptive behavior:** Correct and incorrect math attempts lead to explainable next-practice recommendations. Skill advancement is per skill, not locked to an entire grade.
7. **Protected teacher controls:** A teacher authentication gate protects correction and administration flows on device and in the Windows manager.
8. **USB-only transfer:** A Windows application identifies the connected Tab5 over a *validated* USB transport, exports student work, and applies an authorized correction. No network sync is implemented.
9. **Reconciliation:** The original answer is preserved; a correction adds an audit record and causes affected scores/mastery to be recalculated. Transfer is safe to retry.
10. **Backup and restore:** The teacher manager can save a versioned local backup and restore it with checks so work is not silently lost or duplicated.
11. **Security review:** Wireless features are disabled in the learner application. Unauthorized content imports are rejected. Firmware update protections and physical tamper risks are documented and tested to the extent supported by the hardware.
12. **Tests and release evidence:** Automated tests cover answer scoring, corrections, mastery recomputation, schema validation, duplicate USB operations, and corrupted/partial transfers; a manual hardware checklist documents touch and USB behavior.

## Expanded MVP acceptance gates (approved)

- Flashcards: teacher-authored text decks, CSV import, approval, individual assignment, offline review, durable results and authenticated USB round trip. Images/audio and other response modes use versioned content packages and must be validated before release.
- Reading: at least one licensed illustrated narrated story, synchronized highlighting, explicit recording and teacher-reviewed oral reading. Automated speech scoring remains experimental until validated.
- Spelling: at least one spoken word set with handwritten response and teacher correction.
- Teacher Studio: protected local login, deck and lesson creation, assignment, progress review and backup. No network dependency.
- Mandatory educational interaction journal, teacher drilldown, milestone evidence and correction-driven recalculation: see [Learning history](LEARNING_HISTORY.md). Test durable sequence ordering, idempotence, recovery and privacy controls.
- Shared learning history: reviewed errors can suggest practice cards; uncertain recognition and self-ratings must not become formal grades.
- Teacher-added supplemental content: configurable nonnegative extra-credit weights, separate core/bonus/combined grades, optional caps, audited corrections and no advancement solely from bonus points. See [EXTRA_CREDIT.md](EXTRA_CREDIT.md).
- Learner achievement journey: frequent meaningful micro-goals, current/progress/completed views and immediate nonblocking achievements with increasing difficulty and durable certificates only for designated major milestones, printable through Teacher Studio. See [ACHIEVEMENTS.md](ACHIEVEMENTS.md).
- Native standalone lifelong-learning architecture and one guided camera evidence activity, subject to hardware/privacy acceptance. See [LIFELONG_LEARNING.md](LIFELONG_LEARNING.md).
- The original twelve acceptance gates remain required for the engineering pilot. The pilot is not the shipping product. Full reviewed grades 1–4 curriculum coverage in the required subjects is mandatory for launch; production AI assessment is not required where a validated teacher-review fallback is available.

## Usability acceptance

The child and teacher workflows must pass the observed task gates in [USABILITY_AUDIT.md](USABILITY_AUDIT.md). Automated backend tests alone are insufficient.

## Deferred beyond initial launch

- Grade 5 and above curriculum content; optional enrichment beyond the approved grades 1–4 coverage.
- Full handwriting/cursive evaluation, sentence-level recognition, handwriting-quality scores.
- Classroom-wide reporting and fleet provisioning.
- Production secure-boot provisioning unless verified as supported by the exact Tab5 configuration.

## Progression policy

Subjects progress independently. Mastery is calculated from reviewed effective answers and spaced practice, not a single perfect handwriting score. The exact accuracy and repeat-session thresholds are **TBD with the teacher**; do not hard-code unapproved educational policy.

## Logic flow (pseudocode)

1. Load and validate one local student record; resume last saved lesson.
2. Select an available skill based on prerequisites and current mastery.
3. Present an offline question and capture finger strokes or alternate answer entry.
4. Recognize the candidate digits; ask for confirmation if ambiguous.
5. Store the original attempt first; compute effective answer, feedback, and mastery.
6. Periodically create recoverable local checkpoints.
7. When teacher connects a laptop by USB, authenticate and export immutable attempts.
8. Teacher reviews handwriting, submits a correction, and the device validates the authorized operation.
9. Append a correction record, recompute affected derived progress, acknowledge the operation id, and preserve both original and corrected views.

## Development sequence

1. Validate touch traces, storage durability, and the actual supported USB device transport.
2. Implement schema validation and local persistence with error handling and logs.
3. Implement one small math activity and robust correction-capable answer capture.
4. Implement teacher Windows USB manager and backup/restore.
5. Add adaptive selection rules and automated/hardware regression tests.
6. Evaluate the result with a teacher and revise requirements before expanding content.

## Expanded launch scope — November 24, 2026

The original twelve gates remain the first integration milestone, not the complete launch MVP. The [Master Development Tracker](https://chatgpt.com/space/page_02d1380881d08191a124e3becdbf3073) records the expanded scope. No feature below is certified by this specification change.

13. Offline read-along: locally stored narration and word timing, synchronized highlighting, pause/resume and replay.
14. Oral reading and pronunciation: capture local evidence, benchmark against teacher-reviewed examples, distinguish uncertainty from errors, provide teacher review where automatic scoring is unreliable.
15. Reading comprehension and spelling: offline assessments and skill-specific adaptive practice.
16. Teacher-authored lessons and flashcards: validate, preview, assign and transfer via authenticated USB; preserve existing progress.
17. Activity tracking: append-only attempt and correction history, persistent review schedule and explainable mastery.
18. Rewards: non-blocking achievements, progressively harder milestones and certificates for major accomplishments; no duplicate awards.
19. Native UI: consistent flex-card sizes and matching scratch-pad dimensions; validate touch, audio, USB and battery operation on the physical Tab5.
20. Curriculum: ship four complete grade-level learning paths (grades 1–4), with reviewed mathematics, English language arts (reading, writing and spelling), science and social studies. All four years of required core material must be available offline at initial delivery. Every grade/subject path includes progressively harder achievements and designated major accomplishment certificates. Advance independently by skill and subject. See [CURRICULUM_RELEASE.md](CURRICULUM_RELEASE.md); exact standards editions, alignment and mastery thresholds require educator review.

### Release evidence

Record commit SHA, test results, firmware artifact hash and relevant physical acceptance evidence for each gate. The first hardware integration milestone must pass before declaring launch readiness. No cloud speech processing or wireless synchronization is permitted. Do not claim a speech engine is viable until measured on the actual target. Preserve existing firmware and known battery initialization behavior when testing.

## Teacher website and maintenance recovery — interface update

The teacher manager is to expose a protected local browser interface (no internet dependency). Technical diagnostics, firmware repair and explicit factory reset are separate SSH maintenance-host operations using a physically attached Tab5 over USB. See [Teacher Studio and SSH maintenance contract](TEACHER_STUDIO_AND_RECOVERY.md) for roles, state schema, recovery distinctions and additional acceptance gates. This supersedes the desktop-only UI assumption without changing the USB-only transport or offline learner requirements. Implementation is pending.
