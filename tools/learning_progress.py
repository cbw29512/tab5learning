"""Deterministic host-side progress projection from a complete ordered journal.

This is a reporting prototype, not an authorization boundary or validated
educational mastery policy. Caller supplies authenticated reviewed events.
"""
import logging
from collections import Counter

from tools.learning_event import canonical_event

LOG = logging.getLogger(__name__)
VERIFIED = {"independent", "assisted", "missed"}
ASSESSMENTS = {"assessment_recorded", "teacher_correction"}


def project_progress(events, required_correct, policy_version):
    """Rebuild effective outcomes and candidate milestones without editing history."""
    try:
        if type(required_correct) is not int or required_correct < 1:
            raise ValueError("Invalid candidate threshold")
        if not isinstance(policy_version, str) or not policy_version:
            raise ValueError("Policy version required")
        attempts, effective, counts = {}, {}, Counter()
        previous = 0
        identity = None
        for row in events:
            sequence = row["sequence"]
            # Require a complete journal, not an arbitrary filtered timeline.
            if type(sequence) is not int or sequence != previous + 1:
                raise ValueError("Missing or unordered journal sequence")
            event = {k: v for k, v in row.items() if k != "sequence"}
            canonical_event(event)
            current = (event["device_id"], event["student_id"])
            if identity is not None and current != identity:
                raise ValueError("Mixed student journals")
            identity, previous = current, sequence
            kind = event["event_type"]
            counts[kind] += 1
            if kind == "answer_submitted":
                aid = event.get("attempt_id")
                skill = event.get("skill_id")
                if not aid or not skill or aid in attempts:
                    raise ValueError("Invalid or duplicate attempt reference")
                attempts[aid] = skill
            elif kind in ASSESSMENTS:
                aid, outcome = event.get("attempt_id"), event.get("outcome")
                if aid not in attempts or not outcome:
                    raise ValueError("Assessment without original attempt")
                if event.get("skill_id") not in (None, attempts[aid]):
                    raise ValueError("Assessment skill mismatch")
                if kind == "teacher_correction" and aid not in effective:
                    raise ValueError("Correction without assessment")
                if kind == "assessment_recorded" and aid in effective:
                    raise ValueError("Duplicate assessment; use correction")
                effective[aid] = outcome
        skills = {}
        for aid, skill in attempts.items():
            summary = skills.setdefault(skill, {"attempts": 0, "independent": 0,
                "assisted": 0, "missed": 0, "pending": 0, "self_rated": 0,
                "evidence": []})
            summary["attempts"] += 1
            outcome = effective.get(aid)
            if outcome in VERIFIED:
                summary[outcome] += 1
                if outcome == "independent":
                    summary["evidence"].append(aid)
            elif outcome in ("self_known", "self_missed"):
                summary["self_rated"] += 1
            else:
                summary["pending"] += 1
        for summary in skills.values():
            summary["candidate_milestone"] = summary["independent"] >= required_correct
        return {"policy_version": policy_version, "last_sequence": previous,
                "event_counts": dict(counts), "skills": skills}
    except (ValueError, TypeError, KeyError):
        LOG.warning("Progress projection rejected invalid history")
        raise
