"""Check that new literacy mappings point to actual draft evidence, not approval."""
import json
import unittest
from pathlib import Path
from tools.curriculum_draft import decode_draft

ROOT = Path(__file__).resolve().parents[1] / "curriculum" / "grade-1"

class CoverageV6Tests(unittest.TestCase):
    def test_v6_literacy_candidates(self):
        old = json.loads((ROOT / "coverage.v5.json").read_text(encoding="utf-8"))
        new = json.loads((ROOT / "coverage.v6.json").read_text(encoding="utf-8"))
        catalog, _ = decode_draft((ROOT / new["catalog"]).read_bytes())
        self.assertEqual(new["catalog_revision"], catalog["revision"])
        lessons = {x["lesson_id"]: x for x in catalog["lessons"]}
        self.assertEqual(len(lessons), 120)
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
        self.assertEqual(added, ["ELA.1.OE.4","ELA.1.C.1.1.b","ELA.1.C.2.1.a","ELA.1.C.2.1.b","ELA.1.C.2.1.c","ELA.1.C.3.1.a","ELA.1.C.3.1.b","ELA.1.C.3.1.c","ELA.1.C.4.1.a","ELA.1.C.4.1.c","ELA.1.C.4.1.d","ELA.1.C.4.1.e","ELA.1.C.4.1.g","ELA.1.C.4.1.h","ELA.1.C.6.1","ELA.1.C.8.1.a","ELA.1.C.8.1.b","ELA.1.C.9.1"])
        self.assertFalse(new["frameworks"]["english_language_arts"]["reconciled"])
