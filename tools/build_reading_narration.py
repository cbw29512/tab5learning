"""Build a C++ PCM asset from ten reviewed local word WAV files."""
import argparse
import logging
from pathlib import Path
import wave

LOG = logging.getLogger(__name__)
WORDS = ("A", "little", "fox", "ran", "home.", "It", "sat", "by", "a", "tree.")
RATE = 16000
GAP = 1600
MAX_SAMPLES = RATE * 20


def build(source: Path) -> str:
    """Use recorded sample boundaries, not estimated text durations."""
    import struct
    samples = []
    cues = []
    for i, word in enumerate(WORDS):
        with wave.open(str(source / f"{i:02d}.wav"), "rb") as audio:
            n = audio.getnframes()
            if (audio.getnchannels(), audio.getsampwidth(), audio.getframerate(),
                audio.getcomptype()) != (1, 2, RATE, "NONE"):
                raise ValueError(f"Word {i}: expected mono PCM16 at {RATE} Hz")
            if n < 16 or len(samples) + n + GAP > MAX_SAMPLES:
                raise ValueError(f"Word {i}: invalid sample length")
            raw = audio.readframes(n)
            if len(raw) != n * 2:
                raise ValueError(f"Word {i}: truncated audio")
        start = len(samples)
        samples.extend(struct.unpack(f"<{n}h", raw))
        # Pad to integral milliseconds so the existing source clock agrees.
        samples.extend([0] * ((-len(samples)) % 16))
        cues.append((word, start // 16, len(samples) // 16))
        samples.extend([0] * GAP)
    lines = ["#pragma once", '#include "reading_timeline.h"', "#include <cstdint>",
             "inline constexpr uint32_t narration_rate = 16000;",
             "inline constexpr int16_t narration_pcm[] = {"]
    for i in range(0, len(samples), 16):
        lines.append("  " + ",".join(map(str, samples[i:i+16])) + ",")
    lines.extend(["};", "inline constexpr ReadingCue narration_cues[] = {"])
    lines.extend(f'  {{"{word}",{start},{end}}},' for word, start, end in cues)
    lines.extend(["};", f"inline constexpr uint32_t narration_duration = {len(samples)//16};"])
    return "\n".join(lines) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    try:
        args.output.write_text(build(args.source), encoding="utf-8")
        LOG.info("Created narration asset: %s", args.output)
        return 0
    except (OSError, ValueError, wave.Error) as exc:
        LOG.error("Narration asset rejected: %s", exc)
        return 1


if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO)
    raise SystemExit(main())
