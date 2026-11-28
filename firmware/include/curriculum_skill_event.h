#pragma once
#include "curriculum_completion_codec.h"
#include "curriculum_session_evidence.h"
namespace curriculum_preview {
// SPE1: magic[4], sequence u32, session u32, lesson ID[80], revision u32,
// activity ordinal u16, correctness byte, reserved zero, CRC32. LE integers.
// Stable lesson revision fixes the activity identity; never persist an array index
// alone. Session numbers are ordered, not dates or wall-clock timestamps.
using SkillEventBytes=std::array<std::uint8_t,104>;
struct SkillEvent {
  std::uint32_t sequence=0,session=0;
  CompletionKey lesson{};
  unsigned step=0;
  bool correct=false;
};
inline std::uint32_t skill_crc(const SkillEventBytes& bytes){
  std::uint32_t crc=0xffffffffu;
  for(unsigned i=0;i<100;++i){
    crc^=bytes[i];
    for(unsigned bit=0;bit<8;++bit)crc=(crc>>1)^((crc&1u)?0xedb88320u:0u);
  }
  return ~crc;
}
inline void skill_put(SkillEventBytes& b,unsigned at,std::uint32_t value){
  for(unsigned i=0;i<4;++i)b[at+i]=std::uint8_t(value>>(8*i));
}
inline std::uint32_t skill_get(const SkillEventBytes& b,unsigned at){
  std::uint32_t v=0;
  for(unsigned i=0;i<4;++i)v|=std::uint32_t(b[at+i])<<(8*i);
  return v;
}
inline bool skill_event_valid(const SkillEvent& e){
  return e.sequence&&e.session&&completion_key_valid(e.lesson)&&e.step<evidence_step_limit;
}
inline bool skill_encode(const SkillEvent& e,SkillEventBytes& out){
  if(!skill_event_valid(e))return false;
  SkillEventBytes b{};b[0]='S';b[1]='P';b[2]='E';b[3]=1;
  skill_put(b,4,e.sequence);skill_put(b,8,e.session);
  std::memcpy(b.data()+12,e.lesson.id.data(),80);
  skill_put(b,92,e.lesson.revision);
  b[96]=std::uint8_t(e.step);b[97]=std::uint8_t(e.step>>8);
  b[98]=e.correct?1:0;
  skill_put(b,100,skill_crc(b));out=b;return true;
}
inline bool skill_decode(const SkillEventBytes& b,SkillEvent& out){
  if(b[0]!='S'||b[1]!='P'||b[2]!='E'||b[3]!=1||b[98]>1||b[99]||
     skill_get(b,100)!=skill_crc(b))return false;
  SkillEvent e{};e.sequence=skill_get(b,4);e.session=skill_get(b,8);
  std::memcpy(e.lesson.id.data(),b.data()+12,80);
  e.lesson.revision=skill_get(b,92);e.step=unsigned(b[96])|(unsigned(b[97])<<8);
  e.correct=b[98]!=0;
  if(!skill_event_valid(e))return false;
  out=e;return true;
}
}
