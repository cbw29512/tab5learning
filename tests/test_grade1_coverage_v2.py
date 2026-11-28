"""Check v10 evidence and all formerly missing math indicators without claiming approval."""
import json
import unittest
from pathlib import Path
from tools.curriculum_draft import decode_draft

ROOT = Path(__file__).resolve().parents[1] / "curriculum" / "grade-1"

class CoverageV2Tests(unittest.TestCase):
    def test_v2_math_candidates(self):
        old = json.loads((ROOT / "coverage.v1.json").read_text(encoding="utf-8"))
        new = json.loads((ROOT / "coverage.v2.json").read_text(encoding="utf-8"))
        catalog, _ = decode_draft((ROOT / new["catalog"]).read_bytes())
        self.assertEqual(new["catalog_revision"], catalog["revision"])
        lessons = {x["lesson_id"]: x for x in catalog["lessons"]}
        self.assertEqual(len(lessons), 78)
        self.assertEqual([(x["subject"], x["indicator"]) for x in old["rows"]], [(x["subject"], x["indicator"]) for x in new["rows"]])
        for before, after in zip(old["rows"], new["rows"]):
            self.assertIsNone(after["educator_review"])
            self.assertIsNone(after["hardware_acceptance"])
            if after["subject"] == "mathematics":
                self.assertEqual(after["status"], "partial_draft")
                self.assertTrue(after["evidence"])
            else:
                self.assertEqual(before, after)
            for e in after["evidence"]:
                lesson = lessons[e["lesson_id"]]
                self.assertEqual(lesson["subject"], after["subject"])
                self.assertEqual(e["revision"], lesson["revision"])
                self.assertEqual(e["activity_ids"], [a["activity_id"] for a in lesson["activities"]])
        self.assertFalse(new["frameworks"]["mathematics"]["reconciled"])
