"""Portable regression tests; synthetic PCM is not a narration recording."""
import tempfile
import unittest
import wave
from pathlib import Path
from unittest.mock import patch
from tools import build_reading_narration as narration


class NarrationTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        for i in range(len(narration.WORDS)):
            self.write_word(i)

    def write_word(self, index, frames=17, rate=16000, channels=1):
        with wave.open(str(self.root / f"{index:02d}.wav"), "wb") as stream:
            stream.setnchannels(channels)
            stream.setsampwidth(2)
            stream.setframerate(rate)
            stream.writeframes(b"\x01\x00" * frames * channels)

    def test_cues_include_padding_and_gaps(self):
        result = narration.build(self.root)
        self.assertIn('{"A",0,2}', result)
        self.assertIn('{"little",102,104}', result)
        self.assertIn('{"tree.",918,920}', result)
        self.assertIn("narration_duration = 1020;", result)

    def test_reject_wrong_rate(self):
        self.write_word(0, rate=8000)
        with self.assertRaises(ValueError):
            narration.build(self.root)

    def test_reject_stereo(self):
        self.write_word(0, channels=2)
        with self.assertRaises(ValueError):
            narration.build(self.root)

    def test_reject_short_word(self):
        self.write_word(0, frames=15)
        with self.assertRaises(ValueError):
            narration.build(self.root)

    def test_padding_counts_toward_budget(self):
        with patch.object(narration, "MAX_SAMPLES", 16319):
            with self.assertRaises(ValueError):
                narration.build(self.root)
        with patch.object(narration, "MAX_SAMPLES", 16320):
            self.assertIn("narration_duration = 1020;", narration.build(self.root))

    def test_missing_word(self):
        (self.root / "09.wav").unlink()
        with self.assertRaises(FileNotFoundError):
            narration.build(self.root)


if __name__ == "__main__":
    unittest.main()
