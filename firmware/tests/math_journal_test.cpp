#include "math_journal.h"
#include <cassert>

static bool submit(MathJournal& j,MathPractice& state,unsigned choice) {
  MathScratchpad scratch;
  MathAttempt a;
  return make_math_attempt(state,choice,scratch,a)&&j.append(a,state,state);
}
int main() {
  Preferences::reset();
  MathPractice state;
  MathJournal first;
  assert(first.begin(state)==JournalLoad::Empty);
  assert(submit(first,state,0)&&state.retried);
  assert(submit(first,state,1)&&state.index==1);
  MathJournal reboot;
  MathPractice restored;
  assert(reboot.begin(restored)==JournalLoad::Restored);
  assert(restored.attempts==2&&restored.index==1&&restored.independent==0);
  Preferences::fail_write=true;
  assert(!submit(reboot,restored,0)&&restored.attempts==2);
  Preferences::fail_write=false;
  assert(!submit(reboot,restored,0)); // Lock after ambiguous storage failure.
  MathJournal recovery;
  assert(recovery.begin(restored)==JournalLoad::Restored);
  Preferences::ambiguous=true;
  assert(!submit(recovery,restored,0)&&restored.attempts==2);
  Preferences::ambiguous=false;
  MathJournal next_boot;
  assert(next_boot.begin(restored)==JournalLoad::Restored&&restored.attempts==3);
  assert(restored.index==2&&restored.independent==1);
  Preferences::records.erase("a002");
  MathJournal gap;
  assert(gap.begin(restored)==JournalLoad::Error&&restored.attempts==3);
  Preferences::reset();
  MathJournal fresh;
  assert(fresh.begin(state)==JournalLoad::Empty);
  assert(submit(fresh,state,1));
  Preferences::records["a001"][0]^=1;
  MathJournal corrupt;
  assert(corrupt.begin(restored)==JournalLoad::Error);
  Preferences::reset();
  Preferences::fail_open=true;
  MathJournal unavailable;
  assert(unavailable.begin(state)==JournalLoad::Error);
  Preferences::reset();
  MathJournal full;
  assert(full.begin(state)==JournalLoad::Empty);
  for(unsigned i=0;i<32;++i) assert(submit(full,state,0));
  assert(!submit(full,state,0)&&state.attempts==32);
  assert(Preferences::records.size()==32);
}
