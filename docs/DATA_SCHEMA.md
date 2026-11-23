# Tab5 Learning — Data Schema and State Logic

**Status:** Conceptual contract to validate before coding. Serialized representation, field sizes, storage engine and retention policy remain to be selected after hardware testing.

## Record types

| Record | Required fields / intent |
| --- | --- |
| Device | `device_id`, `schema_version`, `content_version`, provisioning state |
| Student | `student_id` (anonymous), `starting_grade`, local preferences, created timestamp; exactly one active student per device |
| Subject | `subject_id`, name (math, reading, spelling), curriculum version |
| Skill | `skill_id`, `subject_id`, grade band, prerequisites, learning objective |
| Question | `question_id`, `skill_id`, prompt, answer format, authoritative answer/validation rule, curriculum version |
| Attempt | `attempt_id`, `student_id`, `question_id`, recorded answer, original recognition result, confidence/uncertain flag, reference to raw touch strokes, device-local ordering/timestamp |
| Stroke trace | `stroke_trace_id`, ordered stroke coordinates, contact boundaries, display transform/calibration metadata |
| Teacher correction | `correction_id`, `attempt_id`, corrected interpretation or grading result, reason, teacher identity/role, audit ordering/timestamp |
| Mastery | `student_id`, `skill_id`, policy version, calculated progress, last reviewed attempt, review schedule; derived and reproducible |
| Session | `session_id`, current skill/question, resume checkpoint, update timestamp |
| Sync operation | `operation_id`, `device_id`, type, payload digest, status, result/version to support safe retries |
| Content manifest | `content_version`, file hashes, compatible schema versions, publisher signature metadata for authorized imports |

No names, birthdays, photos, school identifiers, or other direct personal information are required by the core child profile. A teacher-side mapping to names, if eventually needed, needs a separately designed privacy policy.

## Invariants

- Each device has **zero or one** provisioned active student; replacing the student requires a teacher-authorized reset/archival procedure.
- Attempts are **append-only**. No teacher or recognizer may overwrite the captured original answer or raw stroke data.
- Corrections are **append-only audit events** and refer to an existing attempt.
- Effective grade = original result unless superseded by the latest validated correction; conflicting corrections require deterministic ordering and review.
- Mastery is **derived state**, recalculated from effective reviewed attempts using a versioned mastery policy. It is not an independent source of truth.
- Lesson content updates never delete or reset attempts, progress, and correction history.
- Every USB-import operation has a stable operation ID; repeating it cannot apply a correction twice.
- The device validates payload/schema versions, authorization, lengths and integrity before committing new state.
- Interrupted writes or USB disconnects cannot silently create partially accepted updates.

## State transitions

Device: `UNPROVISIONED -> ACTIVE -> ARCHIVED / REASSIGNED`, with protected teacher action for provisioning, archive and reassignment.

Attempt: `CAPTURING -> STORED -> REVIEWED`; optional `CORRECTED` is represented by a new correction event, not mutation of `STORED`.

USB operation: `RECEIVED -> VALIDATED -> APPLIED -> ACKNOWLEDGED`. Invalid operations go to `REJECTED`, with logged reasons; interrupted operations can be safely retried.

## Testing priorities

- Restart after an attempt never loses an already acknowledged write.
- Teacher correction changes effective grade and mastery without changing the original trace.
- Replayed corrections do not double-apply.
- Wrong device IDs, incompatible content versions, altered hashes, invalid signatures and malformed inputs are rejected.
- Restored backups retain all attempts and audit history.
