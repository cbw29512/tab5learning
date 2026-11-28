#include "reading_pcm_clock.h"
#include <cassert>
#include <initializer_list>

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
  // Exhaustively verify adjacent millisecond ranges at common PCM rates.
  for (uint32_t rate : {8000u, 16000u, 22050u, 44100u, 48000u}) {
    assert(clock.initialize(rate, rate + 7));
    uint32_t previous=0;
    for (uint32_t tick=0; tick<clock.duration_ms(); ++tick) {
      assert(clock.range(tick,tick+1,range));
      assert(range.first==previous);
      assert(clock.position(range.first,ms) && ms==tick);
      previous=range.end;
    }
    assert(previous==rate+7);
  }
  // Maximum sample count must use wide arithmetic without wrapping.
  assert(clock.initialize(8000,UINT32_MAX));
  assert(clock.position(UINT32_MAX,ms) && ms==536870911u);
  assert(clock.range(ms-1,ms,range) && range.end==UINT32_MAX);
  return 0;
}
