#pragma once
#include <M5Unified.h>
#include <atomic>
#include "reading_capture_pump.h"

// Isolated probe only: requires M5Unified with setBufferReleaseCallback.
// One producer; this object and destination buffers must outlive the mic task.
// Caller must disable narration actions throughout capture and shutdown.
// https://docs.m5stack.com/en/arduino/m5tab5/mic
// https://github.com/m5stack/M5Unified/blob/master/src/utility/Mic_Class.hpp
class M5ReadingCapture final : public ReadingCaptureBackend {
 public:
  M5ReadingCapture()=default;
  M5ReadingCapture(const M5ReadingCapture&)=delete;
  M5ReadingCapture& operator=(const M5ReadingCapture&)=delete;

  bool begin() override {
    if(open_ || M5.Speaker.isPlaying() || M5.Mic.isRecording()) return false;
    M5.Speaker.end();
    M5.Mic.end();
    M5.Mic.setSampleRate(ReadingRecording::rate);
    if(!M5.Mic.begin()) {
      M5.Mic.end();
      speaker_ready_=M5.Speaker.begin();
      return false;
    }
    // No request exists yet; callback registration is safe at this point.
    M5.Mic.setBufferReleaseCallback(this,&released);
    open_=true; speaker_ready_=false;
    return true;
  }

  bool submit(int16_t* samples,size_t count,uint32_t rate) override {
    if(!open_ || !idle() || !samples || !count ||
       count>ReadingCapturePump::block_samples || rate!=ReadingRecording::rate)
      return false;
    expected_count_.store(count,std::memory_order_relaxed);
    pending_.store(samples,std::memory_order_release);
    if(!M5.Mic.record(samples,count,rate,false)) {
      pending_.store(nullptr,std::memory_order_release);
      return false;
    }
    return true;
  }

  bool idle() override {
    // The callback, not queue occupancy alone, releases the borrowed buffer.
    return pending_.load(std::memory_order_acquire)==nullptr &&
           M5.Mic.isRecording()==0;
  }

  void end() override {
    if(!open_ || !idle()) return;
    M5.Mic.end();
    M5.Mic.setBufferReleaseCallback(nullptr,nullptr);
    open_=false;
    speaker_ready_=M5.Speaker.begin();
  }

  bool speaker_ready() const { return speaker_ready_; }

 private:
  static void released(void* context,void* data,size_t length) {
    auto* self=static_cast<M5ReadingCapture*>(context);
    // Never block or call port lifecycle methods from the capture task.
    auto* expected=self->pending_.load(std::memory_order_acquire);
    if(expected && expected==data &&
       length==self->expected_count_.load(std::memory_order_relaxed))
      self->pending_.store(nullptr,std::memory_order_release);
  }
  std::atomic<int16_t*> pending_{nullptr};
  std::atomic<size_t> expected_count_{0};
  bool open_=false, speaker_ready_=false;
};
