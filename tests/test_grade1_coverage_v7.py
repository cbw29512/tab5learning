"""Check that new literacy mappings point to actual draft evidence, not approval."""
import json
import unittest
from pathlib import Path
from tools.curriculum_draft import decode_draft

ROOT = Path(__file__).resolve().parents[1] / "curriculum" / "grade-1"

class CoverageV7Tests(unittest.TestCase):
    def test_v7_social_studies_candidates(self):
        old = json.loads((ROOT / "coverage.v6.json").read_text(encoding="utf-8"))
        new = json.loads((ROOT / "coverage.v7.json").read_text(encoding="utf-8"))
        catalog, _ = decode_draft((ROOT / new["catalog"]).read_bytes())
        self.assertEqual(new["catalog_revision"], catalog["revision"])
        lessons = {x["lesson_id"]: x for x in catalog["lessons"]}
        self.assertEqual(len(lessons), 125)
        self.assertEqual([(x["subject"], x["indicator"]) for x in old["rows"]], [(x["subject"], x["indicator"]) for x in new["rows"]])
        added = []
        for before, after in zip(old["rows"], new["rows"]):
            self.assertIsNone(after["educator_review"])
            self.assertIsNone(after["hardware_acceptance"])
            if after["subject"] != "social_studies":
                self.assertEqual(before, after)
            if before["status"] == "missing" and after["status"] == "partial_draft":
                added.append(after["indicator"])
            self.assertEqual(after["status"], "partial_draft" if after["evidence"] else "missing")
            for e in after["evidence"]:
                lesson = lessons[e["lesson_id"]]
                self.assertEqual(lesson["subject"], after["subject"])
                self.assertEqual(e["revision"], lesson["revision"])
                self.assertEqual(e["activity_ids"], [a["activity_id"] for a in lesson["activities"]])
        self.assertEqual(added, ["1.H.2","1.G.2","1.G.3","1.G.4","1.CG.2"])
        self.assertFalse(new["frameworks"]["social_studies"]["reconciled"])
