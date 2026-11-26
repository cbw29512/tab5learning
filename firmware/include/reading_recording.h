#pragma once
#include <cstddef>
#include <cstdint>

// Portable capture state; no microphone is opened by construction.
// Caller owns the sample buffer, and supplies only confirmed captured samples.
enum class RecordingState { Ready, Recording, Review, Incomplete, Error };

class ReadingRecording {
 public:
  static constexpr uint32_t rate=16000;
  static constexpr uint32_t max_samples=rate*30;

  bool start(int16_t* buffer, size_t capacity, bool explicit_request,
             bool audio_idle, bool capture_authorized) {
    if(state_!=RecordingState::Ready || !explicit_request ||
       !audio_idle || !capture_authorized) return false;
    if(!buffer || !capacity || capacity>max_samples) {
      state_=RecordingState::Error; return false;
    }
    buffer_=buffer; capacity_=capacity; count_=0;
    state_=RecordingState::Recording;
    return true;
  }

  // The hardware adapter must not write beyond the supplied remaining space.
  int16_t* destination() const {
    return state_==RecordingState::Recording && count_<capacity_
      ? buffer_+count_ : nullptr;
  }
  size_t remaining() const {
    return state_==RecordingState::Recording ? capacity_-count_ : 0;
  }
  bool captured(size_t samples) {
    if(state_!=RecordingState::Recording) return false;
    if(samples>remaining()) { state_=RecordingState::Error; return false; }
    count_+=samples;
    if(count_==capacity_) state_=RecordingState::Review;
    return true;
  }
  bool finish() {
    if(state_!=RecordingState::Recording) return false;
    state_=count_ ? RecordingState::Review : RecordingState::Incomplete;
    return true;
  }
  void interrupt() {
    if(state_==RecordingState::Recording || state_==RecordingState::Review)
      state_=RecordingState::Incomplete;
  }
  // Call only after hardware capture has fully stopped. Wipe volatile evidence.
  bool discard(bool hardware_idle) {
    if(!hardware_idle || state_==RecordingState::Recording) return false;
    if(buffer_) {
      volatile int16_t* p=buffer_;
      for(size_t i=0;i<capacity_;++i) p[i]=0;
    }
    buffer_=nullptr; capacity_=0; count_=0; state_=RecordingState::Ready;
    return true;
  }
  RecordingState state() const { return state_; }
  size_t count() const { return count_; }
 private:
  RecordingState state_=RecordingState::Ready;
  int16_t* buffer_=nullptr;
  size_t capacity_=0, count_=0;
};
