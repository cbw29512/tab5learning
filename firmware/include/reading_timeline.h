#pragma once
#include <cstddef>
#include <cstdint>

// Portable source-audio timeline. No hardware calls, allocation or exceptions.
// The adapter owns actual playback and reports positions in source milliseconds.
struct ReadingCue {
  const char* text;
  uint32_t start_ms;
  uint32_t end_ms;
};
enum class ReadingStatus { Ready, Playing, Paused, Finished, Error };

class ReadingTimeline {
 public:
  bool initialize(const ReadingCue* cues, size_t count, uint32_t duration) {
    status = ReadingStatus::Error;
    cues_ = nullptr; count_ = 0; duration_ = 0;
    position = 0; active = -1; replay_end_ = 0;
    if (!cues || !count || count > 4096 || !duration) return false;
    uint32_t previous = 0;
    for (size_t i = 0; i < count; ++i) {
      const auto& cue = cues[i];
      if (!cue.text || !cue.text[0] || cue.start_ms < previous ||
          cue.start_ms >= cue.end_ms || cue.end_ms > duration) return false;
      previous = cue.end_ms;
    }
    cues_ = cues; count_ = count; duration_ = duration;
    status = ReadingStatus::Ready;
    return true;
  }

  bool play() {
    if (status == ReadingStatus::Error) return false;
    if (status == ReadingStatus::Finished) position = 0;
    status = ReadingStatus::Playing;
    active = lookup(position);
    return true;
  }

  bool observe(uint32_t ms) {
    if (status != ReadingStatus::Playing || ms < position || ms > duration_)
      return false;
    if (replay_end_ && ms >= replay_end_) {
      position = replay_end_; replay_end_ = 0; active = -1;
      status = ReadingStatus::Paused;
    } else {
      position = ms; active = lookup(ms);
      if (ms == duration_) status = ReadingStatus::Finished;
    }
    return true;
  }

  bool pause(uint32_t confirmed_ms) {
    if (!observe(confirmed_ms)) return false;
    if (status == ReadingStatus::Playing) status = ReadingStatus::Paused;
    return true;
  }

  bool replay(size_t index) {
    if (status == ReadingStatus::Error || index >= count_) return false;
    position = cues_[index].start_ms;
    replay_end_ = cues_[index].end_ms;
    active = static_cast<int>(index);
    status = ReadingStatus::Playing;
    return true;
  }

  uint32_t replay_end() const { return replay_end_; }
  ReadingStatus status = ReadingStatus::Error;
  uint32_t position = 0;
  int active = -1;

 private:
  // Borrowed immutable cue array must outlive the timeline.
  const ReadingCue* cues_ = nullptr;
  size_t count_ = 0;
  uint32_t duration_ = 0, replay_end_ = 0;

  int lookup(uint32_t ms) const {
    size_t lo = 0, hi = count_;
    while (lo < hi) {
      const size_t mid = lo + (hi - lo) / 2;
      if (cues_[mid].start_ms <= ms) lo = mid + 1;
      else hi = mid;
    }
    if (!lo || ms >= cues_[lo - 1].end_ms) return -1;
    return static_cast<int>(lo - 1);
  }
};
