"""Pure decimal grade projection; inputs must be authorized effective records."""
import logging
from decimal import Decimal, InvalidOperation, ROUND_HALF_UP, localcontext

LOG = logging.getLogger(__name__)


def number(value):
    # Decimal strings avoid binary floating-point grading discrepancies.
    if type(value) not in (str, int, Decimal):
        raise ValueError("Expected decimal number")
    if len(str(value)) > 64:
        raise ValueError("Numeric input too long")
    result = Decimal(value)
    if not result.is_finite() or not 0 <= result <= 1000000:
        raise ValueError("Invalid nonnegative grade value")
    if result.as_tuple().exponent < -6:
        raise ValueError("Excess numeric precision")
    return result


def project_grade(core, rows, *, policy_version, aggregate_cap=None, cap_at_100=True):
    """Calculate one student's one subject/period from reviewed effective rows.

    Assignment IDs are unique across versions. Missing or uncertain work adds
    nothing. This projection does not modify the core score or mastery state.
    """
    try:
        if not isinstance(policy_version, str) or not policy_version.strip():
            raise ValueError("Policy version required")
        if type(cap_at_100) is not bool:
            raise ValueError("Invalid grade cap")
        core = number(core)
        if core > 100:
            raise ValueError("Core score exceeds 100")
        cap = None if aggregate_cap is None else number(aggregate_cap)
        seen, details = set(), []
        with localcontext() as ctx:
            ctx.prec = 40
            bonus = Decimal(0)
            for row in rows:
                if set(row) != {"assignment_id", "maximum_bonus", "earned", "possible", "status"}:
                    raise ValueError("Invalid credit record")
                key = row["assignment_id"]
                if not isinstance(key, str) or not key.strip() or key in seen:
                    raise ValueError("Duplicate or invalid assignment")
                seen.add(key)
                weight = number(row["maximum_bonus"])
                status = row["status"]
                if status not in ("reviewed", "missing", "uncertain", "self_rated"):
                    raise ValueError("Invalid assessment status")
                credit = Decimal(0)
                if status == "reviewed":
                    earned, possible = number(row["earned"]), number(row["possible"])
                    if possible == 0 or earned > possible:
                        raise ValueError("Invalid reviewed score")
                    credit = weight * earned / possible
                elif row["earned"] is not None or row["possible"] is not None:
                    raise ValueError("Unreviewed score must be absent")
                bonus += credit
                details.append({"assignment_id": key, "bonus": str(credit.quantize(Decimal("0.01"), rounding=ROUND_HALF_UP))})
            awarded = bonus if cap is None else min(bonus, cap)
            combined = core + awarded
            if cap_at_100:
                combined = min(combined, Decimal(100))
            def display(value):
                return str(value.quantize(Decimal("0.01"), rounding=ROUND_HALF_UP))
            return {"policy_version": policy_version, "core": display(core),
                    "bonus_before_caps": display(bonus), "bonus_awarded": display(combined-core),
                    "combined": display(combined), "assignments": details}
    except (InvalidOperation, ValueError, TypeError, KeyError):
        LOG.warning("Extra credit projection rejected")
        raise
