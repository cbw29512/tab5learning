#include "math_journal.h"
#include <cassert>
#include <vector>
#include <algorithm>
static bool submit(MathJournal& j,MathPractice& state,unsigned choice) {
  MathScratchpad scratch; MathAttempt a;
  return make_math_attempt(state,choice,scratch,a)&&j.append(a,state,state);
}
static void seed_legacy(const MathPractice& state) {
  MathCheckpoint b{}; assert(math_encode(state,b));
  FakeNvs::records["pocketlearn"]["mathv1"]={b.begin(),b.end()};
}
// Test sink never publishes partial records.
class SnapshotSink final : public MathSnapshotSink {
 public:
  MathSnapshot manifest{};
  std::vector<FakeNvs::Bytes> staged, published;
  unsigned fail_slot=0;
  bool fail_begin=false,fail_commit=false,committed=false,aborted=false;
  bool begin(const MathSnapshot& value) noexcept override {
    manifest=value; staged.clear(); return !fail_begin;
  }
  bool record(std::uint32_t slot,const AttemptBytes& bytes,std::size_t n) noexcept override {
    if(slot==fail_slot) return false;
    staged.emplace_back(bytes.begin(),bytes.begin()+n); return true;
  }
  bool commit() noexcept override {
    if(fail_commit) return false;
    published=staged; committed=true; return true;
  }
  void abort() noexcept override { staged.clear(); aborted=true; }
};
static void snapshot_tests() {
  using namespace FakeNvs;
  reset(); MathJournal journal; MathPractice state;
  assert(journal.begin(state)==JournalLoad::Empty);
  SnapshotSink empty;
  assert(journal.snapshot(empty)==MathSnapshotResult::Complete&&empty.committed);
  assert(empty.manifest.count==0&&empty.published.empty());
  MathScratchpad ink;
  assert(scratch_touch(ink,656,184,true));
  assert(scratch_touch(ink,700,200,false));
  MathAttempt first;
  assert(make_math_attempt(state,0,ink,first)&&journal.append(first,state,state));
  assert(submit(journal,state,1));
  const auto unchanged=records;
  SnapshotSink good;
  assert(journal.snapshot(good)==MathSnapshotResult::Complete);
  assert(good.committed&&good.manifest.count==2&&good.published.size()==2);
  assert(good.published[0]==records["pljournal"]["a001"]);
  assert(good.published[1]==records["pljournal"]["a002"]&&records==unchanged);
  MathSnapshotVerifier verifier;
  assert(verifier.begin(good.manifest)&&!verifier.complete());
  AttemptBytes bytes{};
  std::copy(good.published[0].begin(),good.published[0].end(),bytes.begin());
  assert(!verifier.record(2,bytes,good.published[0].size())&&!verifier.complete());
  assert(verifier.begin(good.manifest));
  assert(verifier.record(1,bytes,good.published[0].size())&&!verifier.complete());
  auto wrong=good.manifest; ++wrong.count; assert(!verifier.begin(wrong));
  SnapshotSink broken; broken.fail_slot=2;
  assert(journal.snapshot(broken)==MathSnapshotResult::SinkFailure);
  assert(broken.aborted&&!broken.committed&&broken.staged.empty());
  SnapshotSink bad_start; bad_start.fail_begin=true;
  assert(journal.snapshot(bad_start)==MathSnapshotResult::SinkFailure&&bad_start.aborted);
  SnapshotSink bad_end; bad_end.fail_commit=true;
  assert(journal.snapshot(bad_end)==MathSnapshotResult::SinkFailure&&bad_end.aborted&&!bad_end.committed);
  records["pljournal"]["a757"]=records["pljournal"]["a001"];
  SnapshotSink extra;
  assert(journal.snapshot(extra)==MathSnapshotResult::Invalid&&!extra.committed);
  assert(records["pljournal"].count("a757"));
  records["pljournal"].erase("a757");
  records["pljournal"]["a002"][0]^=1; SnapshotSink corrupt;
  assert(journal.snapshot(corrupt)==MathSnapshotResult::Invalid&&corrupt.aborted&&!corrupt.committed);
  reset(); MathPractice old; math_answer(old,0); math_answer(old,1); seed_legacy(old);
  assert(journal.begin(state)==JournalLoad::Restored&&state.attempts==2);
  assert(submit(journal,state,0));
  SnapshotSink migrated;
  assert(journal.snapshot(migrated)==MathSnapshotResult::Complete);
  MathPractice base,final;
  assert(math_decode(migrated.manifest.base,base)&&base.attempts==2);
  assert(math_decode(migrated.manifest.final,final)&&final.attempts==3);
  assert(migrated.manifest.count==1&&migrated.published.size()==1);
  full=true; assert(!submit(journal,state,0));
  SnapshotSink unavailable;
  assert(journal.snapshot(unavailable)==MathSnapshotResult::Unavailable&&!unavailable.committed);
  full=false; assert(journal.begin(state)==JournalLoad::Restored);
  SnapshotSink recovered;
  assert(journal.snapshot(recovered)==MathSnapshotResult::Complete);
  records["pljournal"]["basev1"][0]^=1;
  SnapshotSink damaged_base;
  assert(journal.snapshot(damaged_base)==MathSnapshotResult::Invalid&&!damaged_base.committed);
}
int main() {
  snapshot_tests();
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
