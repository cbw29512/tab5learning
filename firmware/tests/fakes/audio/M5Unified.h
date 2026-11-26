#pragma once
#include <cstddef>
#include <cstdint>

struct FakeSpeaker {
  bool playing=false, running=true, allow=true;
  bool isPlaying() const { return playing; }
  void end() { running=false; }
  bool begin() { return running=allow; }
};
struct FakeMic {
  using Callback=void(*)(void*,void*,size_t);
  bool running=false, allow=true, accept=true;
  size_t queued=0;
  uint32_t rate=0;
  void* context=nullptr;
  Callback callback=nullptr;
  bool begin() { return running=allow; }
  void end() { running=false; queued=0; }
  void setSampleRate(uint32_t r) { rate=r; }
  size_t isRecording() const { return queued; }
  void setBufferReleaseCallback(void* c,Callback f) { context=c; callback=f; }
  bool record(int16_t* p,size_t n,uint32_t r,bool stereo) {
    if(!running || !accept || !p || !n || r!=rate || stereo) return false;
    queued=1; return true;
  }
  void release(void* p,size_t n) { if(callback) callback(context,p,n); }
};
struct FakeM5 { FakeSpeaker Speaker; FakeMic Mic; };
inline FakeM5 M5;
