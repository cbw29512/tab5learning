#include "reading_controller.h"
#include <cassert>

struct FakeAudio : ReadingAudio {
  uint32_t current=0, end=0;
  unsigned starts=0, stops=0;
  bool running=false, fail_start=false, fail_stop=false, fail_position=false;
  bool start(uint32_t from,uint32_t until) override {
    ++starts;
    if(fail_start || from>=until) return false;
    current=from; end=until; running=true; return true;
  }
  bool stop(uint32_t& confirmed) override {
    ++stops;
    if(fail_stop) return false;
    confirmed=current; running=false; return true;
  }
  bool position(uint32_t& source) override {
    if(fail_position || !running) return false;
    source=current; return true;
  }
};

int main() {
  static const ReadingCue cues[]={{"The",0,100},{"cat",150,300},{"ran",300,400}};
  FakeAudio audio;
  ReadingController c(audio);
  assert(!c.play());
  assert(c.initialize(cues,3,500));
  assert(c.play() && audio.running && audio.end==500);
  assert(c.play() && audio.starts==1);
  audio.current=125;
  assert(c.tick() && c.state().active==-1);
  audio.current=175;
  assert(c.pause() && !audio.running && c.state().position==175);
  assert(c.play() && audio.current==175);
  assert(!c.replay(99) && audio.running && audio.current==175);
  assert(c.replay(1) && audio.current==150 && audio.end==300);
  audio.current=250;
  assert(c.tick() && c.state().active==1);
  audio.current=300;
  assert(c.tick() && !audio.running && c.state().status==ReadingStatus::Paused);
  assert(c.play() && audio.current==300 && audio.end==500);
  audio.current=500;
  assert(c.tick() && !audio.running && c.state().status==ReadingStatus::Finished);
  assert(c.play() && audio.current==0);
  audio.fail_position=true;
  assert(!c.tick() && !audio.running && c.state().status==ReadingStatus::Error);
  audio.fail_position=false;
  assert(c.initialize(cues,3,500));
  audio.fail_start=true;
  assert(!c.play() && c.state().status==ReadingStatus::Error);
  audio.fail_start=false;
  assert(c.initialize(cues,3,500) && c.play());
  audio.fail_stop=true;
  assert(!c.pause() && c.state().status==ReadingStatus::Error);
  // A failed stop is explicitly not proof of hardware silence.
  assert(audio.running);
  return 0;
}
