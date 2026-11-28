#include "reading_preview_playback.h"
#include <cassert>
struct Backend : ReadingSpeakerBackend {
  size_t pending=0, length=0;
  const int16_t* data=nullptr;
  size_t queued() const override { return pending; }
  bool submit(const int16_t* p,size_t n,uint32_t) override {
    data=p; length=n; pending=1; return true;
  }
  void cancel() override { pending=0; }
};
int main() {
  static const int16_t pcm[16000]={};
  static const ReadingCue cues[]={{"A",0,100},{"fox",200,400}};
  Backend backend;
  ReadingSpeakerOutput output(backend);
  ReadingPreviewPlayback playback(output,pcm,16000,16000,cues,2);
  assert(!playback.replay(2));
  assert(playback.listen() && playback.busy());
  assert(backend.length==16000);
  assert(!playback.replay(1));
  playback.stop();
  assert(playback.busy());
  assert(playback.poll() && !playback.busy());
  assert(playback.replay(1));
  assert(backend.data==pcm+3200 && backend.length==3200);
  backend.pending=0;
  assert(playback.poll() && !playback.busy());
  // Pause must reject an invalid word and wait for cancellation to settle.
  assert(playback.listen());
  assert(!playback.pause(2));
  assert(playback.pause(1));
  assert(playback.paused() && playback.resume_offset()==200);
  assert(!playback.resume());
  assert(playback.poll());
  assert(playback.resume());
  assert(!playback.paused());
  assert(backend.data==pcm+3200 && backend.length==12800);
  assert(playback.pause(1));
  playback.stop();
  playback.poll();
  assert(!playback.paused() && !playback.resume());
  assert(playback.replay(0));
  assert(!playback.pause(0));
  playback.stop();
  playback.poll();
  return 0;
}
