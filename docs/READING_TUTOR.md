# Offline reading tutor — approved design direction
Status: design contract; hardware and recognition quality not yet verified. Required MVP slice: one narrated illustrated story, recording and teacher review. Full reading library and validated automatic scoring remain later expansion.

## Definition of done
- Student selects a leveled illustrated book, hears narration with synchronized word highlighting, then reads the same passage aloud.
- Device stores original audio and resumes after reboot; recording is visible and limited to explicit reading sessions.
- Feedback distinguishes recognized words, possible errors and uncertain recognition. Never grade uncertainty as a reading error.
- Teacher PC imports recordings through authenticated USB, runs offline assessment and allows corrections before returning personalized practice.
- Independent reading without prior narration measures transfer; comprehension is assessed separately from fluency.
- Repeated practice targets reviewed skill weaknesses across reading and spelling.
- No network access, accounts, ads or mandatory subscriptions. One assigned student per device.

## Data contract
Extends DATA_SCHEMA.md; all IDs immutable and versioned.
| Record | Fields |
| --- | --- |
| Book | book_id, version, level, title, attribution, source URL, license, asset hashes |
| Passage | passage_id, book version, ordered token IDs, displayed text, audio asset, ordered start/end timestamps, skill IDs |
| Reading attempt | attempt_id, student_id, passage version, mode (modeled/independent), audio hash/reference, duration_ms, sequence, capture status |
| Assessment | assessment_id, attempt_id, model/version, policy version, aligned token results, confidence, review state |
| Token result | token_id, observed text if available, status (recognized/substitution/omission/insertion/uncertain), audio offsets, confidence |
| Review | correction_id, assessment_id, corrected token results, teacher identity, reason, sequence |
| Comprehension | existing question/attempt schema, passage version |
| Mastery | existing derived skill record; reviewed results only |

State: SELECT -> LISTEN -> READY -> RECORD -> STORE -> PENDING_ASSESSMENT -> FEEDBACK -> PRACTICE. Independent mode bypasses LISTEN. Interrupted recording is incomplete, not failed. Assessments and corrections append; original audio remains unchanged until authorized retention deletion. Export retries idempotent. No live microphones outside explicit recording. Audio retention, consent, encryption and deletion procedures must be approved before child pilot; no real student data in GitHub.

## Scoring
Use correct words per minute only on eligible timed passages with reviewed alignment, word accuracy with an explicit denominator, and separate comprehension. Do not infer phoneme mastery from a word substitution alone. Accept dialect variation and self-corrections under a versioned teacher-approved policy. Suppress scores when capture quality or alignment is insufficient. Never label a speech disability or rank children publicly.

## Implementation sequence
1. Portable book manifest and timestamp validation with synthetic fixtures.
2. Hardware spike: audio playback, word highlighting and microphone capture, measured RAM/CPU and battery.
3. Durable recordings and USB export on existing security contract.
4. Windows offline speech model evaluation using licensed models and consented representative samples; compare with teacher annotations.
5. Experimental constrained device-side alignment only if measured accuracy and resource budgets permit; no forced matching that hides reading mistakes.
6. Adaptive review, phonics practice and independent transfer checks.
7. Curated books: per-asset commercial redistribution rights, attribution and adaptation notices; narration voice/model rights verified separately.

## Release tests
Invalid timestamps, missing assets, corrupted audio, recording interruption, USB disconnect, duplicate correction, wrong student/device, uncertain words, substitutions/insertions/omissions, self-correction, dialect variation, replayed model versions, content update preserving attempts, retention deletion, and no wireless services. Compare child-speech recognition against human-reviewed references before enabling any automatic scoring. Use synthetic fixtures in CI; never store real child recordings in repository.
