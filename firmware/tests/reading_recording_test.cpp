#include "reading_recording.h"
#include <cassert>
int main() {
  int16_t pcm[4]={1,2,3,4};
  ReadingRecording recording;
  assert(!recording.start(pcm,4,false,true,true));
  assert(!recording.start(pcm,4,true,false,true));
  assert(!recording.start(pcm,4,true,true,false));
  assert(recording.state()==RecordingState::Ready);
  assert(recording.start(pcm,4,true,true,true));
  assert(!recording.start(pcm,4,true,true,true));
  assert(recording.destination()==pcm && recording.remaining()==4);
  assert(recording.captured(2));
  assert(recording.destination()==pcm+2 && recording.remaining()==2);
  assert(!recording.discard(true));
  assert(recording.captured(2));
  assert(recording.state()==RecordingState::Review);
  assert(!recording.destination() && !recording.remaining());
  assert(!recording.discard(false));
  assert(recording.discard(true));
  for(auto value:pcm) assert(value==0);
  assert(recording.start(pcm,4,true,true,true));
  assert(recording.finish());
  assert(recording.state()==RecordingState::Incomplete);
  assert(recording.discard(true));
  assert(recording.start(pcm,4,true,true,true));
  assert(recording.captured(1));
  recording.interrupt();
  assert(recording.state()==RecordingState::Incomplete && recording.count()==1);
  assert(recording.discard(true));
  assert(recording.start(pcm,4,true,true,true));
  assert(!recording.captured(5));
  assert(recording.state()==RecordingState::Error);
  assert(recording.discard(true));
  assert(recording.start(pcm,4,true,true,true));
  assert(!recording.captured(0));
  assert(recording.state()==RecordingState::Error && recording.count()==0);
  assert(recording.discard(true));
  assert(!recording.start(nullptr,4,true,true,true));
  assert(recording.discard(true));
  assert(!recording.start(pcm,ReadingRecording::max_samples+1,true,true,true));
  return 0;
}
