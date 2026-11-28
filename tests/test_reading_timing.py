import unittest
from tools.reading_timing import NarrationTimeline, validate_timing


class ReadingTimingTests(unittest.TestCase):
    def test_valid(self):
        tokens = [{"token_id": "1", "text": "The", "start_ms": 0, "end_ms": 200},
                  {"token_id": "2", "text": "fox", "start_ms": 250, "end_ms": 500}]
        self.assertEqual(validate_timing(tokens, 600)[1], ("2", "fox", 250, 500))

    def test_invalid_intervals(self):
        for start, end in [(-1, 100), (100, 100), (0, 601), (False, 100)]:
            with self.subTest(start=start, end=end), self.assertRaises(ValueError):
                validate_timing([{"token_id": "1", "text": "fox",
                                  "start_ms": start, "end_ms": end}], 600)

    def test_duplicate_and_overlap(self):
        first = {"token_id": "1", "text": "The", "start_ms": 0, "end_ms": 200}
        for token_id, start in [("1", 200), ("2", 199)]:
            with self.assertRaises(ValueError):
                validate_timing([first, {"token_id": token_id, "text": "fox",
                                        "start_ms": start, "end_ms": 400}], 600)


class NarrationTimelineTests(unittest.TestCase):
    def setUp(self):
        self.timeline = NarrationTimeline([
            {"token_id": "1", "text": "The", "start_ms": 50, "end_ms": 200},
            {"token_id": "2", "text": "fox", "start_ms": 250, "end_ms": 500},
            {"token_id": "3", "text": "ran", "start_ms": 500, "end_ms": 600},
        ], 700)

    def test_boundaries_and_silence(self):
        for position, expected in [(0, None), (49, None), (50, "1"),
                                   (199, "1"), (200, None), (249, None),
                                   (250, "2"), (499, "2"), (500, "3"),
                                   (599, "3"), (600, None), (700, None)]:
            with self.subTest(position=position):
                cue = self.timeline.active_token(position)
                self.assertEqual(cue[0] if cue else None, expected)

    def test_replay_and_seek(self):
        start, end = self.timeline.replay_bounds("2")
        self.assertEqual((start, end), (250, 500))
        self.assertEqual(self.timeline.active_token(550)[0], "3")
        self.assertEqual(self.timeline.active_token(start)[0], "2")
        self.assertEqual(self.timeline.active_token(start)[0], "2")

    def test_invalid_inputs(self):
        for position in (-1, 701, True, 1.5, None):
            with self.subTest(position=position), self.assertRaises(ValueError):
                self.timeline.active_token(position)
        for token_id in ("missing", None):
            with self.assertRaises(ValueError):
                self.timeline.replay_bounds(token_id)


if __name__ == "__main__":
    unittest.main()
