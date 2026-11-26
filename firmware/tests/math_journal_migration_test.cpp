#include "math_journal_migration.h"
#include <cassert>
using R=MathMigrationResult;
using namespace FakeNvs;

static void submit(MathJournal& journal,MathPractice& state,unsigned choice,bool ink=false) {
  static MathScratchpad scratch;
  static MathAttempt attempt;
  scratch_clear(scratch);
  if(ink) {
    assert(scratch_touch(scratch,656,184,true));
    assert(scratch_touch(scratch,700,200,false));
  }
  assert(make_math_attempt(state,choice,scratch,attempt));
  assert(journal.append(attempt,state,state));
}
int main() {
  static MathJournal source,dest(MathJournalTarget::DedicatedStaging);
  MathPractice s,d,out;
  reset(); dedicated_initialized=true;
  assert(source.begin(s)==JournalLoad::Empty&&dest.begin(d)==JournalLoad::Empty);
  assert(MathJournalMigration::copy(source,source,out)==R::Unavailable);
  assert(MathJournalMigration::copy(dest,source,out)==R::Unavailable);
  assert(MathJournalMigration::copy(source,dest,out)==R::Complete&&out.attempts==0);
  submit(source,s,0,true); submit(source,s,1); submit(source,s,0);
  const auto original=records["pljournal"];
  writes_before_full=1;
  assert(MathJournalMigration::copy(source,dest,out)==R::Full&&out.attempts==0);
  assert(records["dedicated/pljournal"].size()==1&&records["pljournal"]==original);
  assert(MathJournalMigration::copy(source,dest,out)==R::Unavailable);
  writes_before_full=-1;
  assert(dest.begin(d)==JournalLoad::Restored&&d.attempts==1);
  assert(MathJournalMigration::copy(source,dest,out)==R::Complete&&out.attempts==3);
  assert(records["dedicated/pljournal"]==original&&records["pljournal"]==original);
  writes_before_full=0;
  assert(MathJournalMigration::copy(source,dest,out)==R::Complete); // no rewrite
  writes_before_full=-1;
  assert(dest.begin(d)==JournalLoad::Restored&&d.attempts==3);
  // Equal grading state but different ink must be rejected.
  reset(); dedicated_initialized=true;
  assert(source.begin(s)==JournalLoad::Empty&&dest.begin(d)==JournalLoad::Empty);
  submit(source,s,0,true); submit(dest,d,0,false);
  const auto divergent=records;
  assert(MathJournalMigration::copy(source,dest,out)==R::Conflict&&records==divergent);
  // Corrupt source suffix must be rejected BEFORE copying the good prefix.
  reset(); dedicated_initialized=true;
  assert(source.begin(s)==JournalLoad::Empty&&dest.begin(d)==JournalLoad::Empty);
  submit(source,s,0); submit(source,s,1);
  records["pljournal"]["a002"][0]^=1;
  assert(MathJournalMigration::copy(source,dest,out)==R::InvalidSource);
  assert(records["dedicated/pljournal"].empty());
  // Recover from lost commit acknowledgment, then exact retry.
  reset(); dedicated_initialized=true;
  assert(source.begin(s)==JournalLoad::Empty&&dest.begin(d)==JournalLoad::Empty);
  submit(source,s,0); submit(source,s,1);
  ambiguous=true;
  assert(MathJournalMigration::copy(source,dest,out)==R::Interrupted);
  ambiguous=false;
  assert(dest.begin(d)==JournalLoad::Restored&&d.attempts==1);
  assert(MathJournalMigration::copy(source,dest,out)==R::Complete&&out.attempts==2);
  // Copy a legacy summary without creating fictional historical records.
  reset(); dedicated_initialized=true;
  MathPractice old; math_answer(old,0); math_answer(old,1);
  MathCheckpoint base{}; assert(math_encode(old,base));
  records["pocketlearn"]["mathv1"]={base.begin(),base.end()};
  assert(source.begin(s)==JournalLoad::Restored&&s.attempts==2);
  assert(dest.begin(d)==JournalLoad::Empty);
  submit(source,s,0,true);
  const auto legacy_source=records["pljournal"];
  writes_before_full=1; // base committed, first attempt not committed
  assert(MathJournalMigration::copy(source,dest,out)==R::Full);
  writes_before_full=-1;
  assert(dest.begin(d)==JournalLoad::Restored&&d.attempts==2);
  assert(MathJournalMigration::copy(source,dest,out)==R::Complete&&out.attempts==3);
  assert(records["dedicated/pljournal"]==legacy_source&&records["pljournal"]==legacy_source);
  assert(dest.begin(d)==JournalLoad::Restored&&d.attempts==3);
}
