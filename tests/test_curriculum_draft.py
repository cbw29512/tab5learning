import copy
import hashlib
import json
import unittest
from pathlib import Path

from tools.curriculum_draft import decode_draft

SOURCE = Path(__file__).resolve().parents[1] / "curriculum" / "grade-1" / "starter.v1.json"


class CurriculumDraftTests(unittest.TestCase):
    def setUp(self):
        self.raw = SOURCE.read_bytes()
        self.catalog, self.digest = decode_draft(self.raw)

    def reject(self, change):
        doc = copy.deepcopy(self.catalog)
        change(doc)
        with self.assertRaises(ValueError):
            decode_draft(json.dumps(doc).encode())

    def test_starter_catalog(self):
        self.assertEqual(len(self.catalog["lessons"]), 4)
        self.assertEqual(len({x["subject"] for x in self.catalog["lessons"]}), 4)
        self.assertEqual(sum(len(x["activities"]) for x in self.catalog["lessons"]), 24)
        self.assertEqual(self.digest, hashlib.sha256(self.raw).hexdigest())

    def test_release_and_asset_claims_rejected(self):
        self.reject(lambda d: d.update(status="approved"))
        self.reject(lambda d: d.update(assets=["missing.mp3"]))
        self.reject(lambda d: d["lessons"][0].update(review_state="approved"))

    def test_bounds_and_evidence(self):
        self.reject(lambda d: d["lessons"][0]["activities"][1].update(answer_index=99))
        self.reject(lambda d: d["lessons"][1]["activities"][1].update(answer_index=0))
        self.reject(lambda d: d["lessons"][0]["activities"].pop())
        self.reject(lambda d: d["lessons"][0].update(prerequisites=["missing-skill"]))
        self.reject(lambda d: d["lessons"][0].update(prerequisites=[d["lessons"][0]["skill_id"]]))
        self.reject(lambda d: d["lessons"].append(d["lessons"][0]))
        with self.assertRaises(ValueError):
            decode_draft(b'{"schema_version":1,"schema_version":1}')
        with self.assertRaises(ValueError):
            decode_draft(b"x" * (1024*1024+1))

    def test_future_grade_label_supported(self):
        self.catalog["grade_label"] = "college-intro"
        for lesson in self.catalog["lessons"]:
            lesson["grade_label"] = "college-intro"
        self.assertEqual(decode_draft(json.dumps(self.catalog).encode())[0]["grade_label"], "college-intro")
