"""Check that new literacy mappings point to actual draft evidence, not approval."""
import json
import unittest
from pathlib import Path
from tools.curriculum_draft import decode_draft

ROOT = Path(__file__).resolve().parents[1] / "curriculum" / "grade-1"

class CoverageV4Tests(unittest.TestCase):
    def test_v4_literacy_candidates(self):
        old = json.loads((ROOT / "coverage.v3.json").read_text(encoding="utf-8"))
        new = json.loads((ROOT / "coverage.v4.json").read_text(encoding="utf-8"))
        catalog, _ = decode_draft((ROOT / new["catalog"]).read_bytes())
        self.assertEqual(new["catalog_revision"], catalog["revision"])
        lessons = {x["lesson_id"]: x for x in catalog["lessons"]}
        self.assertEqual(len(lessons), 100)
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
        self.assertEqual(added, ["ELA.1.F.3.8.j","ELA.1.F.3.8.k","ELA.1.F.3.8.l","ELA.1.F.3.8.m","ELA.1.F.3.8.o","ELA.1.F.4.1","ELA.1.F.4.2","ELA.1.F.4.3.b","ELA.1.F.4.3.c"])
        self.assertFalse(new["frameworks"]["english_language_arts"]["reconciled"])
