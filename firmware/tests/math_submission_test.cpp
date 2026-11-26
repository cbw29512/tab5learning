#include "math_submission.h"
#include <cassert>

int main() {
  FakeNvs::reset();
  static MathJournal journal;
  static MathScratchpad scratch;
  static MathSubmission submission;
  MathPractice state;
  MathResult feedback=MathResult::Ignored;
  assert(journal.begin(state)==JournalLoad::Empty);
  assert(scratch_touch(scratch,656,184,true));
  for(unsigned i=1;i<kMaxInkPoints;++i)
    assert(scratch_touch(scratch,656+i,185,false));
  assert(scratch.ink.count==kMaxInkPoints);
  assert(submission.submit(journal,state,scratch,0,feedback)==MathSubmitStatus::Saved);
  assert(state.attempts==1&&feedback==MathResult::TryAgain&&scratch.ink.count==kMaxInkPoints);
  assert(FakeNvs::records["pljournal"]["a001"].size()==28+5*kMaxInkPoints);
  FakeNvs::fail_write=true;
  assert(submission.submit(journal,state,scratch,1,feedback)==MathSubmitStatus::StorageError);
  assert(state.attempts==1&&feedback==MathResult::TryAgain&&scratch.ink.count==kMaxInkPoints);
  FakeNvs::fail_write=false;
  assert(journal.begin(state)==JournalLoad::Restored&&state.attempts==1);
  assert(submission.submit(journal,state,scratch,1,feedback)==MathSubmitStatus::Saved);
  assert(state.attempts==2&&state.index==1&&scratch.ink.count==0);
  assert(feedback==MathResult::Correct);
  assert(submission.submit(journal,state,scratch,4,feedback)==MathSubmitStatus::Rejected);
  assert(state.attempts==2);
  assert(journal.begin(state)==JournalLoad::Restored&&state.attempts==2);
}
