#include "curriculum_skill_writer.h"
#include <cassert>
#include <vector>
using namespace curriculum_preview;
struct Media:SkillHistoryBackend {
  std::vector<SkillEventBytes> records;
  bool reject=false,ambiguous=false,partial=false,bad_read=false;
  unsigned calls=0;
  bool size(std::uint64_t& n)override{
    n=records.size()*SkillEventBytes{}.size()+unsigned(partial);return true;
  }
  bool read(unsigned slot,SkillEventBytes& b)override{
    if(slot>=records.size())return false;
    b=records[slot];if(bad_read)b[98]^=1;return true;
  }
  bool append_synced(unsigned expected,const SkillEventBytes& b)override{
    ++calls;if(reject||expected!=records.size())return false;
    records.push_back(b);return !ambiguous;
  }
};
int main(){
  Media m;SkillHistoryWriter first(m);
  assert(!first.available()&&first.begin()&&!first.begin());
  Preview before{};assert(before.choose_subject(0)&&before.choose_lesson(0));
  while(!independent_choice(before.activity())){
    if(before.activity().kind==Kind::Choice)assert(before.answer(0)&&before.next());
    else assert(before.next());
  }
  auto after=before;assert(after.answer((unsigned(before.activity().answer)+1)%before.activity().count));
  assert(first.observe(before,before)==SkillWriteResult::Ignored&&m.calls==0);
  auto review=after;review.reviewing=true;
  assert(first.observe(before,review)==SkillWriteResult::Ignored);
  assert(first.observe(before,after)==SkillWriteResult::Saved);
  assert(first.observe(before,after)==SkillWriteResult::AlreadyObserved&&m.calls==1);
  assert(first.skill(before.lesson).observations==1&&first.skill(before.lesson).correct==0);
  SkillEvent e{};assert(skill_decode(m.records[0],e)&&e.session==1&&e.sequence==1);
  // A boot with no answers must not allocate a durable session number.
  {SkillHistoryWriter empty(m);assert(empty.begin());}
  SkillHistoryWriter second(m);assert(second.begin());
  after.selected=after.activity().answer;
  assert(second.observe(before,after)==SkillWriteResult::Saved);
  assert(skill_decode(m.records[1],e)&&e.session==2&&e.sequence==2);
  auto p=second.skill(before.lesson);
  assert(p.known&&p.observations==2&&p.correct==1&&p.sessions==2);
  // Append may succeed physically but return an error: stop; don't retry.
  SkillHistoryWriter uncertain(m);assert(uncertain.begin());m.ambiguous=true;
  assert(uncertain.observe(before,after)==SkillWriteResult::Unavailable);
  assert(m.records.size()==3&&!uncertain.skill(before.lesson).known);
  const auto calls=m.calls;m.ambiguous=false;
  assert(uncertain.observe(before,after)==SkillWriteResult::Unavailable&&m.calls==calls);
  assert(!uncertain.begin());
  SkillHistoryWriter reopened(m);assert(reopened.begin()&&reopened.verified_count()==3);
  assert(reopened.skill(before.lesson).correct==2);
  m.bad_read=true;
  assert(reopened.observe(before,after)==SkillWriteResult::Unavailable);
  m.bad_read=false;
  SkillHistoryWriter recovered(m);assert(recovered.begin()&&recovered.verified_count()==4);
  m.partial=true;
  assert(recovered.observe(before,after)==SkillWriteResult::Unavailable);
  assert(!recovered.available());m.partial=false;
  SkillHistoryWriter rejected(m);assert(rejected.begin());m.reject=true;
  assert(rejected.observe(before,after)==SkillWriteResult::Unavailable&&m.records.size()==4);
  m.reject=false;
  // Full log remains readable, never silently evicts evidence.
  Media full;
  for(unsigned i=1;i<=SkillHistoryReplay::max_records;++i){
    SkillEventBytes b{};e.sequence=i;e.session=i;
    assert(skill_encode(e,b));full.records.push_back(b);
  }
  SkillHistoryWriter capacity(full);assert(capacity.begin());
  assert(capacity.observe(before,after)==SkillWriteResult::Full&&full.calls==0&&capacity.available());
}
