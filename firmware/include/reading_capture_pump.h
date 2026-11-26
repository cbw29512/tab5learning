#pragma once
#include "reading_recording.h"

// A successful submission borrows the destination until idle() is true.
// Implementations must confirm completion, not merely queue acceptance.
class ReadingCaptureBackend {
 public:
  virtual ~ReadingCaptureBackend()=default;
  virtual bool begin()=0;
  virtual bool submit(int16_t* samples,size_t count,uint32_t rate)=0;
  virtual bool idle()=0;
  virtual void end()=0;
};

class ReadingCapturePump {
 public:
  static constexpr size_t block_samples=320; // 20 ms at 16 kHz.
  ReadingCapturePump(ReadingRecording& recording,ReadingCaptureBackend& backend)
    : recording_(recording),backend_(backend) {}
  ReadingCapturePump(const ReadingCapturePump&)=delete;
  ReadingCapturePump& operator=(const ReadingCapturePump&)=delete;

  bool start(int16_t* buffer,size_t capacity,bool request,bool audio_idle,bool authorized) {
    if(open_ || !recording_.start(buffer,capacity,request,audio_idle,authorized)) return false;
    if(!backend_.begin()) { recording_.interrupt(); return false; }
    open_=true; stop_=false; interrupted_=false; pending_=0;
    return true;
  }
  // Stop submitting immediately; drain a borrowed block before releasing it.
  void stop(bool interrupted=false) {
    if(open_) { stop_=true; interrupted_=interrupted_ || interrupted; }
  }
  void tick() {
    if(!open_) return;
    if(pending_) {
      if(!backend_.idle()) return;
      const size_t complete=pending_; pending_=0;
      if(!recording_.captured(complete)) { interrupted_=true; stop_=true; }
    }
    if(stop_ || recording_.state()!=RecordingState::Recording) {
      if(!backend_.idle()) return;
      backend_.end(); open_=false;
      if(interrupted_) recording_.interrupt();
      else recording_.finish();
      return;
    }
    if(!backend_.idle()) return;
    const size_t remaining=recording_.remaining();
    const size_t count=remaining<block_samples?remaining:block_samples;
    if(!count || !backend_.submit(recording_.destination(),count,ReadingRecording::rate)) {
      stop_=true; interrupted_=true; return;
    }
    pending_=count;
  }
  bool active() const { return open_; }
  bool discard() { return !open_ && recording_.discard(backend_.idle()); }
 private:
  ReadingRecording& recording_;
  ReadingCaptureBackend& backend_;
  size_t pending_=0;
  bool open_=false,stop_=false,interrupted_=false;
};
