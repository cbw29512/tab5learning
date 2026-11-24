import unittest
from tools.achievement_view import achievement_view, CRITERIA


class AchievementTests(unittest.TestCase):
    def setUp(self):
        self.catalog = {"catalog_id": "demo", "version": 1, "skills": [
            {"skill_id": k, "subject": "math", "level_label": "1", "prerequisites": deps}
            for k, deps in [("a", []), ("b", ["a"]), ("c", ["b"])]
        ]}
        self.checks = {k: True for k in CRITERIA}

    def test_review_is_not_completion(self):
        view = achievement_view(self.catalog, {"a"}, {"b": self.checks})
        self.assertEqual(view["completed"][0]["skill_id"], "a")
        self.assertEqual(view["current"][0]["status"], "ready_for_review")
        self.assertEqual(view["upcoming"][0]["status"], "prerequisite_locked")

    def test_correction_requires_review(self):
        view = achievement_view(self.catalog, {"b"}, {})
        self.assertEqual(view["current"][0]["status"], "needs_review")
        self.assertEqual(view["completed"], [])

    def test_reject_unknown_evidence(self):
        with self.assertRaises(ValueError):
            achievement_view(self.catalog, set(), {"unknown": self.checks})
