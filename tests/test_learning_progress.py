import unittest
from tools.learning_progress import project_progress


def event(seq, kind, **extra):
    return dict(sequence=seq, schema_version=1, device_id="device1",
                student_id="student1", session_id="session1",
                event_id=f"event{seq}", elapsed_ms=seq,
                event_type=kind, **extra)


class ProgressTests(unittest.TestCase):
    def test_correction_revokes_candidate_without_erasing_history(self):
        rows = [event(1, "answer_submitted", attempt_id="a", skill_id="addition"),
                event(2, "assessment_recorded", attempt_id="a", outcome="independent")]
        self.assertTrue(project_progress(rows, 1, "test")["skills"]["addition"]["candidate_milestone"])
        rows.append(event(3, "teacher_correction", attempt_id="a", outcome="missed"))
        result = project_progress(rows, 1, "test")["skills"]["addition"]
        self.assertFalse(result["candidate_milestone"])
        self.assertEqual(result["missed"], 1)
        self.assertEqual(rows[1]["outcome"], "independent")

    def test_uncertain_and_self_rating_excluded(self):
        rows = [event(1, "answer_submitted", attempt_id="a", skill_id="reading"),
                event(2, "assessment_recorded", attempt_id="a", outcome="uncertain"),
                event(3, "answer_submitted", attempt_id="b", skill_id="reading"),
                event(4, "assessment_recorded", attempt_id="b", outcome="self_known")]
        result = project_progress(rows, 1, "test")["skills"]["reading"]
        self.assertEqual((result["pending"], result["self_rated"]), (1, 1))
        self.assertFalse(result["candidate_milestone"])

    def test_reject_gap_and_orphan(self):
        with self.assertRaises(ValueError):
            project_progress([event(2, "session_start")], 1, "test")
        with self.assertRaises(ValueError):
            project_progress([event(1, "teacher_correction", attempt_id="a", outcome="missed")], 1, "test")

    def test_reject_mixed_student(self):
        rows = [event(1, "session_start"), event(2, "session_end")]
        rows[1]["student_id"] = "student2"
        with self.assertRaises(ValueError):
            project_progress(rows, 1, "test")
