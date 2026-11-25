#include "reading_pcm_clock.h"
#include <cassert>

int main() {
  PcmSourceClock clock;
  PcmRange range{};
  uint32_t ms=0;
  assert(!clock.range(0,1,range));
  assert(!clock.initialize(7999,16000));
  assert(!clock.initialize(48001,16000));
  assert(!clock.initialize(16000,0));
  assert(!clock.initialize(48000,1));
  assert(clock.initialize(16000,16000));
  assert(clock.duration_ms()==1000);
  assert(clock.range(100,250,range));
  assert(range.first==1600 && range.end==4000);
  assert(clock.position(range.first,ms) && ms==100);
  assert(clock.position(range.end,ms) && ms==250);
  assert(!clock.range(250,250,range));
  assert(!clock.range(0,1001,range));
  assert(!clock.position(16001,ms));
  assert(clock.initialize(44100,44101));
  PcmRange next{};
  assert(clock.range(1,2,range));
  assert(range.first==45 && range.end==89);
  assert(clock.position(range.first,ms) && ms==1);
  assert(clock.range(2,3,next) && next.first==range.end);
  assert(clock.range(999,1000,range) && range.end==44101);
  assert(clock.position(44101,ms) && ms==1000);
  assert(!clock.initialize(0,44100));
  assert(clock.duration_ms()==0 && !clock.position(0,ms));
  return 0;
}
