# Reading host verification

2026-11-25: Python narration regression suite at PR #1 head `f8d315d13ed8b01de56be7a88b6a9bb49d5f5f88`.

Files retrieved through GitHub and reproduced locally; `git hash-object` verified both blob hashes:

- `tools/build_reading_narration.py`: `ce3bcf67bc91c5653fc0b762a22c614a27c97f9d`
- `tests/test_build_reading_narration.py`: `1606c3f3787f4395621729f7780ee5eb5fad4466`

Command: `python -m unittest discover -s tests -v` in isolated directory containing those two exact files.

Result: 6 tests passed (0.018 seconds). Covers cue boundaries, missing words, padded sample budget, short recordings, stereo and wrong sample rate. This is not the full repository suite or native firmware certification.

Direct Git clone failed DNS resolution; exact-head pull-request workflow lookup returned no runs. Next: validate C++ reading suites, package reviewed narration and integrate playback. Hardware acceptance remains outstanding.
