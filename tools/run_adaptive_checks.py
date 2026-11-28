"""Run the portable curriculum/adaptive regressions without GitHub Actions.

Requires Python 3 and a native C++17 compiler (g++ or clang++). No network,
dependency installation, firmware upload, SD access or student records.
"""
import argparse
import logging
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

LOG = logging.getLogger(__name__)
ROOT = Path(__file__).resolve().parents[1]

# Keep these test targets aligned with the portable CI curriculum gate.
# POSIX file/owner tests and USB/crypto tests remain in the full CI suite.
SUITES = (
    "curriculum_preview",
    "curriculum_resume",
    "curriculum_completion",
    "curriculum_progress",
    "curriculum_transition",
    "curriculum_skill_history",
    "curriculum_skill_writer",
    "curriculum_historical_recommendation",
    "learner_navigation",
    "learner_layout",
    "learner_display_policy",
)


def command_ok(args, timeout):
    """Return a failure on nonzero exit, launch error or timeout."""
    try:
        result = subprocess.run(args, cwd=ROOT, check=False, timeout=timeout)
        if result.returncode:
            LOG.error("Exit code %s: %s", result.returncode, args)
            return False
        return True
    except (OSError, subprocess.TimeoutExpired):
        LOG.exception("Command failed: %s", args)
        return False


def run_suite(compiler, suite, directory):
    """Build a fresh host executable; never execute a stale binary."""
    try:
        source = ROOT / "firmware" / "tests" / (suite + "_test.cpp")
        binary = Path(directory) / (suite + ".exe")
        if not source.is_file():
            LOG.error("Missing regression source: %s", source)
            return False
        flags = [
            compiler, "-std=c++17", "-Wall", "-Wextra", "-Werror",
            "-I" + str(ROOT / "firmware" / "tests" / "fakes"),
            "-I" + str(ROOT / "firmware" / "include"),
            str(source), "-o", str(binary),
        ]
        return command_ok(flags, 180) and command_ok([str(binary)], 60)
    except (OSError, ValueError):
        LOG.exception("Cannot execute suite %s", suite)
        return False


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler", default="g++", help="Native g++/clang++ executable")
    args = parser.parse_args(argv)
    logging.basicConfig(level=logging.INFO, format="%(levelname)s: %(message)s")
    try:
        compiler = shutil.which(args.compiler)
        if not compiler:
            LOG.error("Native C++17 compiler not found: %s", args.compiler)
            return 2
        # Verify generated catalog consistency using the existing exact test.
        failures = []
        if not command_ok(
            [sys.executable, "-m", "unittest", "discover", "-s", "tests", "-p", "test_generate_curriculum_preview.py"],
            60,
        ):
            failures.append("generated curriculum")
        with tempfile.TemporaryDirectory(prefix="tab5-adaptive-") as directory:
            for suite in SUITES:
                LOG.info("Testing %s", suite)
                if not run_suite(compiler, suite, directory):
                    failures.append(suite)
        if failures:
            LOG.error("FAILED: %s", ", ".join(failures))
            return 1
        LOG.info("PASS: %d native suites and generated curriculum check", len(SUITES))
        return 0
    except (OSError, ValueError):
        LOG.exception("Local regression runner stopped")
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
