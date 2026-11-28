"""Generate a developer-only synthetic reading preview; not reviewed curriculum."""
import argparse
import logging
from pathlib import Path
import subprocess
import tempfile

from build_reading_narration import WORDS, build

LOG = logging.getLogger(__name__)


def generate(output: Path) -> None:
    # Keep intermediate audio out of the repository and student storage.
    with tempfile.TemporaryDirectory(prefix="tab5-narration-") as folder:
        root = Path(folder)
        raw = root / "raw.wav"
        for i, word in enumerate(WORDS):
            subprocess.run(
                ["espeak", "-v", "en-us", "-s", "130", "-w", str(raw), word],
                check=True, timeout=30,
            )
            subprocess.run(
                ["ffmpeg", "-v", "error", "-y", "-i", str(raw), "-ac", "1",
                 "-ar", "16000", "-c:a", "pcm_s16le",
                 str(root / f"{i:02d}.wav")],
                check=True, timeout=30,
            )
        # The existing builder validates PCM format, duration and cue bounds.
        content = build(root)
        output.write_text(content, encoding="utf-8")
    LOG.info("Generated synthetic preview: %s; educator review required", output)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    try:
        generate(args.output)
        return 0
    except (OSError, ValueError, EOFError, subprocess.SubprocessError) as exc:
        LOG.error("Preview narration generation failed: %s", exc)
        return 1


if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO)
    raise SystemExit(main())
