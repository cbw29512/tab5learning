# Pocket Learning — permanent development tracker
Source of truth: existing private repository, feature/tab5-ink-usb-probe, PR #1. Continue existing work; no replacement repo. Read MVP_SPEC, USABILITY_AUDIT, LIFELONG_LEARNING, USB_SECURITY, DATA_SCHEMA, LEARNING_HISTORY, TEACHER_AUTH and COMPETITIVE_DESIGN before changes.
Definition of done: installable native Tab5 firmware and Windows Teacher Studio, elementary pilot curriculum, offline single learner, authenticated USB, durable educational history, evidence-based progression, camera learning, usability/security/hardware acceptance. K-college capability is architectural; full curriculum coverage is later expansion.
Status vocabulary: specified / implemented / host-tested / hardware-tested / accepted. Host CI does not establish educational effectiveness, physical behavior or release readiness.
| ID | Deliverable | Current evidence / gap |
|---|---|---|
| M1 | Touch, USB, battery and camera hardware | Diagnostic source exists; physical acceptance outstanding |
| M2 | Durable educational event journal | Host tests passed; device persistence, time quality, evidence payloads and recovery pending |
| M3 | Teacher Studio | Local account/session/deck tests passed; visual editor integration, student permissions, screen lock pending |
| M4 | Flashcards | Host import/review/draft tests passed; device lesson and authenticated assignment pending |
| M5 | Math and spelling | Ink diagnostic only; actual exercises and assessment pending |
| M6 | Narrated reading | Timing validation foundation; licensed assets, playback, capture, review pending |
| M7 | Timeline and progress | Host projection tested; teacher drilldown and durable milestone transitions pending |
| M8 | Security/release | USB currently unauthenticated diagnostic only; no real student data; signing, encryption, retention, packaging and pilot gates pending |
| M9 | Competencies and standards | Graph and mastery recommendation host CI #44 passed; actual standards crosswalk, educator-approved policies and integration pending |
| M10 | Premium student UX | Navigation state host CI #47 passed; actual rendering, accessibility, audio, resume and user testing pending |
| M11 | Camera evidence | Specified; native capture and storage pending |
| M12 | Curriculum expansion | K-12/college capable model; coverage audit and later packages pending |
| M13 | Learner achievement journey | Host projection implemented; native UI, award history and acceptance pending; see ACHIEVEMENTS.md |
## Next vertical slice
Portable child navigation -> native rendering/touch -> one real activity -> durable evidence -> authorized teacher import -> review and recommendation. Preserve diagnostic build separately; do not ship diagnostic USB access. In parallel, wire visual editor and standards mapping.
## Handoff discipline
Every implementation session: inspect current branch, choose unfinished gate, commit functional changes with tests, check exact CI result, record evidence and blockers here. Do not invent completion percentages or use real student data. No formal grade promotion by software. Keep educational click history within retention/privacy rules.

## M13 extension — certificates and frequent rewards
Mandatory: short educational micro-goals, visible current/progress/completed achievements, instant nonblocking badges of increasing difficulty, certificates only for designated major milestones, persistent portfolio and authorized printable export. Distinguish activity completion from mastery. See ACHIEVEMENTS.md. Host badge/certificate candidate rules and persistent award ledger implemented (a89a7c0, 333b8f1, 554cbab); CI #62 passed. Prototype badge thresholds require educator review. Trusted evidence integration, append-only award review/status history, full certificate metadata, native notification rendering, print and acceptance pending. Portable notification controller and tests implemented (67a849a, 9fa96ec, 4f9b0e2); 2.5-second nonblocking lifetime, suppression and coalescing; CI #66 passed. Offline printable HTML certificate preview and tests implemented (1c6a824, 5a4cc9c); committed certificate lookup and learner/version checks. Trusted metadata, review status persistence, authorization, physical print/PDF validation and native UI remain pending.

## M14 — Teacher-controlled extra credit
Mandatory teacher-added content classification and configurable bonus weights, independent core mastery, audit history and supplemental certificates. Contract specified in EXTRA_CREDIT.md; decimal host grade projection and regression tests implemented (ce6417d, f905f39). CI #56 passed. Teacher authorization, scoped persistent assignments, audit revisions, native UI, certificates and acceptance pending.

## Native learner preview hardware gate
Historical hardware and vendor research: TAB5_HARDWARE_BASELINE.md. Separate learner_preview PlatformIO environment, native dashboard/navigation and shared landscape hit regions committed. Host layout tests added; actual firmware compile, image revision/partition validation, backup and hardware acceptance pending. No flash performed. Subject screens are placeholders; no real student data.

## Firmware compilation gate
Build-only PR workflow learner-preview-build.yml added at ed922f4; compiles learner_preview and archives binaries, package inventory and SHA256 evidence. No upload. Actual build result not yet confirmed. Battery gate in TAB5_HARDWARE_BASELINE.md is mandatory; recover known-good power source before physical flash. No approval to flash or use real student data.

## Mathematics preview and recovery
Five-question synthetic addition lesson wired to native touchscreen (ee52232); retry and independent-answer distinction. Portable versioned checkpoint codec with CRC and invariant validation (b06baca), corruption/restore tests (e91a212), both math suites wired into CI (fc9414f). Physical storage integration and power-loss validation pending; checkpoint summary is not immutable attempt history. No achievement issuance from unsaved preview state. Build run #3 for ee52232 was in progress at last check; host CI #83 passed before new math suites were wired. Battery gate remains mandatory.

## Native practice reward integration
Host CI #86 and #90 passed. NVS checkpoint adapter and save-before-acknowledge/resume implemented (0b6e0aa, 2f9ccd3). Native preview derives two practice badges from valid saved independent-answer counts (52282be), tests (9a86d29), native journey and 2.5-second notification integration (257b9ac), CI suite registration (592d1ee). Thresholds 1 and 5 are synthetic preview only; no mastery/certificates or durable full award ledger integration. Native build #6 was in progress before reward changes; new build and tests not yet verified. Hardware battery gate, storage failure/power-loss tests, handwriting, full event history and authorization remain blockers. No physical flash.

## Build reliability and battery source recovery
Host CI #95 passed. Native build #8 cancelled; job has no step details, root cause unconfirmed. Workflow changed cancel-in-progress to false (867ecc7) to let active compilation finish during rapid commits. GitHub may still replace pending builds. Recovered prior battery source and contract from Tab5LASR at af26727; documented in TAB5_HARDWARE_BASELINE.md. No power register changes until exact M5Unified driver reconciled; no hardware flash or battery certification.
