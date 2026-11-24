# Flashcards — approved product direction

## Definition of done
A teacher signs into the local Windows manager, creates or imports a deck, previews and approves it, assigns it to a student or class, and transfers it to each device over authenticated USB. The student practices offline, resumes after reboot, and the teacher retrieves results. Preserve the existing math MVP gate.

## Data contract
- Deck: immutable deck_id, version, title, subject, grade band, author, asset manifest, license/attribution.
- Card: card_id, deck version, skill IDs, prompt text/media, answer text/media, response mode (self-review, choice, numeric ink, spelling ink, oral), accepted answers, optional hint.
- Assignment: assignment_id, student_id, deck version, teacher_id, sequence, optional due date.
- Review event: attempt_id, student_id, card_id/version, response, assessment source, outcome (independent, assisted, missed, uncertain, self-rated), sequence, optional timestamp, policy version.
- Review state: derived next-due date/session, interval, streak, last reviewed event. Rebuild from append-only events and teacher corrections.

## State and policy
SELECT -> PROMPT -> RESPONSE -> FEEDBACK -> DURABLE REVIEW -> NEXT. Self-rating is practice evidence, not a formal grade. Uncertain handwriting or speech goes to review, not automatic failure. Proposed intervals: missed later in same session, assisted next day, independent 3 days, repeated success 7 then 30 days; teacher configurable, not validated educational thresholds. Bound retries and daily workload. When clock unreliable, use session ordering and flag calendar due dates for reconciliation.

## Authoring and distribution
Teacher creates front/back cards, images, audio, hints and accepted answers; imports bounded CSV (spreadsheet export). Local AI may draft content but requires teacher approval. Preserve asset rights. Classroom assignment expands to individually authorized USB transfers, no network. Deck updates retain historical attempts and pin old versions as needed. No real student data in repository. Admin credentials and signing keys remain outside GitHub.

## Adaptive integration
Reading, spelling and math can suggest practice cards from reviewed errors; keep source skill and attempt provenance. Students can practice built-in or assigned decks. Home screen: Math, Reading, Spelling, Flashcards; library accessible within Reading. Daily practice shows due items without punitive streaks or rankings.

## Tests
Reject malformed decks, duplicate IDs, invalid assets and unauthorized imports. Verify restart recovery, duplicate USB operation handling, old-version reviews, teacher corrections, bounded retries, clock loss, self-rating separation, and no wireless services. Hardware and recognition quality remain unverified.
