#include "curriculum_transition.h"
#include "curriculum_resume_store.h"
#include <cassert>
#include <vector>
using namespace curriculum_preview;

struct MemoryMedia:CompletionBackend{
  std::vector<CompletionBytes> records;
  bool fail=false;
  bool size(std::uint64_t& n)override{n=records.size()*96;return true;}
  bool read(unsigned i,CompletionBytes& b)override{
    if(i>=records.size())return false;
    b=records[i];return true;
  }
  bool append(unsigned expected,const CompletionBytes& b)override{
    if(fail||expected!=records.size())return false;
    records.push_back(b);return true;
  }
};
Preview last_activity(){
  Preview p;
  assert(p.choose_subject(0)&&p.change_page(1)&&p.choose_lesson(0));
  while(p.step+1<p.current().count){
    if(p.screen==Screen::Activity&&p.activity().kind==Kind::Choice)assert(p.answer(0));
    else assert(p.next());
  }
  return p;
}
int main(){
  FakeNvs::reset();
  ResumeStore resume;assert(resume.begin()==ResumeLoad::Empty);
  MemoryMedia media;CompletionLedger ledger(media);assert(ledger.begin());
  Preview current{},candidate=current;
  assert(candidate.choose_subject(0));
  assert(commit_transition(current,candidate,resume,&ledger)==TransitionResult::Applied);
  assert(!resume.has_resume()&&media.records.empty());
  candidate=current;assert(candidate.choose_lesson(0));
  assert(commit_transition(current,candidate,resume,&ledger)==TransitionResult::Applied);
  assert(resume.has_resume()&&media.records.empty());
  current=last_activity();assert(resume.save(current));
  candidate=current;
  if(candidate.activity().kind==Kind::Choice)assert(candidate.answer(0));
  else if(candidate.activity().kind==Kind::AdultReview)assert(candidate.next());
  if(candidate.screen==Screen::Feedback){
    assert(commit_transition(current,candidate,resume,&ledger)==TransitionResult::Applied);
    assert(media.records.empty());
    assert(candidate.next());
  }else assert(candidate.next());
  assert(candidate.screen==Screen::Finished);
  // Storage failure cannot display a finished lesson.
  media.fail=true;
  assert(commit_transition(current,candidate,resume,&ledger)==TransitionResult::CompletionError);
  assert(current.screen!=Screen::Finished&&media.records.empty());
  media.fail=false;assert(ledger.begin());
  // SD append succeeds but checkpoint fails. Preserve committed completion.
  FakeNvs::full=true;
  assert(commit_transition(current,candidate,resume,&ledger)==TransitionResult::ResumeError);
  assert(current.screen!=Screen::Finished&&media.records.size()==1);
  // Restart with the earlier checkpoint; repeat final activity and commit
  // the same lesson. No duplicate ledger row or incorrect page restoration.
  FakeNvs::full=false;
  ResumeStore reboot;assert(reboot.begin()==ResumeLoad::Restored);
  CompletionLedger replay(media);assert(replay.begin());
  assert(reboot.restore(current)&&current.page==1&&current.screen==Screen::Activity);
  candidate=current;
  while(candidate.screen!=Screen::Finished){
    if(candidate.screen==Screen::Activity&&candidate.activity().kind==Kind::Choice)assert(candidate.answer(0));
    else assert(candidate.next());
  }
  assert(commit_transition(current,candidate,reboot,&replay)==TransitionResult::Applied);
  assert(current.screen==Screen::Finished&&media.records.size()==1);
  ResumeStore done;assert(done.begin()==ResumeLoad::Restored&&done.restore(current));
  assert(current.page==1&&current.screen==Screen::Finished);
  // Reviewing a verified completion never writes a second ledger entry or
  // changes the existing resume checkpoint, even if NVS cannot write.
  const auto saved=current;auto review=current;review.reviewing=true;
  review.screen=Screen::Activity;review.step=0;
  FakeNvs::full=true;
  assert(commit_transition(current,review,done,&replay)==TransitionResult::Applied);
  Preview original;assert(done.restore(original)&&original.step==saved.step&&original.screen==saved.screen);
  assert(media.records.size()==1);
  assert(commit_transition(current,review,done,nullptr)==TransitionResult::CompletionError);
  FakeNvs::full=false;
  // Current normal preview still saves only location; no completion backend.
  FakeNvs::reset();ResumeStore preview;assert(preview.begin()==ResumeLoad::Empty);
  current=last_activity();assert(commit_transition(current,candidate,preview)==TransitionResult::Applied);
  assert(media.records.size()==1&&preview.has_resume());
}
