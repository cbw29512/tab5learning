import unittest
from tools.extra_credit import project_grade


def row(key="a", weight="10", earned="8", possible="10", status="reviewed"):
    return dict(assignment_id=key, maximum_bonus=weight, earned=earned,
                possible=possible, status=status)


class ExtraCreditTests(unittest.TestCase):
    def test_partial_and_missing(self):
        result = project_grade("85", [row(), row("b", earned=None, possible=None,
                               status="missing")], policy_version="v1")
        self.assertEqual(result["core"], "85.00")
        self.assertEqual(result["bonus_awarded"], "8.00")
        self.assertEqual(result["combined"], "93.00")

    def test_caps(self):
        self.assertEqual(project_grade(95, [row()], policy_version="v1")["combined"], "100.00")
        self.assertEqual(project_grade(95, [row()], policy_version="v1",
                                       cap_at_100=False)["combined"], "103.00")
        self.assertEqual(project_grade(85, [row()], policy_version="v1",
                                       aggregate_cap=3)["combined"], "88.00")

    def test_correction_and_zero_weight(self):
        before = project_grade(85, [row()], policy_version="v1")
        after = project_grade(85, [row(earned="4")], policy_version="v2")
        self.assertEqual(before["combined"], "93.00")
        self.assertEqual(after["combined"], "89.00")
        self.assertEqual(project_grade(85, [row(weight=0)], policy_version="v1")["combined"], "85.00")

    def test_invalid_and_duplicate(self):
        for records in ([row(), row()], [row(weight="-1")], [row(weight="NaN")],
                        [row(earned="11")], [row(possible=0)], [row(status="uncertain")]):
            with self.assertRaises(ValueError):
                project_grade(85, records, policy_version="v1")
