import json
import unittest
from pathlib import Path
from tools.curriculum_draft import decode_draft
from tools.teacher_curriculum_review import curriculum_page, SOURCE
from tools.teacher_web import TeacherWeb
import test_teacher_web as harness


class GradeOneLibraryTests(unittest.TestCase):
    def test_catalog_revisions_and_counts(self):
        old, _ = decode_draft(SOURCE.with_name("starter.v1.json").read_bytes())
        new, _ = decode_draft(SOURCE.read_bytes())
        self.assertEqual(new["revision"], 6)
        short_u = next(x for x in new["lessons"] if x["lesson_id"] == "g1-short-u")
        self.assertEqual(short_u["subject"], "english_language_arts")
        self.assertEqual(short_u["unit"], "phonics")
        self.assertEqual(short_u["prerequisites"], ["g1-short-i"])
        self.assertEqual(len(new["lessons"]), 44)
        self.assertEqual(sum(len(x["activities"]) for x in new["lessons"]), 264)
        self.assertEqual(old["lessons"], new["lessons"][:4])
        previous, _ = decode_draft(SOURCE.with_name("starter.v2.json").read_bytes())
        self.assertEqual(previous["lessons"], new["lessons"][:12])
        v3, _ = decode_draft(SOURCE.with_name("starter.v3.json").read_bytes())
        self.assertEqual(v3["lessons"], new["lessons"][:20])
        v4, _ = decode_draft(SOURCE.with_name("starter.v4.json").read_bytes())
        self.assertEqual(v4["lessons"], new["lessons"][:28])
        v5, _ = decode_draft(SOURCE.with_name("starter.v5.json").read_bytes())
        self.assertEqual(v5["lessons"], new["lessons"][:36])
        for subject in {l["subject"] for l in new["lessons"]}:
            self.assertEqual(sum(l["subject"] == subject for l in new["lessons"]), 11)
        self.assertEqual({x["subject"] for x in new["lessons"]}, {x["subject"] for x in old["lessons"]})

    def test_preview(self):
        self.assertIn("44 draft lessons", curriculum_page())
        page = curriculum_page("g1-tens-and-ones")
        self.assertIn("1.NR.1.2",page)
        self.assertIn("Four tens",page)
        with self.assertRaises(ValueError):
            curriculum_page("../../accounts.sqlite")

    def test_web_access(self):
        web = harness.TeacherWebTests()
        web.setUp()
        self.addCleanup(web.doCleanups)
        web.login()
        self.assertEqual(web.call("/curriculum")[0],403)
        web.app = TeacherWeb(Path(web.temp.name)/"curriculum", test_mode=True)
        web.app.accounts.create("admin","1234","admin",bootstrap=True)
        web.cookie = ""
        self.assertEqual(web.call("/curriculum")[0],403)
        web.login("admin","1234")
        self.assertIn("Review Grade 1 draft lessons",web.call()[2])
        self.assertIn("44 draft lessons",web.call("/curriculum")[2])
        self.assertIn("1.E.1",web.call("/curriculum/g1-goods-and-services")[2])
        self.assertEqual(web.call("/curriculum/unknown")[0],400)
