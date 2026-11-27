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

## Expanded MVP records (logical contract)

| Record | Required fields / intent |
| --- | --- |
| Lesson revision | lesson_id, revision, skill_ids, activity_ids, asset manifest, assignment scope |
| Passage revision | passage_id, revision, locale, ordered token IDs and text, narration asset digest, duration_ms |
| Narration cue | token_id, start_ms, end_ms; ordered, non-overlapping, bounded by duration |
| Oral assessment | attempt_id, passage revision, optional local evidence reference, observed tokens, confidence, review status, policy version |
| Flashcard revision | card_id, revision, skill_id, prompt, expected answer |
| Review event | event_id, student_id, card revision, result, sequence; derived due state |
| Activity event | event_id, student_id, session_id, kind, monotonic sequence, related record ID |
| Reward definition | reward_id, criteria version, type (achievement, milestone, certificate), prerequisite IDs |
| Reward grant | student_id, reward_id, criteria version, qualifying evidence IDs, earned sequence |
| Lesson assignment | assignment_id, student_id, lesson revision, authorized operation ID |

### State and invariants

Read-along: READY -> PLAYING -> PAUSED / FINISHED; asset or timing validation failure -> ERROR. Position is bounded by narration duration. Resume uses audio playback position as the source of truth, not elapsed wall-clock time. Highlighting is derived from validated cues.

Oral assessment: CAPTURED -> PENDING_REVIEW -> REVIEWED. A low-confidence or missing observation is not a confirmed error. Original observations remain immutable; teacher corrections append review events. Audio retention is separately configured, local-only and minimized; deletion of optional audio must not silently delete assessment history.

Flashcards: append review events before updating derived review schedules. An interrupted or repeated event cannot advance the schedule twice. Content revisions must not erase previous reviews.

Rewards: derive eligibility from reviewed evidence, grant once per student/reward/criteria version, and persist before displaying notifications. Achievements are non-blocking; certificates require explicit major milestone criteria. Exact educational thresholds remain configurable and teacher-reviewed.

USB imports: validate lesson revisions, asset hashes, cue bounds, student scope and signatures before activation. Never replace historical content references with an unrelated revision.

### Additional acceptance tests

- Missing narration, invalid cue intervals, overlapping cues and duration overflow are rejected.
- Pause/replay/resume maintain correct highlighting, including gaps between words.
- Uncertain speech observations are routed to review rather than graded as definite errors.
- Lesson revisions and repeated review events preserve prior progress.
- Replayed reward grants are idempotent; certificates cannot be issued for an ordinary achievement.
- Student reassignment does not expose another child's historical data.

This contract specifies logical fields, not a selected embedded storage engine or speech model. Native memory and audio feasibility must be measured on the physical Tab5.

## Draft curriculum resume checkpoint CPR1
One 96-byte canonical little-endian CRC32-protected blob in separate default NVS namespace `plcurriculum`, key `resume1`: magic/version 4, zero-padded stable lesson ID 80, lesson revision u32, activity index u16, finished flag, reserved zero, CRC32. Store only the last active location, not assessment answers, per-lesson mastery or formal grades. Save on lesson start, advance and completion before visible transition, exact retry no write, feedback transient (repeat same activity if interrupted). Opening Curriculum resumes last active/finished lesson; choosing another replaces the location. Missing is empty; damaged or unsupported revision fails closed, never erased or overwritten by preview. Commit/readback error stops advancement and requires restart/reconciliation. Default NVS shared physical capacity remains limited; one small blob only, not a substitute for approved microSD evidence storage, independent backup or signed curriculum updates.
