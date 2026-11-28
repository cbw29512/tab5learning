"""WSGI integration for independently protected synthetic mathematics history."""
import re
import unittest
from pathlib import Path

import test_teacher_web as harness
from test_math_backup_archive import snapshot
from tools.teacher_web import TeacherWeb


class StudentBackupWebTests(unittest.TestCase):
    def setUp(self):
        self.web = harness.TeacherWebTests()
        self.web.setUp()
        self.addCleanup(self.web.doCleanups)
        root = Path(self.web.temp.name)
        self.external = root / "external"
        self.external.mkdir()
        self.web.app = TeacherWeb(root / "student-test", test_mode=True, backup_dir=self.external)
        self.web.app.accounts.create("admin","1234","admin",bootstrap=True)
        self.csrf = self.web.login("admin","1234")
        self.token = self.web.cookie.split("=",1)[1]
        self.route = "/backups/students"
        for n in (1,2):
            self.assertEqual(self.web.call("/assignments",dict(csrf=self.csrf,
                student_id=f"student-{n}",device_id=f"tab5-{n}",grade="1"))[0],303)
        self.web.app.math_archive.save(self.token,"tab5-1","student-1",snapshot(3))

    def post(self, action, student="student-1", **kwargs):
        return self.web.call(self.route,dict(csrf=self.csrf,student=student,action=action,**kwargs))

    def test_backup_preview_confirmation_scope(self):
        self.assertIn("may be stale",self.web.call(self.route)[2])
        self.assertEqual(self.web.call(self.route,dict(csrf="bad",action="generate",student="student-1"))[0],403)
        page = self.post("generate")[2]
        key = re.search(r"<code>([a-f0-9]{64})</code>",page).group(1)
        proof = re.search(r'name="proof" value="([a-f0-9]{64})"',page).group(1)
        values = dict(proof=proof,recovery_key=key,repeat_key=key,saved_key="yes")
        self.assertEqual(self.post("save",student="student-2",**values)[0],403)
        page = self.post("save",**values)[2]
        self.assertIn("Verified mathematics archive saved",page)
        self.assertNotIn(key,page)
        name = re.search(r'pocket-student-test-[a-f0-9]{32}\.plse',page).group(0)
        self.assertTrue((self.external/"admin"/name).is_file())
        self.assertEqual(self.post("preview",student="student-2",filename=name,recovery_key=key)[0],403)
        self.assertIn("failed",self.post("preview",filename=name,recovery_key="0"*64)[2])
        page = self.post("preview",filename=name,recovery_key=key)[2]
        self.assertIn("Review student restore",page)
        self.assertNotIn(key,page)
        digest = re.search(r'name="digest" value="([a-f0-9]{64})"',page).group(1)
        proof = re.search(r'name="proof" value="([a-f0-9]{64})"',page).group(1)
        args = dict(filename=name,recovery_key=key,digest=digest,proof=proof)
        self.assertEqual(self.post("restore",student="student-2",**args)[0],403)
        self.assertIn("restored and verified",self.post("restore",**args)[2])
        self.assertEqual(self.web.app.math_archive.load(self.token,"tab5-1","student-1"),snapshot(3))
        self.assertEqual(self.web.call("/teachers",dict(csrf=self.csrf,username="other",password="5678"))[0],303)
        self.csrf = self.web.login("other","5678")
        self.assertNotIn("student-1",self.web.call(self.route)[2])
        self.assertEqual(self.post("preview",filename=name,recovery_key=key)[0],403)

    def test_no_saved_history_no_backup(self):
        page = self.post("generate",student="student-2")[2]
        key = re.search(r"<code>([a-f0-9]{64})</code>",page).group(1)
        proof = re.search(r'name="proof" value="([a-f0-9]{64})"',page).group(1)
        self.assertIn("failed",self.post("save",student="student-2",proof=proof,
            recovery_key=key,repeat_key=key,saved_key="yes")[2])
        self.assertEqual(list((self.external/"admin").glob("*.plse")),[])
