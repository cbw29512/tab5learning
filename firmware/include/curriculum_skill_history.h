#pragma once
#include <cstdio>
#include "curriculum_skill_event.h"
namespace curriculum_preview {
// Read-only reference contract. A future qualified owner must exclude writers
// during replay. No opening, creating, writing, repair or automatic truncation.
class SkillHistoryReader {
 public:
  virtual ~SkillHistoryReader()=default;
  virtual bool size(std::uint64_t& bytes)=0;
  virtual bool read(unsigned slot,SkillEventBytes& bytes)=0;
};
struct SkillHistorySample {
  bool known=false;
  unsigned observations=0,correct=0,sessions=0;
  unsigned error_sessions=0,recent_error_sessions=0;
};
class SkillHistoryWriter;
class SkillHistoryReplay {
  friend class SkillHistoryWriter; // Publish a new record only after exact durable readback.
  struct Seen { CompletionKey key{}; unsigned step=0; };
  struct Count {
    unsigned observed=0,correct=0,sessions=0,last_session=0;
    unsigned error_sessions=0,recent_error_sessions=0,prior_error_run=0;
    bool session_error=false;
  };
  std::array<Seen,256> seen{};
  std::array<Count,lesson_count> counts{};
  unsigned seen_count=0,records=0;
  std::uint32_t session=0;
  bool ready=false;
  bool fail(const char* reason){
    ready=false;std::printf("ERR,SKILL_HISTORY,%s\n",reason);return false;
  }
  bool apply(const SkillEvent& e){
    // A new session starts with its first durable response; empty boots do
    // not consume session IDs. Reject backwards/skipped sessions.
    if(e.session!=session){
      if(e.session!=session+1u)return fail("session-order");
      session=e.session;seen_count=0;
    }
    for(unsigned i=0;i<seen_count;++i)
      if(seen[i].key==e.lesson&&seen[i].step==e.step)return fail("duplicate");
    if(seen_count>=seen.size())return fail("session-capacity");
    seen[seen_count++]={e.lesson,e.step};
    for(std::size_t i=0;i<lesson_count;++i){
      CompletionKey key{};
      if(!completion_key(lessons[i],key))return fail("catalog-key");
      if(!(key==e.lesson))continue;
      if(e.step>=lessons[i].count||!independent_choice(lessons[i].activities[e.step]))
        return fail("activity");
      auto& c=counts[i];
      // Track consecutive contributing sessions with at least one error.
      // Sessions that never sampled this skill do not change its streak.
      if(c.last_session!=session){
        ++c.sessions;c.last_session=session;
        c.prior_error_run=c.recent_error_sessions;
        c.recent_error_sessions=0;c.session_error=false;
      }
      ++c.observed;
      if(e.correct)++c.correct;
      else if(!c.session_error){
        c.session_error=true;++c.error_sessions;
        c.recent_error_sessions=c.prior_error_run+1;
      }
      break;
    }
    // Historical or retired revisions are preserved in the source but
    // deliberately excluded from current-skill aggregates.
    return true;
  }
 public:
  static constexpr unsigned max_records=4096;
  bool begin(SkillHistoryReader& reader){
    ready=false;counts={};seen_count=records=session=0;
    std::uint64_t length=0;
    if(!reader.size(length)||length%SkillEventBytes{}.size()||
       length/SkillEventBytes{}.size()>max_records)return fail("length");
    const auto n=static_cast<unsigned>(length/SkillEventBytes{}.size());
    for(unsigned i=0;i<n;++i){
      SkillEventBytes b{};SkillEvent e{};
      if(!reader.read(i,b)||!skill_decode(b,e)||e.sequence!=i+1)
        return fail("record");
      if(!apply(e))return false;
    }
    std::uint64_t after=0;
    if(!reader.size(after)||after!=length)return fail("changed-length");
    records=n;ready=true;return true;
  }
  bool available()const{return ready;}
  unsigned verified_count()const{return ready?records:0;}
  SkillHistorySample skill(std::size_t index)const{
    if(!ready||index>=lesson_count)return {};
    const auto& c=counts[index];return {true,c.observed,c.correct,c.sessions,c.error_sessions,c.recent_error_sessions};
  }
};
}
