# MVP alignment audit — 2026-11-25

Audited branch baseline: 4d5b30917c44fa62b0f6732a2ed154741d3dd36a, draft PR #1. 175 commits, 107 changed files. PR reported unmergeable; exact-head PR workflow lookup returned no runs. This is a document and changed-file inventory audit, not full source, security, or hardware certification.

## Scope reconciliation

The original math/ink integration pilot is not the complete product. Preserve the expanded launch gates in MVP_SPEC and the editable Master Development Tracker. Include grades 1–3 reading, math, spelling, science and social studies, with selected grade 4 skills unlocked by mastery. Science/social studies need explicit curriculum acceptance coverage; no expansion to secondary/college content before elementary MVP acceptance. Educator review is required for curriculum, mastery thresholds and assessment validity.

One child per device; Windows teacher administration through physical USB only; no wireless or routine SD transfers. Signed updates and authenticated corrections; preserve original evidence and progress. Equal-size flex cards and scratchpad; nonblocking achievements, progressively harder milestones and major certificates. No automatic grading from uncertain recognition. Camera remains a feasibility/privacy-gated activity, not a reason to delay core workflow integration.

## Evidence register

| Gate | Current state | Next acceptance evidence |
|---|---|---|
| M01 identity/offline | native integration incomplete | provision, reboot, isolation |
| M02 placement | partial | native multi-subject routing |
| M03 math/spelling | synthetic math journal; spelling incomplete | reviewed practice and handwriting |
| M04 reading | narration tools, playback and estimated highlight core | UI integration, pause/resume, measured timing |
| M05–06 oral assessment | pending | explicit bounded recording, consent, teacher review, benchmark |
| M07–08 lessons/cards | desktop and host slices | authenticated assignment, durable native review |
| M09 history | synthetic native journal | full events and correction audit |
| M10 teacher studio | local login/deck slice | student-scoped review and administration |
| M11 USB | diagnostic foundation | signed transactional import/export, replay rejection, recovery |
| M12 mastery | host foundation | reviewed evidence drives skill unlocks |
| M13 rewards | host rules and synthetic native badges | durable awards and printable major certificates |
| M14 UI | native preview | physical geometry/touch/accessibility |
| M15 curriculum | incomplete | reviewed grade 1–3 subject packages and grade 4 unlocks |
| M16 hardware | unverified | reproducible native build, backup, battery and power-loss acceptance |

## Development controls

1. Each session inspect current branch, select an incomplete gate, implement and test, commit, verify exact-head CI or execute portable tests, and record evidence in both trackers. No status-only run when a safe implementation is possible.
2. Prioritize vertical workflow integration: reading UI and timing, explicit recording and teacher review, secure USB round trip, adaptive native lessons, curriculum, rewards, physical acceptance.
3. Investigate unmergeable PR and missing CI evidence; no force push, merge or flash of unverified artifacts. Pin resolved board dependencies only after compatibility verification.
4. Preserve documented power initialization; do not guess register writes. Physical acceptance remains mandatory. No real student data before privacy/security gates.
5. Public repository license has unresolved owner placeholder; owner confirmation required before replacing it. Review dependency licenses and public exposure before distribution.
6. Refresh stale PR and repository tracker descriptions. No completion percentages inferred from commit count.

The audit does not establish any new passing tests or release acceptance.
