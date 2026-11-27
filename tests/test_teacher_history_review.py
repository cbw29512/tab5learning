"""Read-only Teacher Studio review of verified original synthetic evidence."""
import struct
import unittest
from pathlib import Path

import test_teacher_web as harness
from test_math_backup_archive import snapshot, crc
from tools.math_backup_codec import decode, encode, MathBackup
from tools.teacher_web import TeacherWeb


class HistoryReviewTests(unittest.TestCase):
    def setUp(self):
        self.web = harness.TeacherWebTests()
        self.web.setUp()
        self.addCleanup(self.web.doCleanups)
        root = Path(self.web.temp.name)
        external = root / "external"
        external.mkdir()
        self.web.app = TeacherWeb(root / "history-test", test_mode=True, backup_dir=external)
        self.web.app.accounts.create("admin", "1234", "admin", bootstrap=True)
        self.csrf = self.web.login("admin", "1234")
        self.token = self.web.cookie.split("=",1)[1]
        for n in (1,2):
            self.assertEqual(self.web.call("/assignments", dict(csrf=self.csrf,
                student_id=f"student-{n}",device_id=f"tab5-{n}",grade="1"))[0],303)

    def review(self, student="student-1", page="1", **overrides):
        return self.web.call("/history", dict(csrf=self.csrf,student=student,page=page), **overrides)

    def test_exact_evidence_and_pagination(self):
        sample = decode(snapshot(11))
        first = sample.records[0]
        body = first[:6] + b"\x02\x00" + first[8:24]
        ink = struct.pack("<HHBHHB", 0,0,1,100,100,0)
        raw = encode(MathBackup(sample.base,sample.final,(crc(body+ink),)+sample.records[1:]))
        self.web.app.math_archive.save(self.token,"tab5-1","student-1",raw)
        status, headers, page = self.review()
        self.assertEqual(status,200)
        self.assertEqual(headers["Cache-Control"],"no-store")
        self.assertIn("Selected: 4",page)
        self.assertIn("Expected: 5",page)
        self.assertIn('points="0,0 100,100"',page)
        self.assertIn("Page 1 of 2",page)
        self.assertNotIn("<h2>Attempt 11</h2>",page)
        page = self.review(page="2")[2]
        self.assertIn("<h2>Attempt 11</h2>",page)
        self.assertNotIn("<h2>Attempt 1</h2>",page)
        self.assertEqual(self.web.app.math_archive.load(self.token,"tab5-1","student-1"),raw)

    def test_missing_scope_and_access(self):
        self.assertIn("No verified history available",self.review()[2])
        self.assertEqual(self.review(student="unknown")[0],403)
        self.assertEqual(self.review(page="-1")[0],400)
        self.assertEqual(self.review(HTTP_ORIGIN="https://evil.example")[0],403)
        self.assertEqual(self.web.call("/teachers",dict(csrf=self.csrf,username="other",password="5678"))[0],303)
        self.csrf = self.web.login("other","5678")
        self.assertEqual(self.review()[0],403)
        self.assertNotIn("student-1",self.web.call("/history")[2])

    def test_not_available_outside_preview(self):
        self.web.app = TeacherWeb(Path(self.web.temp.name)/"normal")
        self.web.app.accounts.create("admin","synthetic-admin-pass","admin",bootstrap=True)
        self.csrf = self.web.login()
        self.assertEqual(self.web.call("/history")[0],403)
        self.assertEqual(self.review()[0],403)
