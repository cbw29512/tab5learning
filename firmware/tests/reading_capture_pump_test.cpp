#include "reading_capture_pump.h"
#include <cassert>
struct Fake : ReadingCaptureBackend {
  bool busy=false, opened=false, allow=true, accept=true;
  size_t submitted=0;
  int16_t* destination=nullptr;
  bool begin() override { return opened=allow; }
  bool submit(int16_t* p,size_t n,uint32_t rate) override {
    assert(opened && !busy && p && n<=320 && rate==16000);
    if(!accept) return false;
    destination=p; submitted=n; busy=true; return true;
  }
  bool idle() override { return !busy; }
  void end() override { assert(!busy); opened=false; }
};
int main() {
  int16_t pcm[320]={};
  ReadingRecording r; Fake f; ReadingCapturePump p(r,f);
  assert(p.start(pcm,320,true,true,true));
  p.tick(0); assert(f.busy && r.count()==0);
  p.stop(true); p.tick(0); assert(p.active() && !p.discard());
  f.busy=false; p.tick(0);
  assert(!p.active() && !f.opened && r.count()==320);
  assert(r.state()==RecordingState::Incomplete);
  assert(p.discard());
  assert(p.start(pcm,320,true,true,true));
  p.tick(0); f.busy=false; p.tick(0);
  assert(r.state()==RecordingState::Review && !p.active());
  assert(p.discard());
  // A final short block must fit exactly, and no pending block is counted early.
  int16_t longer[325]={};
  assert(p.start(longer,325,true,true,true));
  p.tick(0); assert(f.submitted==320 && f.destination==longer);
  f.busy=false; p.tick(0);
  assert(r.count()==320 && f.submitted==5 && f.destination==longer+320);
  p.stop(); p.tick(0); assert(p.active() && r.count()==320);
  f.busy=false; p.tick(0);
  assert(!p.active() && r.count()==325 && r.state()==RecordingState::Review);
  assert(p.discard());
  // Submission failure must close capture without inventing audio evidence.
  f.accept=false;
  assert(p.start(pcm,320,true,true,true));
  p.tick(0); p.tick(0);
  assert(!p.active() && !f.opened && r.count()==0);
  assert(r.state()==RecordingState::Incomplete);
  assert(p.discard());
  f.accept=true;
  // Timeout is measured across timer wrap; a late release is not evidence.
  assert(p.start(pcm,320,true,true,true));
  p.tick(UINT32_MAX-499);
  p.tick(499);
  assert(!p.timed_out() && p.active());
  p.tick(500);
  assert(p.timed_out() && p.active() && !p.discard());
  assert(r.state()==RecordingState::Incomplete && r.count()==0);
  assert(!p.start(pcm,320,true,true,true));
  f.busy=false; p.tick(501);
  assert(!p.active() && r.count()==0 && p.discard());
  assert(p.start(pcm,320,true,true,true));
  assert(!p.timed_out());
  p.stop(); p.tick(0);
  assert(!p.active() && p.discard());
  f.allow=false;
  assert(!p.start(pcm,320,true,true,true));
  assert(r.state()==RecordingState::Incomplete && !p.active());
  assert(p.discard());
}
