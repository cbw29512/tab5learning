#include "math_journal_migration.h"
#include "math_journal_restore.h"
#include <cassert>
using namespace FakeNvs;
using R=MathMigrationResult;
static void submit(MathJournal& j,MathPractice& s,unsigned x) {
  static MathScratchpad pad;
  static MathAttempt a;
  scratch_clear(pad);
  assert(scratch_touch(pad,x,200,true));
  assert(make_math_attempt(s,0,pad,a)&&j.append(a,s,s));
}
int main() {
  static MathJournal source,dest(MathJournalTarget::DedicatedStaging);
  MathPractice s,d,out;
  reset(); dedicated_initialized=true;
  assert(source.begin(s)==JournalLoad::Empty&&dest.begin(d)==JournalLoad::Empty);
  submit(source,s,700); submit(source,s,701);
  const auto original=records["pljournal"];
  // Two records fit; marker does not. Recovery and retry must preserve bytes.
  writes_before_full=2;
  assert(MathJournalMigration::copy_and_seal(source,dest,out)==R::Full&&out.attempts==0);
  assert(!records["dedicated/pljournal"].count("copyv1"));
  writes_before_full=-1;
  assert(dest.begin(d)==JournalLoad::Restored&&d.attempts==2);
  ambiguous=true;
  assert(MathJournalMigration::copy_and_seal(source,dest,out)==R::Interrupted);
  ambiguous=false;
  assert(dest.begin(d)==JournalLoad::Restored&&d.attempts==2);
  const auto sealed=records["dedicated/pljournal"];
  writes_before_full=0;
  assert(MathJournalMigration::copy_and_seal(source,dest,out)==R::Complete&&out.attempts==2);
  assert(records["dedicated/pljournal"]==sealed&&records["pljournal"]==original);
  static MathScratchpad pad; static MathAttempt a;
  assert(make_math_attempt(d,0,pad,a)&&!dest.append(a,d,d));
  assert(MathJournalRestorer::apply(dest,{},d)==MathRestoreResult::Unavailable);
  writes_before_full=-1;
  submit(source,s,702);
  assert(MathJournalMigration::copy_and_seal(source,dest,out)==R::Conflict);
  // Detect missing tail even though remaining records form a valid prefix.
  records["dedicated/pljournal"].erase("a002");
  assert(dest.begin(d)==JournalLoad::Error&&d.attempts==2);
  records["dedicated/pljournal"]=sealed;
  // A different valid ink trace with identical grading state is not accepted.
  records["dedicated/pljournal"]["a001"]=sealed.at("a002");
  assert(dest.begin(d)==JournalLoad::Error);
  records["dedicated/pljournal"]=sealed;
  records["dedicated/pljournal"]["copyv1"][0]^=1;
  assert(dest.begin(d)==JournalLoad::Error);
  records["dedicated/pljournal"]=sealed;
  assert(dest.begin(d)==JournalLoad::Restored&&d.attempts==2);
}
