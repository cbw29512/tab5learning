#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include "curriculum_preview.h"

namespace curriculum_preview {
// CPR1: magic[4], lesson ID[80] zero padded, revision u32 LE,
// activity index u16 LE, finished byte, reserved zero, CRC32 LE.
// One last-lesson location only. Never an answer, grade or mastery record.
using ResumeBytes=std::array<std::uint8_t,96>;
inline std::uint32_t resume_crc(const ResumeBytes& b){
  std::uint32_t crc=0xffffffffu;
  for(std::size_t i=0;i<92;++i){
    crc^=b[i];
    for(unsigned bit=0;bit<8;++bit)
      crc=(crc>>1)^((crc&1u)?0xedb88320u:0u);
  }
  return ~crc;
}
inline void resume_put32(ResumeBytes& b,unsigned at,std::uint32_t value){
  for(unsigned i=0;i<4;++i)b[at+i]=std::uint8_t(value>>(8*i));
}
inline std::uint32_t resume_get32(const ResumeBytes& b,unsigned at){
  std::uint32_t value=0;
  for(unsigned i=0;i<4;++i)value|=std::uint32_t(b[at+i])<<(8*i);
  return value;
}
inline bool resume_encode(const Preview& p,ResumeBytes& b){
  if(p.screen!=Screen::Activity&&p.screen!=Screen::Feedback&&p.screen!=Screen::Finished)return false;
  if(p.lesson>=lesson_count)return false;
  const auto& l=p.current();
  const auto n=std::strlen(l.id);
  if(n==0||n>80||!l.revision||p.step>=l.count||p.step>65535||
     (p.screen==Screen::Finished&&p.step+1!=l.count))return false;
  b={};b[0]='C';b[1]='P';b[2]='R';b[3]=1;
  std::memcpy(b.data()+4,l.id,n);
  resume_put32(b,84,l.revision);
  b[88]=std::uint8_t(p.step);b[89]=std::uint8_t(p.step>>8);
  b[90]=p.screen==Screen::Finished?1:0;
  resume_put32(b,92,resume_crc(b));
  return true;
}
inline bool resume_decode(const ResumeBytes& b,Preview& out){
  if(b[0]!='C'||b[1]!='P'||b[2]!='R'||b[3]!=1||b[90]>1||b[91]||
     resume_get32(b,92)!=resume_crc(b))return false;
  const auto step=unsigned(b[88])|(unsigned(b[89])<<8);
  for(std::size_t i=0;i<lesson_count;++i){
    const auto& l=lessons[i];const auto n=std::strlen(l.id);
    if(n==0||n>80||std::memcmp(b.data()+4,l.id,n)!=0)continue;
    bool padded=true;
    for(std::size_t j=n;j<80;++j)if(b[4+j])padded=false;
    if(!padded)continue;
    if(resume_get32(b,84)!=l.revision||step>=l.count||
       (b[90]&&step+1!=l.count))return false;
    Preview candidate{};
    candidate.subject=l.subject;candidate.lesson=i;candidate.step=step;
    unsigned preceding=0;
    for(std::size_t j=0;j<i;++j)if(lessons[j].subject==l.subject)++preceding;
    candidate.page=preceding/lessons_per_page;
    // Feedback is intentionally not persisted: repeat that activity on resume.
    candidate.screen=b[90]?Screen::Finished:Screen::Activity;
    out=candidate;return true;
  }
  return false;
}
}
