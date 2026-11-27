# Offline curriculum draft package V1

This is a **host-only draft catalog**, not an installable or approved curriculum. The existing DATA_SCHEMA, CURRICULUM_RELEASE, BETA_RELEASE and LEARNING_HISTORY contracts remain authoritative. Grade 1 closed beta first; commercial scope/pricing remain subject to approval.

## Schema and state
Package: stable package_id, schema_version, revision, grade_label, draft status, assets (empty in text-only V1), lessons. Lesson: stable lesson_id/revision, skill, prerequisites, grade, subject, unit, objective, minutes, standards framework/edition/ID/source/partial mapping, accessibility notes, provenance, educator review state/date and ordered activities. Activity: stable ID, phase, kind, prompt, options/answer index for choices, feedback and evidence mode. Teacher observation is ungraded until reviewed. Instruction never creates assessment evidence. Grade labels are not restricted to grades 1–4. Within-catalog dependencies must be present and acyclic. Revisions and IDs will anchor immutable attempts; publishing cannot overwrite historical lesson bytes.

Author draft → validate bounded schema and dependencies → educator/rights review → package versioned assets and hash manifest → publisher signature and compatible engine check → authorized USB staging → atomic activation → retain historical references. Only draft validation implemented. No signature verification, installation, persistent package catalog, asset resolution, entitlement, audio or device rendering in this slice. Do not treat draft validation as review/approval or claim coverage.

Starter at curriculum/grade-1/starter.v1.json contains four original partial lessons and six activities per lesson: instruction, guided practice, independent practice, assessment, remediation and review. Science and oral decoding require adult observation; no automatic speech scoring or mastery claim. National framework references are candidate partial mappings, not adoption by any state or educator approval. Full Grade 1 standards inventory and educator review still required. Text authored for this project; distribution rights review pending. No student information or external content copied.

Run from repo root:
```
python -m unittest discover -s tests -p "test_curriculum_draft.py" -v
```

Future: separate activity renderers, signed asset manifest with sizes/hashes, per-revision storage and content reference checks, USB staging and verified recovery, teacher approval and comprehensive standards inventory. Do not make unsupported activities available to learners.
