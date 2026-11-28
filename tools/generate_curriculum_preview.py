"""Generate checked-in, bounded C++ preview data from the validated draft catalog."""
import json
import logging
from pathlib import Path
import textwrap
from tools.curriculum_draft import decode_draft, MAX_BYTES

LOG = logging.getLogger(__name__)
ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "curriculum/grade-1/starter.v16.json"
TARGET = ROOT / "firmware/include/curriculum_preview.generated.h"
SUBJECTS = ("mathematics","english_language_arts","science","social_studies")
KINDS = {"choice":"Choice","instruction":"Instruction","teacher_review":"AdultReview"}
# Presentation-only categories; older broad catalog units retain their IDs.
UNITS = (
    (0, "counting", "Counting"),
    (0, "number-and-operations", "Addition and subtraction"),
    (0, "place-value", "Tens and ones"),
    (0, "geometry", "Shapes"),
    (0, "measurement", "Measurement"),
    (0, "time", "Time"),
    (1, "phonological-awareness", "Sounds and rhymes"),
    (1, "phonics", "Word sounds and spelling"),
    (1, "comprehension", "Reading stories"),
    (1, "writing", "Writing"),
    (2, "light-and-sound", "Light and sound"),
    (2, "earth-and-sky", "Earth and sky"),
    (2, "life-science", "Plants and animals"),
    (3, "community", "Community and citizenship"),
    (3, "geography", "Maps and places"),
    (3, "economics", "Needs and choices"),
    (3, "history", "Past and present"),
    (0, "data", "Charts and data"),
    (0, "money", "Money"),
    (0, "patterns", "Patterns"),
    (1, "fluency", "Reading smoothly"),
    (1, "book-reference", "Books and reference skills"),
    (1, "vocabulary", "Words and meanings"),
    (1, "grammar", "Grammar and punctuation"),
    (1, "handwriting", "Handwriting"),
    (1, "listening", "Listening and speaking"),
    (1, "visual-reading", "Pictures and words"),
)
UNIT_OVERRIDES = {
    "g1-count-across-tens": "counting",
    "g1-tens-and-ones": "place-value",
    "g1-compare-two-digit": "place-value",
    "g1-clock-hours": "time",
    "g1-clock-half-hours": "time",
}
UNIT_ALIASES = {"waves": "light-and-sound", "civics": "community"}


def unit_index(lesson):
    try:
        key = UNIT_OVERRIDES.get(lesson["lesson_id"], lesson["unit"])
        key = UNIT_ALIASES.get(key, key)
        subject = SUBJECTS.index(lesson["subject"])
        return next(i for i, (s, k, _) in enumerate(UNITS) if (s, k) == (subject, key))
    except (ValueError, StopIteration):
        LOG.error("Unmapped preview unit: %s", lesson["lesson_id"])
        raise ValueError("Unmapped preview unit") from None


def generate(raw):
    try:
        catalog, _ = decode_draft(raw)
        # The native lesson browser now paginates; retain catalog bounds
        # and per-activity display constraints instead of a four-lesson cap.
        lines = ['#pragma once','// Generated from starter.v16.json. Do not edit by hand.',
                 '#include "curriculum_preview_types.h"','namespace curriculum_preview {']
        def literal(value, width=60, rows=1):
            if not value.isascii() or len(textwrap.wrap(value,width=width)) > rows:
                raise ValueError("Text does not fit the preview display")
            return json.dumps(value)
        for i,l in enumerate(catalog["lessons"]):
            lines.append(f"inline constexpr Activity activities_{i}[]={{")
            for a in l["activities"]:
                options = a["choices"]
                if len(options)>3:
                    raise ValueError("Preview supports at most three choices")
                choices = ",".join(literal(x) for x in options+[""]*(3-len(options)))
                answer = a["answer_index"] if a["answer_index"] is not None else -1
                lines.append("  {Kind::"+KINDS[a["kind"]]+","+literal(a["phase"])+","
                             +literal(a["prompt"],rows=5)+","+literal(a["feedback"],rows=5)
                             +",{"+choices+"},"+str(len(options))+","+str(answer)+"},")
            lines.append("};")
        lines.append("inline constexpr Lesson lessons[]={")
        for i,l in enumerate(catalog["lessons"]):
            lines.append(f'  {{{SUBJECTS.index(l["subject"])},{literal(l["lesson_id"],width=80)},'
                         f'{l["revision"]},{literal(l["title"])},activities_{i},{len(l["activities"])} }},'.replace(" },","},"))
        lines.append("}")
        # Match the exact C++ array terminator, then namespace terminator.
        lines[-1]="};"
        lines.append("inline constexpr Unit units[]={")
        for subject, key, label in UNITS:
            lines.append(f'  {{{subject},{literal(key)},{literal(label)}}},')
        lines.append("};")
        lines.append("inline constexpr unsigned lesson_units[]={")
        for lesson in catalog["lessons"]:
            lines.append(f"  {unit_index(lesson)},")
        lines.append("};")
        # Compact adjacency table: offsets delimit each lesson's direct prerequisites.
        indices = {lesson["lesson_id"]: i for i, lesson in enumerate(catalog["lessons"])}
        offsets, prerequisites = [0], []
        for lesson in catalog["lessons"]:
            prerequisites.extend(indices[key] for key in lesson["prerequisites"])
            offsets.append(len(prerequisites))
        lines.append("inline constexpr unsigned lesson_prerequisite_offsets[]={")
        lines.extend(f"  {value}," for value in offsets)
        lines.append("};")
        lines.append("inline constexpr unsigned lesson_prerequisites[]={")
        # Sentinel keeps the array nonempty even for a catalog without prerequisites.
        lines.extend(f"  {value}," for value in prerequisites + [len(indices)])
        lines.append("};")
        lines.append("}")
        return "\n".join(lines)+"\n"
    except Exception:
        LOG.warning("Curriculum preview generation failed")
        raise


def main():
    try:
        with SOURCE.open("rb") as stream:
            data = generate(stream.read(MAX_BYTES+1))
        TARGET.write_text(data, encoding="utf-8", newline="\n")
    except Exception:
        LOG.exception("Cannot generate curriculum preview")
        raise


if __name__ == "__main__":
    main()
