"""Portable review policy; self-ratings are not formal grades."""
import logging

LOG = logging.getLogger(__name__)


def next_review(streak, outcome, day):
    """Derive a practice interval from a trusted integer local day index.

    A zero interval means eligible later in the session; caller must cap retries.
    Uncertain evidence does not change streak or award mastery.
    """
    try:
        if type(streak) is not int or streak < 0:
            raise ValueError("Invalid streak")
        if type(day) is not int or day < 0:
            raise ValueError("Invalid day")
        if outcome == "uncertain":
            return {"streak": streak, "due_day": None, "needs_review": True}
        if outcome in ("missed", "self_missed"):
            new_streak, interval = 0, 0
        elif outcome in ("assisted", "self_known"):
            new_streak, interval = 0, 1
        elif outcome == "independent":
            new_streak = min(streak + 1, 3)
            interval = (3, 7, 30)[new_streak - 1]
        else:
            raise ValueError("Invalid outcome")
        return {"streak": new_streak, "due_day": day + interval,
                "needs_review": False}
    except (ValueError, TypeError):
        LOG.warning("Review policy rejected input")
        raise
