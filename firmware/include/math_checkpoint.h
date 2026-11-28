#pragma once
#include "math_practice.h"
#include <cstdint>

// Explicit byte format, independent of compiler padding and native endianness.
// Version 1 identifies the fixed synthetic question bank. Not student evidence.
using MathCheckpoint = std::array<std::uint8_t,16>;
inline std::uint32_t checkpoint_crc(const MathCheckpoint& b) {
  std::uint32_t crc=0xffffffffu;
  for(unsigned i=0;i<12;++i) {
    crc^=b[i];
    for(unsigned bit=0;bit<8;++bit)
      crc=(crc>>1)^((crc&1u)?0xedb88320u:0u);
  }
  return ~crc;
}
inline bool math_state_valid(const MathPractice& s) {
  return s.index<=math_questions.size() && s.independent<=s.index &&
    s.attempts>=s.index && s.finished==(s.index==math_questions.size()) &&
    !(s.finished && s.retried) &&
    s.attempts-s.index>=s.index-s.independent+unsigned(s.retried);
}
inline bool math_encode(const MathPractice& s,MathCheckpoint& b) {
  if(!math_state_valid(s)) return false;
  b={};
  b[0]='P'; b[1]='L'; b[2]=1; b[3]=static_cast<std::uint8_t>(s.index);
  b[4]=static_cast<std::uint8_t>(s.independent);
  b[5]=s.retried?1:0; b[6]=s.finished?1:0;
  for(unsigned i=0;i<4;++i) b[8+i]=static_cast<std::uint8_t>(s.attempts>>(8*i));
  auto crc=checkpoint_crc(b);
  for(unsigned i=0;i<4;++i) b[12+i]=static_cast<std::uint8_t>(crc>>(8*i));
  return true;
}
inline bool math_decode(const MathCheckpoint& b,MathPractice& out) {
  if(b[0]!='P'||b[1]!='L'||b[2]!=1||b[5]>1||b[6]>1||b[7]!=0) return false;
  std::uint32_t crc=0, attempts=0;
  for(unsigned i=0;i<4;++i) {
    crc|=std::uint32_t(b[12+i])<<(8*i);
    attempts|=std::uint32_t(b[8+i])<<(8*i);
  }
  if(crc!=checkpoint_crc(b)) return false;
  MathPractice candidate;
  candidate.index=b[3]; candidate.independent=b[4]; candidate.retried=b[5]!=0;
  candidate.finished=b[6]!=0; candidate.attempts=attempts;
  if(!math_state_valid(candidate)) return false;
  out=candidate;
  return true;
}
