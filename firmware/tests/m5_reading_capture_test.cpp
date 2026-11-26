#include "m5_reading_capture.h"
#include <cassert>

int main() {
  M5ReadingCapture mic;
  ReadingRecording recording;
  ReadingCapturePump pump(recording,mic);
  int16_t pcm[320]={}, other[320]={};
  M5.Speaker.playing=true;
  assert(!pump.start(pcm,320,true,true,true));
  assert(M5.Speaker.running && !M5.Mic.running && pump.discard());
  M5.Speaker.playing=false;
  assert(pump.start(pcm,320,true,true,true));
  assert(!M5.Speaker.running && M5.Mic.running);
  pump.tick(0);
  M5.Mic.queued=0; // Queue empties before callback.
  pump.stop();
  pump.tick(0);
  assert(pump.active() && recording.count()==0 && !pump.discard());
  M5.Mic.release(other,320);
  M5.Mic.release(pcm,319);
  pump.tick(0);
  assert(pump.active() && recording.count()==0);
  M5.Mic.release(pcm,320);
  pump.tick(0);
  assert(!pump.active() && recording.count()==320);
  assert(recording.state()==RecordingState::Review);
  assert(!M5.Mic.running && !M5.Mic.callback && mic.speaker_ready());
  assert(pump.discard());
  // Empty queue without callback must remain quarantined after timeout.
  assert(pump.start(pcm,320,true,true,true));
  pump.tick(10);
  M5.Mic.queued=0;
  pump.tick(1010);
  assert(pump.timed_out() && pump.active() && !pump.discard());
  assert(recording.state()==RecordingState::Incomplete && recording.count()==0);
  assert(!M5.Speaker.running);
  M5.Mic.release(pcm,320);
  pump.tick(1011);
  assert(!pump.active() && recording.count()==0 && mic.speaker_ready());
  assert(pump.discard());
  M5.Mic.allow=false;
  assert(!pump.start(pcm,320,true,true,true));
  assert(M5.Speaker.running && !M5.Mic.running && pump.discard());
  M5.Mic.allow=true;
  M5.Mic.accept=false;
  assert(pump.start(pcm,320,true,true,true));
  pump.tick(0); pump.tick(0);
  assert(!pump.active() && recording.state()==RecordingState::Incomplete);
  assert(recording.count()==0 && mic.speaker_ready() && pump.discard());
}
