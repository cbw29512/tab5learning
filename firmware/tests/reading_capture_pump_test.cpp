#include "reading_capture_pump.h"
#include <cassert>
struct Fake : ReadingCaptureBackend {
  bool busy=false, opened=false, allow=true;
  bool begin() override { return opened=allow; }
  bool submit(int16_t* p,size_t n,uint32_t rate) override {
    assert(opened && !busy && p && n<=320 && rate==16000);
    busy=true; return true;
  }
  bool idle() override { return !busy; }
  void end() override { assert(!busy); opened=false; }
};
int main() {
  int16_t pcm[320]={};
  ReadingRecording r; Fake f; ReadingCapturePump p(r,f);
  assert(p.start(pcm,320,true,true,true));
  p.tick(); assert(f.busy && r.count()==0);
  p.stop(true); p.tick(); assert(p.active() && !p.discard());
  f.busy=false; p.tick();
  assert(!p.active() && !f.opened && r.count()==320);
  assert(r.state()==RecordingState::Incomplete);
  assert(p.discard());
  assert(p.start(pcm,320,true,true,true));
  p.tick(); f.busy=false; p.tick();
  assert(r.state()==RecordingState::Review && !p.active());
  assert(p.discard());
  f.allow=false;
  assert(!p.start(pcm,320,true,true,true));
  assert(r.state()==RecordingState::Incomplete && !p.active());
  assert(p.discard());
}
