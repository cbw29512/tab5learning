#include "math_journal.h"
#include <cassert>

static bool submit(MathJournal& journal,MathPractice& state,unsigned choice) {
  static MathScratchpad ink;
  static MathAttempt attempt;
  return make_math_attempt(state,choice,ink,attempt)&&journal.append(attempt,state,state);
}
int main() {
  using namespace FakeNvs;
  reset();
  static MathJournal factory,stage(MathJournalTarget::DedicatedStaging);
  MathPractice original,staged;
  assert(factory.begin(original)==JournalLoad::Empty);
  assert(submit(factory,original,0)&&original.attempts==1);
  const auto saved=records["pljournal"];
  assert(stage.begin(staged)==JournalLoad::Error&&records["pljournal"]==saved);
  // Even with a valid legacy checkpoint, dedicated staging starts isolated.
  MathCheckpoint legacy{}; assert(math_encode(original,legacy));
  records["pocketlearn"]["mathv1"]={legacy.begin(),legacy.end()};
  dedicated_initialized=true;
  assert(stage.begin(staged)==JournalLoad::Empty&&staged.attempts==0);
  for(unsigned i=0;i<800;++i) assert(submit(stage,staged,0));
  assert(stage.begin(staged)==JournalLoad::Restored&&staged.attempts==800);
  assert(records["pljournal"]==saved&&records["dedicated/pljournal"].size()==800);
  assert(factory.begin(original)==JournalLoad::Restored&&original.attempts==1);
  // The portable backup limit is enforced even on a bigger partition.
  for(unsigned i=800;i<math_journal_max_records;++i) assert(submit(stage,staged,0));
  assert(!submit(stage,staged,0)&&stage.failure()==JournalFailure::Full);
  assert(stage.begin(staged)==JournalLoad::Restored&&staged.attempts==math_journal_max_records);
  auto& dest=records["dedicated/pljournal"];
  dest["a4097"]=dest["a001"];
  assert(stage.begin(staged)==JournalLoad::Error&&stage.failure()==JournalFailure::Conflict);
  dest.erase("a4097"); dest["a0001"]=dest["a001"];
  assert(stage.begin(staged)==JournalLoad::Error);
  dest.erase("a0001"); dest.erase("a002");
  assert(stage.begin(staged)==JournalLoad::Error);
  fail_inventory=true;
  assert(stage.begin(staged)==JournalLoad::Error&&records["pljournal"]==saved);
}
