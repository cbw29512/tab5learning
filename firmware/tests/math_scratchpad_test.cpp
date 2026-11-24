#include "math_scratchpad.h"
#include <cassert>
int main() {
  MathScratchpad s;
  assert(!scratch_touch(s,600,200,true));
  assert(!scratch_touch(s,700,200,false));
  assert(scratch_touch(s,700,200,true));
  assert(scratch_touch(s,710,210,false));
  assert(!scratch_touch(s,1250,210,false));
  assert(!scratch_touch(s,720,220,false));
  scratch_release(s);
  assert(scratch_touch(s,720,220,true));
  assert(ink_stroke_count(s.ink)==2);
  assert(s.ink.count==3);
  scratch_clear(s);
  assert(s.ink.count==0 && !s.drawing);
  assert(scratch_clear_hit(1100,108));
  assert(!scratch_clear_hit(1248,108));
  assert(!scratch_inside(700,300));
}
