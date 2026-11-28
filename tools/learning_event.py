"""Bounded semantic event envelope for the host-side journal prototype."""
import json
import logging
import re

LOG = logging.getLogger(__name__)
ID = re.compile(r"[A-Za-z0-9_-]{1,64}\Z")
# Allow only explicit educational events and reference fields, never arbitrary telemetry.
KINDS = {
    "session_start", "session_end", "session_resume", "screen_opened",
    "control_selected", "lesson_started", "lesson_completed", "prompt_shown",
    "answer_submitted", "hint_requested", "retry", "card_revealed", "card_rated",
    "narration_started", "narration_paused", "narration_repeated",
    "recording_started", "recording_stored", "assessment_recorded",
    "teacher_correction", "assignment", "milestone_achieved", "milestone_revoked",
    "content_updated",
}
REQUIRED = {"schema_version", "device_id", "student_id", "session_id",
            "event_id", "elapsed_ms", "event_type"}
OPTIONAL = {"content_id", "content_version", "skill_id", "attempt_id",
            "control_id", "evidence_id", "outcome"}
OUTCOMES = {"independent", "assisted", "missed", "uncertain", "self_known", "self_missed"}


def canonical_event(event):
    """Reject unknown fields and return a deterministic serialized envelope."""
    try:
        if not isinstance(event, dict) or not REQUIRED <= event.keys() <= REQUIRED | OPTIONAL:
            raise ValueError("Invalid event fields")
        if type(event["schema_version"]) is not int or event["schema_version"] != 1:
            raise ValueError("Unsupported schema")
        if not isinstance(event["event_type"], str) or event["event_type"] not in KINDS:
            raise ValueError("Unknown event type")
        for key in event.keys() - {"schema_version", "elapsed_ms", "event_type", "outcome", "content_version"}:
            if not isinstance(event[key], str) or not ID.fullmatch(event[key]):
                raise ValueError("Invalid reference")
        if type(event["elapsed_ms"]) is not int or not 0 <= event["elapsed_ms"] <= 2**63-1:
            raise ValueError("Invalid elapsed time")
        if "content_version" in event and (type(event["content_version"]) is not int or not 1 <= event["content_version"] <= 2**31-1):
            raise ValueError("Invalid content version")
        if "outcome" in event and (not isinstance(event["outcome"], str) or event["outcome"] not in OUTCOMES):
            raise ValueError("Invalid outcome")
        return json.dumps(event, sort_keys=True, separators=(",", ":"))
    except (ValueError, TypeError):
        LOG.warning("Learning event rejected")
        raise
