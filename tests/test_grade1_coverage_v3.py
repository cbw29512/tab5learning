"""Check that new literacy mappings point to actual draft evidence, not approval."""
import json
import unittest
from pathlib import Path
from tools.curriculum_draft import decode_draft

ROOT = Path(__file__).resolve().parents[1] / "curriculum" / "grade-1"

class CoverageV3Tests(unittest.TestCase):
    def test_v3_literacy_candidates(self):
        old = json.loads((ROOT / "coverage.v2.json").read_text(encoding="utf-8"))
        new = json.loads((ROOT / "coverage.v3.json").read_text(encoding="utf-8"))
        catalog, _ = decode_draft((ROOT / new["catalog"]).read_bytes())
        self.assertEqual(new["catalog_revision"], catalog["revision"])
        lessons = {x["lesson_id"]: x for x in catalog["lessons"]}
        self.assertEqual(len(lessons), 90)
        self.assertEqual([(x["subject"], x["indicator"]) for x in old["rows"]], [(x["subject"], x["indicator"]) for x in new["rows"]])
        added = []
        for before, after in zip(old["rows"], new["rows"]):
            self.assertIsNone(after["educator_review"])
            self.assertIsNone(after["hardware_acceptance"])
            if after["subject"] != "english_language_arts":
                self.assertEqual(before, after)
            if before["status"] == "missing" and after["status"] == "partial_draft":
                added.append(after["indicator"])
            self.assertEqual(after["status"], "partial_draft" if after["evidence"] else "missing")
            for e in after["evidence"]:
                lesson = lessons[e["lesson_id"]]
                self.assertEqual(lesson["subject"], after["subject"])
                self.assertEqual(e["revision"], lesson["revision"])
                self.assertEqual(e["activity_ids"], [a["activity_id"] for a in lesson["activities"]])
        self.assertEqual(added, ["ELA.1.F.1.2","ELA.1.F.1.4","ELA.1.F.1.5","ELA.1.F.3.4.b","ELA.1.F.3.4.c","ELA.1.F.3.7","ELA.1.F.3.8.b","ELA.1.F.3.8.d","ELA.1.F.3.8.e","ELA.1.F.3.8.g","ELA.1.F.3.8.h","ELA.1.F.3.8.i","ELA.1.F.3.8.n"])
        self.assertFalse(new["frameworks"]["english_language_arts"]["reconciled"])
