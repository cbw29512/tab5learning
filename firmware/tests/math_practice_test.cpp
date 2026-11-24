#include "math_practice.h"
#include <cassert>
int main() {
  MathPractice s;
  assert(math_answer(s,4)==MathResult::Ignored);
  assert(math_answer(s,0)==MathResult::TryAgain);
  assert(s.index==0 && s.attempts==1);
  assert(math_answer(s,1)==MathResult::Correct);
  assert(s.independent==0);
  assert(math_answer(s,0)==MathResult::Correct);
  assert(math_answer(s,2)==MathResult::Correct);
  assert(math_answer(s,3)==MathResult::Correct);
  assert(math_answer(s,0)==MathResult::Finished);
  assert(s.finished && s.independent==4 && s.attempts==6);
  assert(math_answer(s,0)==MathResult::Ignored);
  for(int i=0;i<4;++i) assert(math_choice_at(32+i*308,300)==i);
  assert(math_choice_at(324,300)==-1);
  assert(math_choice_at(32,540)==-1);
}
