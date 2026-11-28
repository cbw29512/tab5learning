import tempfile
import unittest
from pathlib import Path

from tools.learning_journal import LearningJournal


class JournalTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.path = Path(self.tmp.name) / "history.sqlite"
        self.journal = LearningJournal(self.path, "device1", "student1")
        self.event = dict(schema_version=1, device_id="device1", student_id="student1",
                          session_id="session1", event_id="event1", elapsed_ms=100,
                          event_type="control_selected", control_id="math")

    def test_restart_and_retry(self):
        self.assertEqual(self.journal.append(self.event), 1)
        reopened = LearningJournal(self.path, "device1", "student1")
        self.assertEqual(reopened.append(self.event), 1)
        self.assertEqual(reopened.timeline(), [{"sequence": 1, **self.event}])

    def test_conflict_and_wrong_student(self):
        self.journal.append(self.event)
        for change in ({"control_id": "reading"}, {"student_id": "student2"}):
            with self.assertRaises(ValueError):
                self.journal.append({**self.event, **change})
        with self.assertRaises(ValueError):
            LearningJournal(self.path, "device1", "student2")
        self.assertEqual(len(self.journal.timeline()), 1)

    def test_order_and_page(self):
        self.journal.append(self.event)
        self.journal.append({**self.event, "event_id": "event2", "elapsed_ms": 0})
        self.assertEqual(self.journal.timeline(after=1)[0]["sequence"], 2)

    def test_reject_unrelated_payload(self):
        with self.assertRaises(ValueError):
            self.journal.append({**self.event, "password": "synthetic"})
        self.assertEqual(self.journal.timeline(), [])
