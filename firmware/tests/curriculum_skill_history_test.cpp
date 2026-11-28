#include "curriculum_skill_history.h"
#include <cassert>
#include <vector>
using namespace curriculum_preview;
struct History:SkillHistoryReader {
  std::vector<SkillEventBytes> records;
  bool partial=false,failed=false;
  bool size(std::uint64_t& length)override{
    length=records.size()*SkillEventBytes{}.size()+unsigned(partial);return true;
  }
  bool read(unsigned i,SkillEventBytes& b)override{
    if(failed||i>=records.size())return false;
    b=records[i];return true;
  }
  void add(const SkillEvent& e){SkillEventBytes b{};assert(skill_encode(e,b));records.push_back(b);}
};
int main(){
  History media;SkillHistoryReplay replay;
  assert(!replay.skill(0).known);
  assert(replay.begin(media)&&replay.skill(0).known&&replay.verified_count()==0);
  CompletionKey key{};assert(completion_key(lessons[0],key));
  unsigned a=0,b=0;bool found=false;
  for(unsigned i=0;i<lessons[0].count;++i)if(independent_choice(lessons[0].activities[i])){
    if(!found){a=i;found=true;}else b=i;
  }
  assert(found&&a!=b);
  SkillEvent first{1,1,key,a,false};
  SkillEventBytes bytes{};assert(skill_encode(first,bytes));
  SkillEvent roundtrip{};assert(skill_decode(bytes,roundtrip));
  assert(roundtrip.lesson==key&&roundtrip.sequence==1&&roundtrip.session==1&&roundtrip.step==a&&!roundtrip.correct);
  media.add(first);media.add({2,1,key,b,true});media.add({3,2,key,a,true});
  auto historic=key;++historic.revision;
  media.add({4,2,historic,a,false});
  assert(replay.begin(media)&&replay.verified_count()==4);
  auto p=replay.skill(0);assert(p.known&&p.observations==3&&p.correct==2&&p.sessions==2);
  assert(p.error_sessions==1&&p.recent_error_sessions==0);
  SkillHistoryReplay reopened;assert(reopened.begin(media)&&reopened.skill(0).observations==3);
  assert(!reopened.skill(lesson_count).known);
  media.add({5,2,key,a,false}); // duplicate within session
  assert(!replay.begin(media)&&!replay.skill(0).known);
  media.records.pop_back();media.add({5,4,key,b,true}); // skipped session
  assert(!replay.begin(media));media.records.pop_back();
  media.add({6,3,key,a,true}); // skipped record
  assert(!replay.begin(media));media.records.pop_back();
  media.add({5,3,key,0,true}); // instructional activity
  assert(!replay.begin(media));media.records.pop_back();
  media.partial=true;assert(!replay.begin(media));media.partial=false;
  media.failed=true;assert(!replay.begin(media));media.failed=false;
  auto saved=media.records[0];media.records[0][98]^=1;
  assert(!replay.begin(media));media.records[0]=saved;
  assert(replay.begin(media)&&replay.skill(0).correct==2);
  media.add({5,3,key,a,false});
  media.add({6,3,key,b,false}); // two errors in one session count once
  media.add({7,4,key,a,false});
  assert(replay.begin(media));p=replay.skill(0);
  assert(p.error_sessions==3&&p.recent_error_sessions==2);
  media.add({8,5,key,a,true});
  assert(replay.begin(media)&&replay.skill(0).recent_error_sessions==0);
  media.add({9,5,key,b,false});
  assert(replay.begin(media)&&replay.skill(0).recent_error_sessions==3);
  first.session=0;assert(!skill_encode(first,bytes));
  first.session=1;first.step=evidence_step_limit;assert(!skill_encode(first,bytes));
}
