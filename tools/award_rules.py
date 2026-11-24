"""Host reference: progressive badges from distinct durable learning evidence."""
import logging
from tools.competency_graph import valid_id

LOG = logging.getLogger(__name__)
# Prototype pacing, not a mastery policy. Curriculum authors must validate tiers.
TIERS = (("first_steps", 1), ("explorer", 5), ("adventurer", 15), ("hero", 30))


def badge_candidates(events, subject, earned_ids):
    """Trusted effective independent task records, already durably stored.

    Scope to a single learner and curriculum version before calling.
    Distinct task IDs prevent repeated clicks or retries farming badges.
    """
    try:
        if not valid_id(subject):
            raise ValueError("Invalid subject")
        if not isinstance(earned_ids, (set, frozenset)):
            raise ValueError("Expected earned ID set")
        tasks = {}
        for row in events:
            if set(row) != {"event_id", "task_id", "subject", "outcome"}:
                raise ValueError("Invalid award evidence")
            if any(not valid_id(row[k]) for k in ("event_id", "task_id", "subject")):
                raise ValueError("Invalid award reference")
            if row["outcome"] not in ("independent", "assisted", "missed", "uncertain", "self_known", "self_missed"):
                raise ValueError("Invalid outcome")
            if row["subject"] == subject and row["outcome"] == "independent":
                tasks.setdefault(row["task_id"], row["event_id"])
        result = []
        for tier, threshold in TIERS:
            award_id = f"{subject}-{tier}"
            if len(tasks) >= threshold and award_id not in earned_ids:
                result.append({"award_id": award_id, "kind": "badge",
                               "evidence_ids": list(tasks.values())[:threshold]})
        return result
    except (ValueError, TypeError, KeyError):
        LOG.warning("Badge evidence rejected")
        raise


def certificate_candidate(milestone, approved_evidence):
    """Only a designated major milestone with all approved requirements qualifies."""
    try:
        if set(milestone) != {"milestone_id", "major", "required_ids"}:
            raise ValueError("Invalid milestone")
        if not valid_id(milestone["milestone_id"]) or type(milestone["major"]) is not bool:
            raise ValueError("Invalid milestone identity")
        required = milestone["required_ids"]
        if not isinstance(required, list) or not required or any(not valid_id(k) for k in required) or len(set(required)) != len(required):
            raise ValueError("Invalid milestone requirements")
        if not isinstance(approved_evidence, dict) or any(not valid_id(k) or not valid_id(v) for k,v in approved_evidence.items()):
            raise ValueError("Invalid approved evidence")
        if not milestone["major"] or not set(required) <= approved_evidence.keys():
            return None
        return {"award_id": milestone["milestone_id"], "kind": "certificate",
                "evidence_ids": sorted({approved_evidence[k] for k in required})}
    except (ValueError, TypeError, KeyError):
        LOG.warning("Certificate evidence rejected")
        raise
