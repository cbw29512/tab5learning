import tempfile
import unittest
from pathlib import Path
from tools.award_ledger import AwardLedger
from tools.certificate_print import certificate_html


class CertificatePrintTests(unittest.TestCase):
    def test_committed_only_and_escaping(self):
        with tempfile.TemporaryDirectory() as folder:
            ledger = AwardLedger(Path(folder)/"awards.db", "learner1")
            ledger.issue(dict(award_id="first",kind="badge",evidence_ids=["e1"]),1)
            ledger.issue(dict(award_id="math",kind="certificate",evidence_ids=["e2"]),1)
            data = dict(learner_id="learner1",name="A < B",title="Math & Science",
                        award_id="math",version=1,completion_kind="verified_mastery",
                        completion_date="2026-11-24",status="issued")
            html = certificate_html(ledger,2,data)
            self.assertIn("A &lt; B",html)
            self.assertIn("Math &amp; Science",html)
            self.assertIn("not an accredited qualification",html)
            for seq in (1,3):
                with self.assertRaises(ValueError):
                    certificate_html(ledger,seq,data)
            for change in ({"learner_id":"other"},{"version":2},{"completion_date":"bad"}):
                with self.assertRaises(ValueError):
                    certificate_html(ledger,2,{**data,**change})
            self.assertIn("Under Review",certificate_html(ledger,2,{**data,"status":"under_review"}))
