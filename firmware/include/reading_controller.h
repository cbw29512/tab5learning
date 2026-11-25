#pragma once
#include "reading_timeline.h"

// Driver contract: source milliseconds; stop must cancel queued audio.
// Concrete Tab5 driver must confirm positions rather than estimate wall time.
class ReadingAudio {
 public:
  virtual ~ReadingAudio() = default;
  virtual bool start(uint32_t from, uint32_t until) = 0;
  virtual bool stop(uint32_t& confirmed_position) = 0;
  virtual bool position(uint32_t& source_position) = 0;
};

class ReadingController {
 public:
  explicit ReadingController(ReadingAudio& audio) : audio_(audio) {}

  bool initialize(const ReadingCue* cues, size_t count, uint32_t duration) {
    if (timeline_.status == ReadingStatus::Playing && !stop_driver()) return false;
    duration_ = duration;
    return timeline_.initialize(cues, count, duration);
  }
  const ReadingTimeline& state() const { return timeline_; }

  bool play() {
    if (timeline_.status == ReadingStatus::Playing) return true;
    if (!timeline_.play()) return false;
    return start_driver();
  }

  bool pause() {
    if (timeline_.status != ReadingStatus::Playing) return false;
    uint32_t position = 0;
    if (!audio_.stop(position) || !timeline_.pause(position)) return fail();
    return true;
  }

  bool replay(size_t index) {
    // Validate target against a copy before interrupting existing playback.
    auto next = timeline_;
    if (!next.replay(index)) return false;
    if (timeline_.status == ReadingStatus::Playing && !stop_driver()) return false;
    timeline_ = next;
    return start_driver();
  }

  bool tick() {
    if (timeline_.status != ReadingStatus::Playing) return true;
    uint32_t position = 0;
    if (!audio_.position(position) || !timeline_.observe(position)) return fail();
    if (timeline_.status != ReadingStatus::Playing) return stop_driver();
    return true;
  }

 private:
  ReadingAudio& audio_;
  ReadingTimeline timeline_;
  uint32_t duration_ = 0;

  bool start_driver() {
    const auto end = timeline_.replay_end() ? timeline_.replay_end() : duration_;
    if (timeline_.position == end) {
      return timeline_.observe(end);
    }
    if (!audio_.start(timeline_.position, end)) return fail();
    return true;
  }
  bool stop_driver() {
    uint32_t ignored = 0;
    if (!audio_.stop(ignored)) return fail();
    return true;
  }
  bool fail() {
    uint32_t ignored = 0;
    audio_.stop(ignored);  // Best effort; caller must surface hardware failure.
    timeline_.status = ReadingStatus::Error;
    timeline_.active = -1;
    return false;
  }
};
