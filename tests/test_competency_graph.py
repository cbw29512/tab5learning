import unittest
from tools.competency_graph import eligible_skills, validate_catalog


def catalog():
    return {"catalog_id": "demo", "version": 1, "skills": [
        {"skill_id": "numbers", "subject": "math", "level_label": "K", "prerequisites": []},
        {"skill_id": "addition", "subject": "math", "level_label": "1", "prerequisites": ["numbers"]},
        {"skill_id": "advanced", "subject": "math", "level_label": "College demo", "prerequisites": ["addition"]},
        {"skill_id": "phonics", "subject": "reading", "level_label": "K", "prerequisites": []},
    ]}


class GraphTests(unittest.TestCase):
    def test_independent_subjects_and_levels(self):
        result = eligible_skills(catalog(), {"numbers"})
        self.assertEqual(set(result["available"]), {"addition", "phonics"})
        self.assertEqual(result["blocked"]["advanced"], ["addition"])
        self.assertIn("advanced", eligible_skills(catalog(), {"numbers", "addition"})["available"])

    def test_revocation(self):
        result = eligible_skills(catalog(), {"addition", "advanced"})
        self.assertEqual(result["needs_review"], ["addition", "advanced"])
        self.assertEqual(result["effective_mastery"], [])

    def test_missing_cycle_duplicate(self):
        for deps in (["unknown"], ["numbers"]):
            c = catalog()
            c["skills"][0]["prerequisites"] = deps
            with self.assertRaises(ValueError):
                validate_catalog(c)
        c = catalog()
        c["skills"].append(c["skills"][0])
        with self.assertRaises(ValueError):
            validate_catalog(c)

    def test_unknown_approval(self):
        with self.assertRaises(ValueError):
            eligible_skills(catalog(), {"unknown"})
