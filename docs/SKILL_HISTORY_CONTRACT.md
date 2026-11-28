# Skill history reference contract (SPE1)

Status: development-only codec, replay and abstract append writer. No firmware SD owner or
student-data write path is enabled. Keep CPL1 completion and CPR1 resume unchanged.

## Definition of done for persistent skill history

- Qualify isolated SD write/read/reboot and application restore first.
- Use one exclusive, validated owner and a separate evidence file; verify append,
  sync, exact length, byte readback and restart reconstruction.
- Preserve first independent answer per activity per numbered evidence session.
  Never award grades, completion or mastery from these samples.
- Preserve older revisions without mixing them into current-revision measures.
- Test unavailable storage, ambiguous writes, partial tails, duplicate responses,
  replay after reboot and capacity limits. Never erase corrupt evidence.
- Before real student data: approved retention, protected device assignment,
  authenticated teacher export, secure erasure/reassignment and educator review.
- Daily goals require trustworthy dates, a defined local-day policy and dated
  evidence. Session sequence numbers are not dates.

## Record layout

104 bytes, little-endian: SPE1 magic/version (4), contiguous record sequence
(4), contiguous nonempty session sequence (4), stable lesson ID (80),
lesson revision (4), activity ordinal (2), correctness (1), reserved zero (1),
CRC32 (4). The ordinal is interpreted only within the exact lesson revision.
The curriculum contract must increment revision when an activity changes.
No names, raw answer text, clock timestamps, scores or mastery classifications.

## State and limits

Replay starts unknown, validates every record, rejects malformed records,
duplicate lesson/revision/activity within one session and sequence gaps,
and publishes counts only after a complete read with unchanged length.
Current-revision observations, correct responses and number of contributing
sessions are separate counters. Historical revisions do not affect current
counts. CRC is accidental-corruption detection, not authentication.

Reference bounds: 4096 events, 256 unique first responses per session.
At capacity, fail closed; this is not a production retention policy.
The caller must exclude concurrent writers; length checks alone cannot
detect same-length external modifications. The future writer must allocate
session numbers durably, with no gaps from empty boots and explicit recovery
after uncertain writes. No calendar or adaptive promotion yet.

## Reference writer

One exclusive owner per evidence session; begin once, replay the exact file,
allocate the next session number only in memory, and commit it on the first
response. Admit only valid independent Activity-to-Feedback transitions, reject
practice/review and repeat observations. The backend must append one record and
sync. Verify expected byte length before and after, exact readback, then publish
incremental replay and mark the response observed. Any ambiguous append, readback
or length failure disables the owner; no retries or truncation. Controlled reopen
with a fresh owner reconstructs valid committed records. Capacity returns Full
without deleting anything. This abstract backend is not an ESP/FAT implementation
or authorization to collect real student data.

## Historical practice projection

Replay and incremental writer now count contributing sessions with errors and consecutive contributing sessions containing an error. Multiple errors within one session count once. A subsequent contributing session with only correct responses resets the current error streak; an unobserved session does not. The advisory policy suggests practice for recent errors and labels recurring difficulty only after at least four observations and two consecutive contributing error sessions. These are provisional development thresholds, not validated mastery metrics or dates. It ranks eligible practice within the requested subject, prioritizing recurring errors, then longer error streaks and stable catalog order. No history means no historical recommendation. No live student UI or SD connection enabled.
