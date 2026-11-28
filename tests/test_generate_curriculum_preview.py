import unittest
from tools.generate_curriculum_preview import generate, SOURCE, TARGET

class GeneratedCurriculumTests(unittest.TestCase):
    def test_exact_generated_catalog(self):
        self.assertEqual(generate(SOURCE.read_bytes()), TARGET.read_text(encoding="utf-8"))
