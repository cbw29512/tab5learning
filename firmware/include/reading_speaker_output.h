#pragma once
#include "reading_pcm_clock.h"
#include <cstddef>

// Backend reports queue occupancy, NOT acoustic sample position.
class ReadingSpeakerBackend {
 public:
  virtual ~ReadingSpeakerBackend() = default;
  virtual size_t queued() const = 0;
  virtual bool submit(const int16_t* samples, size_t count, uint32_t rate) = 0;
  virtual void cancel() = 0;
};

enum class SpeakerOutputState { Idle, Queued, QueueEmpty, Cancelling, Error };

// Caller owns immutable PCM storage for the entire backend lifetime.
// One dedicated channel, one producer. QueueEmpty is not proof of silence.
class ReadingSpeakerOutput {
 public:
  explicit ReadingSpeakerOutput(ReadingSpeakerBackend& backend) : backend_(backend) {}
  bool start(const int16_t* pcm, uint32_t count, uint32_t rate,
             uint32_t from, uint32_t until) {
    if (state_ == SpeakerOutputState::Queued ||
        state_ == SpeakerOutputState::Cancelling || backend_.queued()) return false;
    PcmSourceClock clock;
    PcmRange range;
    if (!pcm || !clock.initialize(rate, count) || !clock.range(from, until, range))
      return false;
    if (!backend_.submit(pcm + range.first, range.end - range.first, rate)) {
      state_ = SpeakerOutputState::Error;
      return false;
    }
    state_ = SpeakerOutputState::Queued;
    return true;
  }
  void poll() {
    if (!backend_.queued()) {
      if (state_ == SpeakerOutputState::Queued) state_ = SpeakerOutputState::QueueEmpty;
      if (state_ == SpeakerOutputState::Cancelling) state_ = SpeakerOutputState::Idle;
    }
  }
  void stop() {
    backend_.cancel();
    state_ = SpeakerOutputState::Cancelling;
  }
  SpeakerOutputState state() const { return state_; }
 private:
  ReadingSpeakerBackend& backend_;
  SpeakerOutputState state_ = SpeakerOutputState::Idle;
};
