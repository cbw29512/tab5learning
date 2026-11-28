#include "math_attempt_codec.h"
#include <cassert>
int main() {
  MathPractice s;
  MathScratchpad scratch;
  MathAttempt a;
  AttemptBytes b{};
  std::size_t n=0;
  assert(scratch_touch(scratch,700,200,true));
  assert(make_math_attempt(s,0,scratch,a));
  assert(attempt_encode(a,b,n)&&n==33);
  auto damaged=b;
  damaged[24]^=1;
  assert(!attempt_replay(damaged,n,s)&&s.attempts==0);
  assert(!attempt_replay(b,n-1,s));
  assert(attempt_replay(b,n,s)&&s.retried&&s.attempts==1);
  assert(!attempt_replay(b,n,s)&&s.attempts==1);
  assert(make_math_attempt(s,1,scratch,a));
  assert(attempt_encode(a,b,n));
  assert(attempt_replay(b,n,s)&&s.index==1&&s.independent==0);
}
