# MVP usability audit and release gates

## Scope and evidence
Source review of current device UI/main loop, USB probe, Teacher Studio, deck storage, learning event/progress modules, and MVP/reading/flashcard/history/data contracts. CI run 34 passed host tests, not a hardware or usability certification. This is a source-level workflow audit, not a complete security audit or observed user study.

## Child experience
One provisioned child per device; no login, typing, file browser or administrative controls in learning mode.
Home: Math, Reading, Spelling, Flashcards; daily practice/resume and personal journey. Consistent Home, Back, Help; text plus meaningful icons; spoken instructions and replay where available. One primary task at a time. No information conveyed by color alone. Target touch areas at least 12 mm on physical display, with spacing; validate on device. Support mute, volume, reduced motion and untimed practice. Save at meaningful transitions; interrupted work resumes or clearly offers restart. No public ranking, punitive streaks, ads, purchases or manipulative reward loops. Celebrate learning and effort, distinguish practice from verified mastery. Never grade speech/ink uncertainty as failure. Preserve original evidence within retention policy.

## Teacher experience
Local login -> dashboard -> Students / Create & Assign / Review Work / Connect Device.
No CSV required for ordinary authoring: front/back inputs, add/edit/remove, card preview, save confirmation, optional CSV import. Drafts are distinct from approved content; assignment requires approval and authorized student selection. Show exact device/student, pending changes, transfer progress, verified result and safe retry. No command-line dependency for routine tasks.
Progress summary first, drilldown to skill, lesson, session, semantic interaction, original answer and corrections. Review queue shows uncertain assessments, learning difficulties and sync failures; no automatic diagnosis. Last sync and freshness clearly labeled. Keyboard navigation, visible focus, readable text, specific recovery instructions. Teachers see only assigned students; administrators manage accounts. Avoid logging credentials and unrelated activity.

## Source gaps / priorities
P0: actual firmware still diagnostic with child-facing USB export; no integrated student navigation or persistence. USB remains unauthenticated read-only probe; no production student data. Host event references lack full timestamps, typed evidence and device persistence. Milestone count threshold is candidate only, not validated mastery.
P1: teacher UI is CSV-centric, no per-student authorization, no assignment, no drilldown or exception queue. Session expiration needs screen lock; no secure deployment or recovery yet. Reading contract scope stale. Deck versions immutable but editing workflow absent.
P2: accessible visuals/audio, personal journey, curriculum quality, packaging and performance.

## Acceptance tasks (proposed targets, not measured)
Use synthetic data first; supervised consented child pilot only after privacy/security review.
C1: child launches assigned device and starts an activity in at most two taps from home, without adult navigation help after orientation.
C2: child follows instructions, answers, uses hint, retries and finishes a short activity; no technical labels or dead ends.
C3: child returns home and resumes after restart without losing acknowledged work.
C4: child uses narration replay, mute and a non-color feedback cue; no forced speech grading.
T1: teacher creates five text cards, previews and saves within two minutes without CSV or coding.
T2: teacher selects student and assigns approved deck within three minutes; no cross-student access.
T3: teacher connects correct device, transfers deck and imports results through one guided workflow; disconnect and retry without duplication.
T4: teacher finds uncertain answer, opens original evidence, corrects it and sees updated milestone candidate within two minutes.
T5: teacher locates prior session and educational click, identifies content version and sees original plus correction; backup/restore preserves history.
Test with representative early readers and teachers, record completion, assistance, errors, time, accessibility observations and qualitative feedback. Proposed pilot gate: at least 90% independent completion per critical task in a representative formative sample, zero unresolved critical safety/privacy/data-loss issues; refine with educators, report sample and limitations. All tasks need actual Windows/Tab5 evidence. Host CI is necessary but insufficient.

## Build order
1. Visual teacher deck editor and preview; preserve optional CSV.
2. Portable child navigation state machine and tests, then integrate real display/touch.
3. Student enrollment and assignment authorization.
4. Device persistence and authenticated sync vertical slice.
5. Teacher review queue and history drilldown.
6. Reading/spelling/math activities, accessibility and personal progress.
7. Observed usability, hardware/security tests and installable release.

Track every gate as not tested, failed, or passed with evidence. No claim of full usability until observed tasks pass.
