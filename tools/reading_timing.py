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


class NarrationTimeline:
    """Immutable cues; audio position is the only playback clock."""

    def __init__(self, tokens, duration_ms):
        try:
            self._cues = validate_timing(tokens, duration_ms)
            self._duration = duration_ms
            self._starts = tuple(cue[2] for cue in self._cues)
        except (ValueError, TypeError):
            LOG.warning("Cannot initialize narration timeline")
            raise

    def active_token(self, position_ms):
        """Return the active cue or None in silence and at end of audio."""
        from bisect import bisect_right
        try:
            if type(position_ms) is not int or not 0 <= position_ms <= self._duration:
                raise ValueError("Playback position outside narration")
            # Binary search finds the most recent cue; its end is exclusive.
            index = bisect_right(self._starts, position_ms) - 1
            if index >= 0 and position_ms < self._cues[index][3]:
                return self._cues[index]
            return None
        except (ValueError, TypeError):
            LOG.warning("Invalid narration playback position")
            raise

    def replay_bounds(self, token_id):
        """Return source-audio bounds for a word; player handles seeking."""
        try:
            if not isinstance(token_id, str):
                raise ValueError("Invalid token ID")
            for cue in self._cues:
                if cue[0] == token_id:
                    return cue[2], cue[3]
            raise ValueError("Unknown narration token")
        except ValueError:
            LOG.warning("Cannot resolve narration replay target")
            raise
