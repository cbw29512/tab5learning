"""Portable validation of narration timing; no device or network dependencies."""
import logging

LOG = logging.getLogger(__name__)


def validate_timing(tokens, duration_ms):
    """Return immutable validated timing records or raise ValueError."""
    try:
        if type(duration_ms) is not int or duration_ms <= 0:
            raise ValueError("Invalid audio duration")
        if not isinstance(tokens, list) or not tokens:
            raise ValueError("Missing token timing")
        seen = set()
        previous_end = 0
        validated = []
        for token in tokens:
            if not isinstance(token, dict):
                raise ValueError("Token must be an object")
            token_id = token.get("token_id")
            word = token.get("text")
            start = token.get("start_ms")
            end = token.get("end_ms")
            if not isinstance(token_id, str) or not token_id.strip() or token_id in seen:
                raise ValueError("Missing or duplicate token ID")
            if not isinstance(word, str) or not word.strip():
                raise ValueError("Missing token text")
            # Reject booleans, overlaps and times outside the audio.
            if type(start) is not int or type(end) is not int:
                raise ValueError("Timing must use integer milliseconds")
            if not previous_end <= start < end <= duration_ms:
                raise ValueError("Invalid token timing interval")
            validated.append((token_id, word, start, end))
            seen.add(token_id)
            previous_end = end
        return tuple(validated)
    except (ValueError, TypeError):
        # Do not include potentially personal content in logs.
        LOG.warning("Narration timing validation rejected input")
        raise
