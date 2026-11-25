import unittest

from tools.read_along import ReadAlong


class ReadAlongTests(unittest.TestCase):
    def setUp(self):
        self.player = ReadAlong([
            {"token_id": "a", "text": "The", "start_ms": 0, "end_ms": 100},
            {"token_id": "b", "text": "cat", "start_ms": 150, "end_ms": 300},
            {"token_id": "c", "text": "ran", "start_ms": 300, "end_ms": 400},
        ], 500)

    def test_pause_resume_and_finish(self):
        self.assertEqual(self.player.state.status, "READY")
        self.assertEqual(self.player.play().token_id, "a")
        self.assertIsNone(self.player.observe(125).token_id)
        self.assertEqual(self.player.pause(175).token_id, "b")
        self.assertEqual(self.player.observe(200).position_ms, 175)
        self.assertEqual(self.player.play().position_ms, 175)
        self.assertEqual(self.player.observe(500).status, "FINISHED")
        self.assertEqual(self.player.play().position_ms, 0)

    def test_word_replay_stops_at_boundary(self):
        state = self.player.replay_word("b")
        self.assertEqual((state.position_ms, state.replay_end_ms), (150, 300))
        self.assertEqual(self.player.observe(299).token_id, "b")
        state = self.player.observe(320)
        self.assertEqual((state.status, state.position_ms, state.token_id),
                         ("PAUSED", 300, None))
        self.assertEqual(self.player.play().token_id, "c")

    def test_pause_during_word_replay(self):
        self.player.replay_word("b")
        self.player.pause(200)
        self.assertEqual(self.player.play().replay_end_ms, 300)
        self.assertEqual(self.player.observe(300).status, "PAUSED")

    def test_invalid_input_preserves_state(self):
        self.player.play()
        self.player.observe(180)
        previous = self.player.state
        for position in (-1, 501, True, 179):
            with self.subTest(position=position), self.assertRaises(ValueError):
                self.player.observe(position)
            self.assertEqual(self.player.state, previous)
        with self.assertRaises(ValueError):
            self.player.replay_word("missing")
        self.assertEqual(self.player.state, previous)


if __name__ == "__main__":
    unittest.main()
