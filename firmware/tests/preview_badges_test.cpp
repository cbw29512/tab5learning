#include "preview_badges.h"
#include <cassert>
int main() {
  MathPractice s;
  assert(preview_badge_count(s)==0);
  math_answer(s,0);
  math_answer(s,1);
  assert(preview_badge_count(s)==0);
  math_answer(s,0);
  assert(preview_badge_count(s)==1);
  MathCheckpoint b;
  assert(math_encode(s,b));
  MathPractice restored;
  assert(math_decode(b,restored));
  assert(preview_badge_count(restored)==1);
  math_answer(restored,2);
  math_answer(restored,3);
  math_answer(restored,0);
  assert(preview_badge_count(restored)==1);
  MathPractice perfect;
  for(unsigned choice:{1u,0u,2u,3u,0u}) math_answer(perfect,choice);
  assert(preview_badge_count(perfect)==2);
}
