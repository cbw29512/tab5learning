#include "math_journal.h"
#include <cassert>
static bool submit(MathJournal& j,MathPractice& state,unsigned choice) {
  MathScratchpad scratch; MathAttempt a;
  return make_math_attempt(state,choice,scratch,a)&&j.append(a,state,state);
}
static void seed_legacy(const MathPractice& state) {
  MathCheckpoint b{}; assert(math_encode(state,b));
  FakeNvs::records["pocketlearn"]["mathv1"]={b.begin(),b.end()};
}
int main() {
  using namespace FakeNvs;
  reset(); MathPractice state; MathJournal first;
  assert(first.begin(state)==JournalLoad::Empty);
  assert(submit(first,state,0)&&state.retried);
  assert(submit(first,state,1)&&state.index==1);
  MathJournal reboot; MathPractice restored;
  assert(reboot.begin(restored)==JournalLoad::Restored);
  assert(restored.attempts==2&&restored.index==1&&restored.independent==0);
  fail_write=true;
  assert(!submit(reboot,restored,0)&&restored.attempts==2);
  fail_write=false; assert(!submit(reboot,restored,0));
  MathJournal recovery; assert(recovery.begin(restored)==JournalLoad::Restored);
  ambiguous=true; assert(!submit(recovery,restored,0)&&restored.attempts==2);
  ambiguous=false; MathJournal next_boot;
  assert(next_boot.begin(restored)==JournalLoad::Restored&&restored.attempts==3);
  assert(restored.index==2&&restored.independent==1);
  records["pljournal"].erase("a002"); MathJournal gap;
  assert(gap.begin(restored)==JournalLoad::Error&&restored.attempts==3);
  reset(); MathJournal fresh; assert(fresh.begin(state)==JournalLoad::Empty);
  assert(submit(fresh,state,1)); records["pljournal"]["a001"][0]^=1;
  assert(gap.begin(restored)==JournalLoad::Error);
  reset(); fail_open=true; assert(gap.begin(state)==JournalLoad::Error);
  reset(); MathJournal capacity; assert(capacity.begin(state)==JournalLoad::Empty);
  for(unsigned i=0;i<100;++i) assert(submit(capacity,state,0));
  assert(state.attempts==100&&records["pljournal"].size()==100);
  full=true; assert(!submit(capacity,state,0)&&state.attempts==100);
  assert(capacity.failure()==JournalFailure::Full);
  full=false; assert(capacity.begin(restored)==JournalLoad::Restored&&restored.attempts==100);
  fail_read=true; assert(capacity.begin(restored)==JournalLoad::Error&&restored.attempts==100);
  reset(); MathPractice old; math_answer(old,0); math_answer(old,1); seed_legacy(old);
  const auto original=records["pocketlearn"]["mathv1"];
  MathJournal migration; assert(migration.begin(state)==JournalLoad::Restored&&state.attempts==2);
  assert(records["pljournal"].count("basev1")&&records["pljournal"].size()==1);
  assert(submit(migration,state,0)&&state.attempts==3&&records["pljournal"].count("a001"));
  assert(migration.begin(restored)==JournalLoad::Restored&&restored.attempts==3&&restored.index==2);
  assert(records["pocketlearn"]["mathv1"]==original);
  reset(); seed_legacy(old); ambiguous=true;
  assert(migration.begin(restored)==JournalLoad::Error&&restored.attempts==3);
  ambiguous=false; assert(migration.begin(restored)==JournalLoad::Restored&&restored.attempts==2);
  reset(); assert(migration.begin(state)==JournalLoad::Empty); assert(submit(migration,state,1));
  seed_legacy(old); assert(migration.begin(restored)==JournalLoad::Error);
  assert(migration.failure()==JournalFailure::Conflict&&restored.attempts==2);
  reset(); assert(migration.begin(state)==JournalLoad::Empty); assert(submit(migration,state,0));
  seed_legacy(state); assert(migration.begin(restored)==JournalLoad::Restored);
  assert(restored.attempts==1&&!records["pljournal"].count("basev1"));
}
