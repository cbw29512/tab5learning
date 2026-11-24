"""Learner-facing skill achievement projection, not an award authority."""
import logging
from tools.competency_graph import eligible_skills

LOG = logging.getLogger(__name__)
CRITERIA = {"accuracy", "distinct_tasks", "distinct_sessions", "retention", "transfer"}


def achievement_view(catalog, approved_mastery, checks_by_skill):
    """Checks must originate from trusted, version-matched mastery projections."""
    try:
        graph = eligible_skills(catalog, approved_mastery)
        known = {s["skill_id"] for s in catalog["skills"]}
        if not isinstance(checks_by_skill, dict) or not checks_by_skill.keys() <= known:
            raise ValueError("Unknown skill evidence")
        for checks in checks_by_skill.values():
            if not isinstance(checks, dict) or set(checks) != CRITERIA or any(type(v) is not bool for v in checks.values()):
                raise ValueError("Invalid achievement criteria")
        result = {"current": [], "completed": [], "upcoming": []}
        for skill in catalog["skills"]:
            key = skill["skill_id"]
            checks = checks_by_skill.get(key, {})
            met = sum(checks.values())
            row = {"skill_id": key, "subject": skill["subject"],
                   "criteria_met": met, "criteria_total": len(CRITERIA),
                   "missing_prerequisites": graph["blocked"].get(key, [])}
            if key in graph["effective_mastery"]:
                group, status = "completed", "mastered"
            elif key in graph["needs_review"]:
                group, status = "current", "needs_review"
            elif key in graph["blocked"]:
                group, status = "upcoming", "prerequisite_locked"
            elif met == len(CRITERIA):
                group, status = "current", "ready_for_review"
            elif checks:
                group, status = "current", "in_progress"
            else:
                group, status = "upcoming", "available"
            row["status"] = status
            result[group].append(row)
        return result
    except (ValueError, TypeError):
        LOG.warning("Achievement projection rejected")
        raise
