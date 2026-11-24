"""Offline printable HTML certificate preview; authorization belongs to caller."""
import logging
from datetime import date
from html import escape
from tools.competency_graph import valid_id

LOG = logging.getLogger(__name__)


def certificate_html(ledger, award_sequence, metadata):
    """Render only a committed certificate, never a badge or unearned candidate.

    Metadata must be from an authorized, version-matched local catalog and
    learner mapping. This preview does not sign or establish accreditation.
    """
    try:
        if type(award_sequence) is not int or award_sequence < 1:
            raise ValueError("Invalid award sequence")
        if set(metadata) != {"learner_id", "name", "title", "award_id", "version",
                            "completion_kind", "completion_date", "status"}:
            raise ValueError("Invalid certificate metadata")
        if metadata["learner_id"] != ledger.learner_id:
            raise ValueError("Wrong learner mapping")
        record = next((r for r in ledger.portfolio()
                       if r["sequence"] == award_sequence), None)
        if not record or record["kind"] != "certificate":
            raise ValueError("Committed major certificate required")
        if (metadata["award_id"], metadata["version"]) != (record["award_id"], record["version"]):
            raise ValueError("Wrong milestone version")
        if type(metadata["version"]) is not int or not valid_id(metadata["award_id"]):
            raise ValueError("Invalid milestone identity")
        for field in ("name", "title"):
            value = metadata[field]
            if not isinstance(value, str) or not value.strip() or len(value)>160 or any(ord(c)<32 for c in value):
                raise ValueError("Invalid certificate label")
        kind = metadata["completion_kind"]
        if kind not in ("activity_completion", "verified_mastery"):
            raise ValueError("Invalid completion kind")
        if metadata["status"] not in ("issued", "under_review", "superseded"):
            raise ValueError("Invalid certificate status")
        day = metadata["completion_date"]
        if day is not None:
            if not isinstance(day,str) or date.fromisoformat(day).isoformat()!=day:
                raise ValueError("Invalid completion date")
        # Escape all catalog/student text; no scripts, network assets or tracking.
        name, title = escape(metadata["name"]), escape(metadata["title"])
        label = "Verified mastery" if kind == "verified_mastery" else "Major curriculum completion"
        status = escape(metadata["status"].replace("_"," ").title())
        identifier = escape(f'{ledger.learner_id}/{record["award_id"]}/{record["version"]}')
        return f"""<!doctype html>
<html lang="en"><head><meta charset="utf-8"><title>Pocket Learning certificate</title>
<style>
@page {{size: landscape; margin: 12mm}}
* {{box-sizing:border-box}}
body {{font-family:Georgia,serif;color:#172f49;background:white;margin:0}}
main {{border:6px double #99742c;padding:8mm;text-align:center;overflow-wrap:anywhere}}
h1 {{font-size:30pt;margin:5mm 0}} h2 {{font-size:24pt;margin:4mm 0}}
p {{font-size:14pt;margin:3mm 0}} small {{display:block;margin:3mm 0}}
@media screen {{main {{max-width:1000px;margin:20px auto}}}}
</style></head><body><main>
<p>POCKET LEARNING</p><h1>Certificate of Completion</h1>
<p>Presented to</p><h2>{name}</h2><p>For completing</p><h2>{title}</h2>
<p>{label}</p><p>Completion date: {escape(day or "Not recorded")}</p>
<p>Status: {status}</p><small>Certificate reference: {identifier}</small>
<small>Product-issued learning recognition; not an accredited qualification.</small>
<small>Development preview — verification and authorized export integration pending.</small>
</main></body></html>"""
    except (ValueError, TypeError, KeyError):
        LOG.warning("Certificate preview rejected")
        raise
