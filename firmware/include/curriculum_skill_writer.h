#pragma once
#include "curriculum_skill_history.h"
namespace curriculum_preview {
// Exclusive-owner reference contract: append one whole record and sync it.
// No create, truncate, erase, retry or file path here.
class SkillHistoryBackend:public SkillHistoryReader {
 public:
  virtual bool append_synced(unsigned expected_count,const SkillEventBytes& bytes)=0;
};
enum class SkillWriteResult { Saved, AlreadyObserved, Ignored, Unavailable, Full };
class SkillHistoryWriter {
  SkillHistoryBackend& media;
  SkillHistoryReplay replay;
  std::array<std::uint32_t,lesson_count> observed{};
  unsigned session_responses=0;
  std::uint32_t active_session=0;
  bool attempted=false;
  SkillWriteResult fail(const char* reason){
    replay.fail(reason);return SkillWriteResult::Unavailable;
  }
  bool length_is(unsigned count){
    std::uint64_t n=0;
    return media.size(n)&&n==std::uint64_t(count)*SkillEventBytes{}.size();
  }
 public:
  explicit SkillHistoryWriter(SkillHistoryBackend& backend):media(backend){}
  SkillHistoryWriter(const SkillHistoryWriter&)=delete;
  SkillHistoryWriter& operator=(const SkillHistoryWriter&)=delete;
  // One owner per evidence session. A failed/ambiguous append disables
  // that owner; only a fresh owner after controlled reopen may replay.
  bool begin(){
    if(attempted)return false;
    attempted=true;
    if(!replay.begin(media))return false;
    active_session=replay.session+1u;
    return true;
  }
  bool available()const{return replay.available();}
  SkillHistorySample skill(std::size_t index)const{return replay.skill(index);}
  unsigned verified_count()const{return replay.verified_count();}
  SkillWriteResult observe(const Preview& before,const Preview& after){
    if(!available())return SkillWriteResult::Unavailable;
    if(!independent_response(before,after))return SkillWriteResult::Ignored;
    const auto count=replay.records;
    if(!length_is(count))return fail("append-precondition");
    const auto bit=std::uint32_t{1}<<after.step;
    if(observed[after.lesson]&bit)return SkillWriteResult::AlreadyObserved;
    if(count>=SkillHistoryReplay::max_records||session_responses>=replay.seen.size())
      return SkillWriteResult::Full;
    CompletionKey key{};
    if(!completion_key(after.current(),key)||!active_session)return fail("identity");
    const SkillEvent e{count+1,active_session,key,static_cast<unsigned>(after.step),
      after.selected==after.activity().answer};
    SkillEventBytes record{},readback{};
    if(!skill_encode(e,record))return fail("encode");
    // No second append on any ambiguous outcome; bytes may already exist.
    if(!media.append_synced(count,record)||!length_is(count+1)||
       !media.read(count,readback)||record!=readback||!length_is(count+1))
      return fail("append-readback");
    if(!replay.apply(e))return SkillWriteResult::Unavailable;
    replay.records=count+1;
    observed[after.lesson]|=bit;++session_responses;
    return SkillWriteResult::Saved;
  }
};
}
