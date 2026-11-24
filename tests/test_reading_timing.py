import unittest
from tools.reading_timing import validate_timing


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


if __name__ == "__main__":
    unittest.main()
