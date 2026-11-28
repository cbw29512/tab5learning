#pragma once
#include "math_attempt.h"
#include <array>

// V1 synthetic journal format: magic/version, question, choice, result,
// point count, post-answer checkpoint, 5 bytes per point, CRC32.
// The checkpoint contains the sequence. Canvas coordinates are local pixels.
using AttemptBytes=std::array<std::uint8_t,28+5*kMaxInkPoints>;
inline std::uint32_t attempt_crc(const AttemptBytes& b,std::size_t n) {
  std::uint32_t crc=0xffffffffu;
  for(std::size_t i=0;i<n;++i) {
    crc^=b[i];
    for(unsigned j=0;j<8;++j)
      crc=(crc>>1)^((crc&1u)?0xedb88320u:0u);
  }
  return ~crc;
}
inline bool attempt_encode(const MathAttempt& a,AttemptBytes& b,std::size_t& length) {
  MathPractice after;
  if(!math_decode(a.after,after)||a.sequence!=after.attempts||!a.sequence||
      a.question>=math_questions.size()||a.choice>=4||
      a.result==MathResult::Ignored||a.result>MathResult::Finished||
      a.ink.overflow||a.ink.count>kMaxInkPoints) return false;
  b.fill(0); b[0]='P';b[1]='A';b[2]=1;b[3]=a.question;b[4]=a.choice;
  b[5]=static_cast<std::uint8_t>(a.result);
  b[6]=a.ink.count&255u;b[7]=a.ink.count>>8;
  for(unsigned i=0;i<16;++i) b[8+i]=a.after[i];
  for(std::size_t i=0;i<a.ink.count;++i) {
    const auto& p=a.ink.points[i];
    if(p.x>=592||p.y>=244||(!i&&!p.stroke_start)) return false;
    const auto j=24+5*i;
    b[j]=p.x&255u;b[j+1]=p.x>>8;b[j+2]=p.y&255u;b[j+3]=p.y>>8;b[j+4]=p.stroke_start;
  }
  const auto n=24+5*a.ink.count;
  const auto crc=attempt_crc(b,n);
  for(unsigned i=0;i<4;++i) b[n+i]=crc>>(8*i);
  length=n+4;
  return true;
}
// Replay against the preceding state, rather than trusting a supplied result.
inline bool attempt_replay(const AttemptBytes& b,std::size_t length,MathPractice& state) {
  if(length<28||length>b.size()||b[0]!='P'||b[1]!='A'||b[2]!=1||
     b[3]!=state.index||b[4]>=4||state.attempts==UINT32_MAX) return false;
  const std::size_t count=b[6]+256u*b[7];
  if(count>kMaxInkPoints||length!=28+5*count) return false;
  std::uint32_t crc=0;
  for(unsigned i=0;i<4;++i) crc|=std::uint32_t(b[length-4+i])<<(8*i);
  if(crc!=attempt_crc(b,length-4)) return false;
  for(std::size_t i=0;i<count;++i) {
    const auto j=24+5*i;
    if(b[j]+256u*b[j+1]>=592||b[j+2]+256u*b[j+3]>=244||
       b[j+4]>1||(!i&&!b[j+4])) return false;
  }
  auto next=state;
  const auto result=math_answer(next,b[4]);
  MathCheckpoint expected{};
  if(result==MathResult::Ignored||b[5]!=static_cast<unsigned>(result)||
     !math_encode(next,expected)) return false;
  for(unsigned i=0;i<16;++i) if(b[8+i]!=expected[i]) return false;
  state=next;
  return true;
}
