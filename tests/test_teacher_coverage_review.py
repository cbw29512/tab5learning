import copy
import json
import unittest

from tools.teacher_coverage_review import SOURCE, validate_ledger, MAX_LEDGER_BYTES
from tools.teacher_curriculum_review import SOURCE as CATALOG_SOURCE, curriculum_page
from tools.curriculum_draft import decode_draft


class TeacherCoverageReviewTests(unittest.TestCase):
    def setUp(self):
        self.catalog, _ = decode_draft(CATALOG_SOURCE.read_bytes())
        self.ledger = json.loads(SOURCE.read_bytes())

    def validate(self, ledger):
        return validate_ledger(json.dumps(ledger).encode(), self.catalog, CATALOG_SOURCE.name)

    def test_inventory_and_read_only_page(self):
        self.assertEqual(len(self.validate(self.ledger)["rows"]), 135)
        before = SOURCE.read_bytes()
        page = curriculum_page("coverage")
        self.assertEqual(page.count("<details>"), 135)
        self.assertIn("0 approved", page)
        self.assertEqual(before, SOURCE.read_bytes())

    def test_reject_stale_and_fabricated_evidence(self):
        for key, value in (("catalog_revision", 15), ("catalog", "../other.json")):
            candidate = copy.deepcopy(self.ledger)
            candidate[key] = value
            with self.assertRaises(ValueError):
                self.validate(candidate)
        for key, value in (("lesson_id", "not-a-lesson"), ("revision", 9),
                           ("activity_ids", ["not-an-activity"])):
            candidate = copy.deepcopy(self.ledger)
            candidate["rows"][0]["evidence"][0][key] = value
            with self.assertRaises(ValueError):
                self.validate(candidate)

    def test_duplicate_and_unsupported_approval_rejected(self):
        candidate = copy.deepcopy(self.ledger)
        candidate["rows"].append(candidate["rows"][0])
        with self.assertRaises(ValueError):
            self.validate(candidate)
        for field, value in (("status", "approved"), ("educator_review", {"approved": True}),
                             ("hardware_acceptance", True), ("evidence", [])):
            candidate = copy.deepcopy(self.ledger)
            candidate["rows"][0][field] = value
            with self.assertRaises(ValueError):
                self.validate(candidate)

    def test_size_limit(self):
        with self.assertRaises(ValueError):
            validate_ledger(b" " * (MAX_LEDGER_BYTES + 1), self.catalog, CATALOG_SOURCE.name)
