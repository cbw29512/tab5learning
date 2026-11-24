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
Mandatory: short educational micro-goals, visible current/progress/completed achievements, certificate for every completed milestone, persistent portfolio and authorized printable export. Distinguish activity completion from mastery. See ACHIEVEMENTS.md. Implementation, native display, print and acceptance pending.
