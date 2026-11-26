#include "math_journal_restore.h"
#include <cassert>
#include <vector>

struct Image {
  MathSnapshot manifest{};
  std::vector<std::uint8_t> data;
  MathPractice state{};
  explicit Image(MathPractice base={}) : state(base) {
    assert(math_encode(base,manifest.base));
    manifest.final=manifest.base;
  }
  void add(unsigned choice,bool ink=false) {
    MathScratchpad scratch;
    if(ink) {
      assert(scratch_touch(scratch,656,184,true));
      assert(scratch_touch(scratch,700,200,false));
    }
    MathAttempt a; AttemptBytes b{}; std::size_t n=0;
    assert(make_math_attempt(state,choice,scratch,a));
    assert(attempt_encode(a,b,n)&&attempt_replay(b,n,state));
    data.push_back(n&255u); data.push_back(n>>8);
    data.insert(data.end(),b.begin(),b.begin()+n);
    ++manifest.count; assert(math_encode(state,manifest.final));
  }
  MathRestoreImage view() const { return {manifest,data.data(),data.size()}; }
};
static void seed_legacy(const MathPractice& state) {
  MathCheckpoint b{}; assert(math_encode(state,b));
  FakeNvs::records["pocketlearn"]["mathv1"]={b.begin(),b.end()};
}
int main() {
  using namespace FakeNvs;
  using R=MathRestoreResult;
  reset(); MathJournal journal; MathPractice state;
  assert(MathJournalRestorer::apply(journal,Image{}.view(),state)==R::Unavailable);
  assert(journal.begin(state)==JournalLoad::Empty);
  Image empty;
  assert(MathJournalRestorer::apply(journal,empty.view(),state)==R::Complete);
  Image prefix; prefix.add(0,true);
  assert(MathJournalRestorer::apply(journal,prefix.view(),state)==R::Complete&&state.attempts==1);
  const auto original=records["pljournal"]["a001"];
  Image complete=prefix; complete.add(1); complete.add(0); complete.add(2);
  assert(MathJournalRestorer::apply(journal,complete.view(),state)==R::Complete);
  assert(state.attempts==4&&state.index==3&&records["pljournal"]["a001"]==original);
  const auto unchanged=records;
  assert(MathJournalRestorer::apply(journal,complete.view(),state)==R::Complete&&records==unchanged);
  assert(MathJournalRestorer::apply(journal,prefix.view(),state)==R::Stale&&records==unchanged);
  Image divergent; divergent.add(2); divergent.add(1); divergent.add(0); divergent.add(2);
  assert(MathJournalRestorer::apply(journal,divergent.view(),state)==R::Conflict&&records==unchanged);
  Image invalid=complete; invalid.add(3); invalid.data.back()^=1;
  assert(MathJournalRestorer::apply(journal,invalid.view(),state)==R::Invalid&&records==unchanged);
  Image trailing=complete; trailing.data.push_back(0);
  assert(MathJournalRestorer::apply(journal,trailing.view(),state)==R::Invalid&&records==unchanged);
  Image bad_final=complete; bad_final.manifest.final[0]^=1;
  assert(MathJournalRestorer::apply(journal,bad_final.view(),state)==R::Invalid&&records==unchanged);
  auto null=complete.view(); null.data=nullptr;
  assert(MathJournalRestorer::apply(journal,null,state)==R::Invalid&&records==unchanged);
  reset(); assert(journal.begin(state)==JournalLoad::Empty);
  Image large; for(unsigned i=0;i<757;++i) large.add(0);
  assert(MathJournalRestorer::apply(journal,large.view(),state)==R::Full&&state.attempts==0);
  assert(records["pljournal"].empty());

  // A mid-restore capacity failure leaves only a validated committed prefix.
  reset(); assert(journal.begin(state)==JournalLoad::Empty);
  writes_before_full=2;
  assert(MathJournalRestorer::apply(journal,complete.view(),state)==R::Full&&state.attempts==2);
  assert(MathJournalRestorer::apply(journal,complete.view(),state)==R::Unavailable);
  writes_before_full=-1; assert(journal.begin(state)==JournalLoad::Restored&&state.attempts==2);
  assert(MathJournalRestorer::apply(journal,complete.view(),state)==R::Complete&&state.attempts==4);
  assert(records["pljournal"]["a001"]==original);

  // Simulated commit acknowledgment loss must reconcile before continuing.
  reset(); assert(journal.begin(state)==JournalLoad::Empty);
  ambiguous=true;
  assert(MathJournalRestorer::apply(journal,complete.view(),state)==R::Interrupted&&state.attempts==0);
  ambiguous=false; assert(journal.begin(state)==JournalLoad::Restored&&state.attempts==1);
  assert(MathJournalRestorer::apply(journal,complete.view(),state)==R::Complete&&state.attempts==4);
  assert(records["pljournal"]["a001"]==original);

  reset(); MathPractice old; math_answer(old,0); math_answer(old,1);
  Image migrated(old); migrated.add(0,true);
  assert(journal.begin(state)==JournalLoad::Empty);
  assert(MathJournalRestorer::apply(journal,migrated.view(),state)==R::Conflict&&state.attempts==0);
  seed_legacy(old); assert(journal.begin(state)==JournalLoad::Restored&&state.attempts==2);
  const auto legacy=records["pocketlearn"]["mathv1"];
  assert(MathJournalRestorer::apply(journal,migrated.view(),state)==R::Complete&&state.attempts==3);
  assert(records["pocketlearn"]["mathv1"]==legacy);
  assert(journal.begin(state)==JournalLoad::Restored&&state.attempts==3);
}
