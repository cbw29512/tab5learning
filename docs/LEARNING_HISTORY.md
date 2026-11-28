# Learning history and milestones — MVP acceptance contract

## Objective
Teachers can drill from subject to skill to lesson to individual educational interaction, and reconstruct what was presented, attempted, reviewed and corrected. One student per device, offline, authenticated USB only.

## Data and state
Event envelope: schema_version, device_id, student_id, session_id, event_id, durable sequence, optional UTC timestamp, clock_quality, monotonic elapsed_ms, event_type, content_id/version, skill_id, attempt_id, bounded typed payload. Use sequence as authoritative ordering; clock resets must not reorder events.
Event types: session start/end/resume, screen opened, semantic control selected, lesson started/completed, prompt shown, answer submitted, hint requested, retry, card revealed/rated, narration started/paused/repeated, recording started/stored, assessment recorded, teacher correction, assignment, milestone achieved/revoked, content updated. Do not capture raw clicks outside learning UI, credentials, background audio, or unrelated device activity.
Milestone: milestone_id, student_id, skill_id, criteria_version, evidence attempt IDs, status, achieved sequence, optional timestamp. Milestones derive from reviewed evidence; corrections can revoke or restore a milestone with an audit event. Self ratings and uncertain recognition do not count as verified mastery.
Flow: capture -> validate -> durable append -> acknowledge -> derive report -> authenticated export. Incomplete sessions recover without inventing completion. Duplicate event IDs with identical content are idempotent; conflicting content rejected. Track missing sequence ranges and storage failures explicitly; never silently drop events. Keep content version and original answer references.

## Teacher reporting
Class overview, individual student timeline, subject/skill mastery, lesson drilldown, question detail with original answer and corrections, flashcard review history, reading accuracy/fluency when validated, active learning time excluding idle periods, milestones and evidence. Filters by subject, date, session, skill, assignment and event type. Export authorized reports locally. No public rankings or automatic high-stakes decisions.

## Privacy and retention
Educational interaction logging only. No continuous microphone, location, raw touch coordinates outside answer canvases, passwords or names in event payloads. Explicit recording indicator. Role-based teacher access to assigned students; encryption and retention/deletion policy validated before pilot. Use pseudonymous student IDs and synthetic fixtures only in GitHub. Authorized deletion removes associated sensitive payloads and backups according to retention policy; retain only permitted non-identifying audit evidence. No hidden cloud telemetry.

## Acceptance tests
Replay events to reproduce reports; correct an answer and verify milestone recalculation; test duplicate IDs, conflicting IDs, missing sequences, clock rollback, reboot, power loss, full storage, USB disconnect, version changes, wrong student, unauthorized export and deletion. Verify teacher can inspect each educational control interaction and original evidence. Measure storage and active-time accuracy on hardware.

## Development milestones
M1 hardware touch/USB verified; M2 durable event journal and recovery; M3 teacher login and deck authoring; M4 flashcards and USB round trip; M5 math/spelling; M6 narrated reading and recording; M7 teacher timeline and milestone reconciliation; M8 integrated security and pilot acceptance. Each requires commit, tests and hardware evidence where relevant. Do not mark complete on specification alone.

## Independent archive
Student backup must preserve original evidence, corrections, sequence and exact content versions; teacher content backup must exclude student-specific records. Follow TEACHER_BACKUP_RESTORE.md. Never silently replace newer history with an older snapshot.
