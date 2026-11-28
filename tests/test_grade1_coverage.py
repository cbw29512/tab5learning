"""Protect the initial draft coverage inventory from dangling evidence or false acceptance."""
import json
import unittest
from pathlib import Path
from tools.curriculum_draft import decode_draft

ROOT = Path(__file__).resolve().parents[1] / "curriculum" / "grade-1"


class GradeOneCoverageTests(unittest.TestCase):
    def test_baseline_ledger(self):
        ledger = json.loads((ROOT / "coverage.v1.json").read_text(encoding="utf-8"))
        self.assertEqual(ledger["grade"], "1")
        self.assertEqual(ledger["catalog"], "starter.v9.json")
        catalog, _ = decode_draft((ROOT / ledger["catalog"]).read_bytes())
        self.assertEqual(ledger["catalog_revision"], catalog["revision"])
        lessons = {lesson["lesson_id"]: lesson for lesson in catalog["lessons"]}
        keys = set()
        counts = {}
        for row in ledger["rows"]:
            key = (row["subject"], row["indicator"])
            self.assertNotIn(key, keys)
            keys.add(key)
            counts[key[0]] = counts.get(key[0], 0) + 1
            self.assertTrue(row["remaining"].strip())
            self.assertIsNone(row["educator_review"])
            self.assertIsNone(row["hardware_acceptance"])
            self.assertEqual(row["status"], "partial_draft" if row["evidence"] else "missing")
            for evidence in row["evidence"]:
                lesson = lessons[evidence["lesson_id"]]
                self.assertEqual(lesson["subject"], key[0])
                self.assertEqual(lesson["revision"], evidence["revision"])
                self.assertEqual(evidence["activity_ids"], [a["activity_id"] for a in lesson["activities"]])
        self.assertEqual(counts, {"mathematics":30,"english_language_arts":81,"science":9,"social_studies":15})
        self.assertTrue(all(not f["reconciled"] for f in ledger["frameworks"].values()))
