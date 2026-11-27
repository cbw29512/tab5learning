"""Generate checked-in, bounded C++ preview data from the validated draft catalog."""
import json
import logging
from pathlib import Path
import textwrap
from tools.curriculum_draft import decode_draft, MAX_BYTES

LOG = logging.getLogger(__name__)
ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "curriculum/grade-1/starter.v2.json"
TARGET = ROOT / "firmware/include/curriculum_preview.generated.h"
SUBJECTS = ("mathematics","english_language_arts","science","social_studies")
KINDS = {"choice":"Choice","instruction":"Instruction","teacher_review":"AdultReview"}


def generate(raw):
    try:
        catalog, _ = decode_draft(raw)
        # The native lesson browser now paginates; retain catalog bounds
        # and per-activity display constraints instead of a four-lesson cap.
        lines = ['#pragma once','// Generated from starter.v2.json. Do not edit by hand.',
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
