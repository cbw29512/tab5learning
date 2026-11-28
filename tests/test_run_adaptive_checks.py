import unittest
from pathlib import Path
from unittest.mock import patch
from tools.run_adaptive_checks import main, run_suite, command_ok, SUITES


class AdaptiveRunnerTests(unittest.TestCase):
    def test_missing_compiler(self):
        with patch("tools.run_adaptive_checks.shutil.which", return_value=None):
            self.assertEqual(main([]), 2)

    def test_all_suites_and_generator(self):
        with patch("tools.run_adaptive_checks.shutil.which", return_value="g++"), \
             patch("tools.run_adaptive_checks.command_ok", return_value=True) as command, \
             patch("tools.run_adaptive_checks.run_suite", return_value=True) as suite:
            self.assertEqual(main([]), 0)
            self.assertEqual(suite.call_count, len(SUITES))
            self.assertEqual(command.call_count, 1)

    def test_failing_suite(self):
        with patch("tools.run_adaptive_checks.shutil.which", return_value="g++"), \
             patch("tools.run_adaptive_checks.command_ok", return_value=False), \
             patch("tools.run_adaptive_checks.run_suite", return_value=True):
            self.assertEqual(main([]), 1)

    def test_compile_failure_never_runs_executable(self):
        with patch("tools.run_adaptive_checks.command_ok", return_value=False) as command:
            self.assertFalse(run_suite("g++", "curriculum_preview", Path(".")))
            self.assertEqual(command.call_count, 1)

    def test_launch_failure_is_reported(self):
        with patch("tools.run_adaptive_checks.subprocess.run", side_effect=OSError("missing")):
            self.assertFalse(command_ok(["missing"], 1))
