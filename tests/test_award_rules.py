import tempfile
import unittest
from pathlib import Path
from tools.award_rules import badge_candidates, certificate_candidate
from tools.award_ledger import AwardLedger


class AwardTests(unittest.TestCase):
    def test_progressive_and_no_farming(self):
        rows = [dict(event_id=f"e{i}", task_id=f"q{i}", subject="math", outcome="independent") for i in range(5)]
        self.assertEqual(len(badge_candidates(rows[:1]*30, "math", set())), 1)
        self.assertEqual(len(badge_candidates(rows, "math", set())), 2)
        self.assertEqual(len(badge_candidates(rows, "math", {"math-first_steps"})), 1)
        self.assertEqual(badge_candidates([{**rows[0], "outcome":"uncertain"}], "math", set()), [])

    def test_major_only(self):
        goal = dict(milestone_id="arithmetic", major=False, required_ids=["a","b"])
        self.assertIsNone(certificate_candidate(goal, {"a":"e1","b":"e2"}))
        goal["major"] = True
        self.assertIsNone(certificate_candidate(goal, {"a":"e1"}))
        self.assertEqual(certificate_candidate(goal, {"a":"e1","b":"e2"})["kind"], "certificate")

    def test_persistence_and_duplicate(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder)/"awards.sqlite"
            ledger = AwardLedger(path,"student1")
            candidate = dict(award_id="first",kind="badge",evidence_ids=["e1"])
            self.assertEqual(ledger.issue(candidate,1),(1,True))
            reopened = AwardLedger(path,"student1")
            self.assertEqual(reopened.issue(candidate,1),(1,False))
            self.assertEqual(len(reopened.portfolio()),1)
            with self.assertRaises(ValueError):
                reopened.issue({**candidate,"evidence_ids":["e2"]},1)
            with self.assertRaises(ValueError):
                AwardLedger(path,"student2")
