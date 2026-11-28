"""Versioned mastery recommendation using reviewed evidence, not grade promotion."""
import logging
from datetime import date

LOG = logging.getLogger(__name__)


def recommend(policy, evidence):
    """Caller supplies effective, authorized, single-skill assessment records.

    Evidence fields: attempt_id, task_id, session_id, day (trusted ISO date or
    None), outcome, transfer (validated novel task flag). Policy is teacher
    configured; this function does not grant mastery or authorize advancement.
    """
    try:
        if set(policy) != {"version", "accuracy_percent", "min_tasks", "min_sessions", "retention_days"}:
            raise ValueError("Invalid policy fields")
        if not isinstance(policy["version"], str) or not policy["version"].strip():
            raise ValueError("Policy version required")
        for key, low, high in (("accuracy_percent", 1, 100), ("min_tasks", 2, 10000),
                               ("min_sessions", 2, 1000), ("retention_days", 1, 365)):
            if type(policy[key]) is not int or not low <= policy[key] <= high:
                raise ValueError("Invalid policy threshold")
        seen, reviewed, pending = set(), [], 0
        for row in evidence:
            if set(row) != {"attempt_id", "task_id", "session_id", "day", "outcome", "transfer"}:
                raise ValueError("Invalid evidence fields")
            for key in ("attempt_id", "task_id", "session_id"):
                if not isinstance(row[key], str) or not row[key].strip():
                    raise ValueError("Missing evidence identity")
            if row["attempt_id"] in seen:
                raise ValueError("Duplicate effective attempt")
            seen.add(row["attempt_id"])
            if type(row["transfer"]) is not bool:
                raise ValueError("Invalid transfer flag")
            day = None if row["day"] is None else date.fromisoformat(row["day"])
            if row["outcome"] in ("uncertain", "self_known", "self_missed"):
                pending += 1
                continue
            if row["outcome"] not in ("independent", "assisted", "missed"):
                raise ValueError("Invalid outcome")
            reviewed.append((row, day))
        correct = [(row, day) for row, day in reviewed if row["outcome"] == "independent"]
        tasks = {row["task_id"] for row, _ in correct}
        sessions = {row["session_id"] for row, _ in correct}
        dated = [(row, day) for row, day in correct if day is not None]
        # Retention requires separate sessions on known dates, not mere retries.
        retention = False
        if dated:
            first, first_day = min(dated, key=lambda pair: pair[1])
            retention = any(row["session_id"] != first["session_id"] and
                            (day - first_day).days >= policy["retention_days"]
                            for row, day in dated)
        checks = {
            "accuracy": bool(reviewed) and len(correct)*100 >= len(reviewed)*policy["accuracy_percent"],
            "distinct_tasks": len(tasks) >= policy["min_tasks"],
            "distinct_sessions": len(sessions) >= policy["min_sessions"],
            "retention": retention,
            "transfer": any(row["transfer"] for row, _ in correct),
        }
        return {"policy_version": policy["version"], "recommend_review_for_advancement": all(checks.values()),
                "checks": checks, "reviewed": len(reviewed), "independent": len(correct),
                "pending_or_self_rated": pending,
                "evidence_ids": sorted(row["attempt_id"] for row, _ in correct)}
    except (ValueError, TypeError, KeyError, AttributeError):
        LOG.warning("Mastery recommendation rejected malformed input")
        raise
