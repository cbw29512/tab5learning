#include "math_attempt.h"
#include <cassert>
int main() {
  MathPractice before,after;
  MathScratchpad scratch;
  MathAttempt a;
  assert(scratch_touch(scratch,656,184,true));
  assert(scratch_touch(scratch,700,200,false));
  assert(make_math_attempt(before,0,scratch,a));
  assert(a.sequence==1 && a.question==0 && a.choice==0);
  assert(a.result==MathResult::TryAgain && a.ink.count==2);
  assert(a.ink.points[0].x==0 && a.ink.points[0].y==0);
  assert(a.ink.points[1].x==44 && a.ink.points[1].y==16);
  assert(math_decode(a.after,after)&&after.retried);
  assert(before.attempts==0);
  assert(make_math_attempt(after,1,scratch,a));
  assert(a.sequence==2 && a.result==MathResult::Correct);
  assert(math_decode(a.after,after)&&after.independent==0);
  scratch.ink.overflow=true;
  assert(!make_math_attempt(after,0,scratch,a));
  scratch_clear(scratch);
  assert(!make_math_attempt(after,4,scratch,a));
  assert(make_math_attempt(after,0,scratch,a)&&a.ink.count==0);
}
