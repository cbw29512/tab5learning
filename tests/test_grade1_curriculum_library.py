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
        self.assertEqual(new["revision"], 2)
        self.assertEqual(len(new["lessons"]), 12)
        self.assertEqual(sum(len(x["activities"]) for x in new["lessons"]), 72)
        self.assertEqual(old["lessons"], new["lessons"][:4])
        self.assertEqual({x["subject"] for x in new["lessons"]}, {x["subject"] for x in old["lessons"]})

    def test_preview(self):
        self.assertIn("12 draft lessons", curriculum_page())
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
        self.assertIn("12 draft lessons",web.call("/curriculum")[2])
        self.assertIn("1.E.1",web.call("/curriculum/g1-goods-and-services")[2])
        self.assertEqual(web.call("/curriculum/unknown")[0],400)
