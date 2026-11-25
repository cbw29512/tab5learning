"""Portable read-along playback state; native audio adapter is separate."""
from dataclasses import dataclass
import logging

from tools.reading_timing import NarrationTimeline

LOG = logging.getLogger(__name__)


@dataclass(frozen=True)
class PlaybackState:
    status: str
    position_ms: int
    token_id: str | None
    replay_end_ms: int | None = None


class ReadAlong:
    """Consume actual audio positions, never extrapolate from wall time."""

    def __init__(self, tokens, duration_ms):
        try:
            self._timeline = NarrationTimeline(tokens, duration_ms)
            self._duration = duration_ms
            self.state = PlaybackState("READY", 0, None)
        except (ValueError, TypeError):
            LOG.warning("Cannot initialize read-along")
            raise

    def _set(self, status, position, replay_end=None):
        try:
            cue = self._timeline.active_token(position)
            self.state = PlaybackState(status, position, cue[0] if cue else None,
                                       replay_end)
            return self.state
        except (ValueError, TypeError):
            LOG.warning("Read-along state update rejected")
            raise

    def play(self):
        """Return desired state; adapter must seek/start audio accordingly."""
        try:
            if self.state.status == "PLAYING":
                return self.state
            position = 0 if self.state.status == "FINISHED" else self.state.position_ms
            return self._set("PLAYING", position, self.state.replay_end_ms)
        except (ValueError, TypeError):
            LOG.warning("Cannot start narration")
            raise

    def pause(self, position_ms):
        """Call with the confirmed position after audio pauses."""
        try:
            if self.state.status != "PLAYING":
                raise ValueError("Narration is not playing")
            current = self.observe(position_ms)
            if current.status != "PLAYING":
                return current
            return self._set("PAUSED", current.position_ms, current.replay_end_ms)
        except (ValueError, TypeError):
            LOG.warning("Cannot pause narration")
            raise

    def replay_word(self, token_id):
        """Adapter seeks to returned position and stops at replay_end_ms."""
        try:
            start, end = self._timeline.replay_bounds(token_id)
            return self._set("PLAYING", start, end)
        except (ValueError, TypeError):
            LOG.warning("Cannot replay narration word")
            raise

    def observe(self, position_ms):
        """Report actual playback clock; reject stale backwards positions."""
        try:
            self._timeline.active_token(position_ms)
            if self.state.status != "PLAYING":
                return self.state
            if position_ms < self.state.position_ms:
                raise ValueError("Playback position moved backwards without seek")
            end = self.state.replay_end_ms
            if end is not None and position_ms >= end:
                # End of bounded replay: pause without highlighting next word.
                self.state = PlaybackState("PAUSED", end, None)
                return self.state
            if position_ms == self._duration:
                return self._set("FINISHED", position_ms)
            return self._set("PLAYING", position_ms, end)
        except (ValueError, TypeError):
            LOG.warning("Invalid audio position update")
            raise
