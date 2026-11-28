"""Bounded draft curriculum catalog; validation is not release authorization."""
import hashlib
import json
import logging
import re

LOG = logging.getLogger(__name__)
MAX_BYTES = 1024 * 1024
MAX_LESSONS = 200
PHASES = ("instruction", "guided", "independent", "assessment", "remediation", "review")
SUBJECTS = {"mathematics", "english_language_arts", "science", "social_studies"}
ID = re.compile(r"[a-z0-9][a-z0-9_-]{0,79}\Z")


def fields(obj, expected):
    if type(obj) is not dict or set(obj) != set(expected.split()):
        raise ValueError("Unexpected catalog fields")


def string(value, limit=2000):
    if type(value) is not str or not value.strip() or len(value) > limit:
        raise ValueError("Invalid catalog text")
    if any(ord(c) < 32 for c in value):
        raise ValueError("Control character in catalog text")


def identifier(value):
    if type(value) is not str or not ID.fullmatch(value):
        raise ValueError("Invalid catalog identifier")


def integer(value, low, high):
    if type(value) is not int or not low <= value <= high:
        raise ValueError("Invalid catalog integer")


def array(value, low, high):
    if type(value) is not list or not low <= len(value) <= high:
        raise ValueError("Invalid catalog list length")


def activity(row):
    fields(row, "activity_id phase kind prompt choices answer_index feedback evidence")
    identifier(row["activity_id"])
    string(row["prompt"])
    if row["phase"] not in PHASES:
        raise ValueError("Unknown lesson phase")
    if row["kind"] == "choice":
        array(row["choices"], 2, 6)
        for option in row["choices"]:
            string(option, 200)
        if len(set(row["choices"])) != len(row["choices"]):
            raise ValueError("Repeated choice")
        integer(row["answer_index"], 0, len(row["choices"])-1)
        string(row["feedback"])
        if row["evidence"] != "selected_choice":
            raise ValueError("Choice evidence required")
    elif row["kind"] == "teacher_review":
        if row["choices"] != [] or row["answer_index"] is not None or row["evidence"] != "teacher_observation":
            raise ValueError("Teacher review must not have automatic scoring")
        string(row["feedback"])
    elif row["kind"] == "instruction":
        if row["choices"] != [] or row["answer_index"] is not None or row["evidence"] != "none" or row["feedback"] != "":
            raise ValueError("Instruction cannot award scores")
    else:
        raise ValueError("Unsupported draft activity")
    if row["phase"] == "assessment" and row["kind"] == "instruction":
        raise ValueError("Assessment requires evidence")


def lesson(row, grade):
    fields(row, "lesson_id revision grade_label subject unit skill_id prerequisites title objective standards minutes accessibility review_state review_date provenance activities")
    for key in ("lesson_id", "unit", "skill_id"):
        identifier(row[key])
    integer(row["revision"], 1, 2**31-1)
    integer(row["minutes"], 1, 180)
    if row["grade_label"] != grade or row["subject"] not in SUBJECTS:
        raise ValueError("Unexpected grade or subject")
    if row["review_state"] != "draft" or row["review_date"] is not None:
        raise ValueError("Draft validator cannot approve lessons")
    for key in ("title", "objective", "provenance"):
        string(row[key])
    array(row["prerequisites"], 0, MAX_LESSONS)
    for dep in row["prerequisites"]:
        identifier(dep)
    if len(set(row["prerequisites"])) != len(row["prerequisites"]):
        raise ValueError("Duplicate prerequisite")
    array(row["accessibility"], 1, 10)
    for note in row["accessibility"]:
        string(note, 500)
    array(row["standards"], 1, 20)
    for standard in row["standards"]:
        fields(standard, "framework edition id url mapping")
        for key in ("framework", "edition", "id", "url"):
            string(standard[key], 500)
        if not standard["url"].startswith("https://") or standard["mapping"] != "partial_draft":
            raise ValueError("Only draft standards mappings allowed")
    array(row["activities"], 6, 100)
    for item in row["activities"]:
        activity(item)
    if set(item["phase"] for item in row["activities"]) != set(PHASES):
        raise ValueError("Missing lesson phase")


def _unique(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise ValueError("Duplicate JSON field")
        result[key] = value
    return result


def decode_draft(raw):
    """Return validated draft and original-byte digest; never install or sign."""
    try:
        if type(raw) is not bytes or not 0 < len(raw) <= MAX_BYTES:
            raise ValueError("Invalid catalog size")
        doc = json.loads(raw.decode("utf-8"), object_pairs_hook=_unique)
        fields(doc, "schema_version package_id revision grade_label status assets lessons")
        integer(doc["schema_version"], 1, 1)
        identifier(doc["package_id"])
        integer(doc["revision"], 1, 2**31-1)
        string(doc["grade_label"], 32)  # Not hard-coded to Grades 1–4.
        if doc["status"] != "draft" or doc["assets"] != []:
            raise ValueError("Draft text-only schema; asset/signature support pending")
        array(doc["lessons"], 1, MAX_LESSONS)
        skills = {}
        lesson_ids, activity_ids = set(), set()
        for item in doc["lessons"]:
            lesson(item, doc["grade_label"])
            if item["lesson_id"] in lesson_ids or item["skill_id"] in skills:
                raise ValueError("Duplicate lesson or skill identifier")
            lesson_ids.add(item["lesson_id"])
            skills[item["skill_id"]] = set(item["prerequisites"])
            for step in item["activities"]:
                if step["activity_id"] in activity_ids:
                    raise ValueError("Duplicate activity identifier")
                activity_ids.add(step["activity_id"])
        # Draft V1 dependencies must be in the same catalog, and acyclic.
        remaining = set(skills)
        while remaining:
            ready = {skill for skill in remaining if not skills[skill] - (set(skills) - remaining)}
            if not ready:
                raise ValueError("Cyclic or missing prerequisite")
            remaining -= ready
        return doc, hashlib.sha256(raw).hexdigest()
    except (ValueError, TypeError, UnicodeError, RecursionError) as exc:
        LOG.warning("Draft curriculum validation rejected")
        raise ValueError("Invalid draft curriculum") from exc
