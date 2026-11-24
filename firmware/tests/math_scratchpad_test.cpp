#include "math_scratchpad.h"
#include <cassert>
int main() {
  assert(math_problem_card.w==math_ink_card.w);
  assert(math_problem_card.h==math_ink_card.h);
  assert(math_problem_card.y==math_ink_card.y);
  assert(scratch_inside(656,184));
  assert(scratch_inside(1247,427));
  assert(!scratch_inside(655,184));
  assert(!scratch_inside(1248,427));
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
  assert(scratch_clear_hit(1100,140));
  assert(!scratch_clear_hit(1248,140));
  assert(!scratch_inside(700,444));
}
