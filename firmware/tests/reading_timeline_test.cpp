#include "reading_timeline.h"
#include <cassert>

int main() {
  static const ReadingCue cues[] = {
    {"The", 50, 200}, {"cat", 250, 500}, {"ran", 500, 600}
  };
  ReadingTimeline t;
  assert(!t.play());
  assert(t.initialize(cues, 3, 700));
  assert(t.status == ReadingStatus::Ready);
  assert(t.play() && t.active == -1);
  assert(t.observe(50) && t.active == 0);
  assert(t.observe(200) && t.active == -1);
  assert(t.observe(250) && t.active == 1);
  assert(t.pause(300) && t.status == ReadingStatus::Paused);
  assert(!t.observe(400) && t.position == 300);
  assert(t.play() && t.position == 300);
  assert(!t.observe(299) && t.position == 300);
  assert(t.observe(700) && t.status == ReadingStatus::Finished);
  assert(t.play() && t.position == 0);
  assert(t.replay(1) && t.position == 250 && t.replay_end() == 500);
  assert(t.pause(300) && t.replay_end() == 500);
  assert(t.play());
  assert(t.observe(520) && t.status == ReadingStatus::Paused);
  assert(t.position == 500 && t.active == -1 && t.replay_end() == 0);
  assert(t.play() && t.active == 2);
  assert(!t.replay(3) && t.position == 500);
  assert(!t.observe(701));
  static const ReadingCue overlapping[] = {{"a", 0, 100}, {"b", 99, 200}};
  assert(!t.initialize(overlapping, 2, 300));
  assert(!t.play() && !t.replay(0));
  assert(!t.initialize(nullptr, 1, 300));
  assert(!t.initialize(cues, 3, 599));
  return 0;
}
