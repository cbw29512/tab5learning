#pragma once
#include "reading_speaker_output.h"
#include "reading_timeline.h"

// Whole passage and single word playback; deliberately no acoustic-position claim.
// All asset storage is borrowed, immutable and must outlive this controller.
class ReadingPreviewPlayback {
 public:
  ReadingPreviewPlayback(ReadingSpeakerOutput& output, const int16_t* pcm,
      uint32_t count, uint32_t rate, const ReadingCue* cues, size_t words)
      : output_(output), pcm_(pcm), count_(count), rate_(rate), cues_(cues), words_(words) {}
  bool listen() {
    PcmSourceClock clock;
    if (!clock.initialize(rate_, count_)) return false;
    if (!start(0, clock.duration_ms())) return false;
    passage_=true; paused_=false; resume_ms_=0;
    return true;
  }
  bool replay(size_t word) {
    if (!cues_ || word >= words_) return false;
    if (!start(cues_[word].start_ms, cues_[word].end_ms)) return false;
    passage_=false; paused_=false; resume_ms_=0;
    return true;
  }
  // Pause is a cancellation followed by a conservative estimated word restart.
  // The chosen boundary is not a measured hardware playback position.
  bool pause(size_t word) {
    if (!passage_ || paused_ || output_.state()!=SpeakerOutputState::Queued ||
        !cues_ || word>=words_) return false;
    resume_ms_=cues_[word].start_ms;
    paused_=true;
    output_.stop();
    return true;
  }
  bool resume() {
    if (!paused_ || busy()) return false;
    PcmSourceClock clock;
    if (!clock.initialize(rate_,count_) || !start(resume_ms_,clock.duration_ms())) return false;
    paused_=false; passage_=true;
    return true;
  }
  bool paused() const { return paused_; }
  uint32_t resume_offset() const { return resume_ms_; }
  void stop() {
    if (busy()) output_.stop();
    paused_=false; passage_=false; resume_ms_=0;
  }
  bool poll() {
    const auto previous = output_.state();
    output_.poll();
    return previous != output_.state();
  }
  bool busy() const {
    return output_.state() == SpeakerOutputState::Queued ||
           output_.state() == SpeakerOutputState::Cancelling;
  }
  const char* status() const {
    if (paused_ && !busy()) return "Paused";
    switch (output_.state()) {
      case SpeakerOutputState::Queued: return "Audio queued";
      case SpeakerOutputState::Cancelling: return "Stopping audio";
      case SpeakerOutputState::QueueEmpty: return "Audio queue empty";
      case SpeakerOutputState::Error: return "Audio unavailable";
      default: return "Ready";
    }
  }
 private:
  bool start(uint32_t from, uint32_t until) {
    return output_.start(pcm_, count_, rate_, from, until);
  }
  ReadingSpeakerOutput& output_;
  const int16_t* pcm_;
  uint32_t count_, rate_;
  const ReadingCue* cues_;
  size_t words_;
  bool passage_=false, paused_=false;
  uint32_t resume_ms_=0;
};
