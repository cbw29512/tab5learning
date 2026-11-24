import unittest
from tools.mastery_policy import recommend


POLICY = dict(version="demo", accuracy_percent=90, min_tasks=2,
              min_sessions=2, retention_days=7)


def row(i, day, **extra):
    return dict(attempt_id=f"a{i}", task_id=f"q{i}", session_id=f"s{i}",
                day=day, outcome="independent", transfer=i == 2) | extra


class MasteryPolicyTests(unittest.TestCase):
    def test_retained_and_transferred(self):
        result = recommend(POLICY, [row(1, "2026-01-01"), row(2, "2026-01-08")])
        self.assertTrue(result["recommend_review_for_advancement"])

    def test_same_task_and_unknown_clock(self):
        result = recommend(POLICY, [row(1, None), row(2, None, task_id="q1")])
        self.assertFalse(result["checks"]["distinct_tasks"])
        self.assertFalse(result["checks"]["retention"])

    def test_correction_and_uncertainty(self):
        records = [row(1, "2026-01-01"), row(2, "2026-01-08", outcome="missed"),
                   row(3, "2026-01-09", outcome="self_known")]
        result = recommend(POLICY, records)
        self.assertFalse(result["recommend_review_for_advancement"])
        self.assertEqual(result["pending_or_self_rated"], 1)

    def test_duplicate_attempt(self):
        with self.assertRaises(ValueError):
            recommend(POLICY, [row(1, None), row(1, None)])
