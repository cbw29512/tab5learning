#include "math_checkpoint.h"
#include <cassert>
int main() {
  MathPractice source, restored;
  MathCheckpoint bytes;
  assert(math_answer(source,0)==MathResult::TryAgain);
  assert(math_encode(source,bytes));
  assert(math_decode(bytes,restored));
  assert(restored.retried && restored.attempts==1);
  assert(math_answer(restored,1)==MathResult::Correct);
  assert(restored.independent==0 && restored.index==1);
  assert(math_encode(restored,bytes));
  for(unsigned i=0;i<bytes.size();++i) {
    auto corrupt=bytes; corrupt[i]^=1;
    assert(!math_decode(corrupt,restored));
    assert(restored.index==1);
  }
  source.index=99;
  assert(!math_encode(source,bytes));
  MathPractice finished;
  for(unsigned choice:{1u,0u,2u,3u,0u}) math_answer(finished,choice);
  assert(math_encode(finished,bytes));
  assert(math_decode(bytes,restored));
  assert(restored.finished && restored.independent==5);
}
