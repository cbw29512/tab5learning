#pragma once
#include "math_continuous.h"
#include "math_scratchpad.h"
#include <cstdio>
#include <cstddef>

namespace math_continuous {
// PMA1: magic[4], bank revision u32 LE, attempt sequence u32,
// pre-answer ordinal u32, choice/result bytes, ink count u16,
// post-answer solved/independent u32 each, retried byte, 3 reserved
// zero bytes, normalized ink points (5 bytes each), CRC32 LE.
// Exact record length: 36+5*n. Separate from legacy PA1/PLMB.
using Record=std::array<std::uint8_t,36+5*kMaxInkPoints>;
inline void put(Record& r,unsigned at,std::uint32_t v){
  for(unsigned i=0;i<4;++i)r[at+i]=std::uint8_t(v>>(8*i));
}
inline std::uint32_t get(const Record& r,unsigned at){
  std::uint32_t v=0;
  for(unsigned i=0;i<4;++i)v|=std::uint32_t(r[at+i])<<(8*i);
  return v;
}
inline std::uint32_t crc(const Record& r,std::size_t n){
  std::uint32_t c=0xffffffffu;
  for(std::size_t i=0;i<n;++i){
    c^=r[i];
    for(unsigned b=0;b<8;++b)c=(c>>1)^((c&1u)?0xedb88320u:0u);
  }
  return ~c;
}
inline bool encode(const State& before,unsigned choice,const MathScratchpad& scratch,
                   Record& r,std::size_t& length){
  length=0;
  auto after=before;const auto result=answer(after,choice);
  const auto& ink=scratch.ink;
  if(result==Result::Rejected||ink.overflow||ink.count>kMaxInkPoints){
    std::printf("ERR,MATH_CONTINUOUS_INPUT\n");return false;
  }
  for(std::size_t i=0;i<ink.count;++i){
    const auto& p=ink.points[i];
    if(!scratch_inside(p.x,p.y)||(!i&&!p.stroke_start))return false;
  }
  r.fill(0);r[0]='P';r[1]='M';r[2]='A';r[3]=1;
  put(r,4,bank_revision);put(r,8,after.attempts);put(r,12,before.solved);
  r[16]=std::uint8_t(choice);r[17]=std::uint8_t(result);
  r[18]=std::uint8_t(ink.count);r[19]=std::uint8_t(ink.count>>8);
  put(r,20,after.solved);put(r,24,after.independent);r[28]=after.retried;
  for(std::size_t i=0;i<ink.count;++i){
    const auto& p=ink.points[i];const auto j=32+5*i;
    const unsigned x=p.x-math_ink_card.x,y=p.y-math_ink_card.y;
    r[j]=std::uint8_t(x);r[j+1]=std::uint8_t(x>>8);
    r[j+2]=std::uint8_t(y);r[j+3]=std::uint8_t(y>>8);r[j+4]=p.stroke_start;
  }
  length=36+5*ink.count;put(r,length-4,crc(r,length-4));return true;
}
inline bool replay(const Record& r,std::size_t n,State& state){
  if(n<36||n>r.size()||r[0]!='P'||r[1]!='M'||r[2]!='A'||r[3]!=1||
     get(r,4)!=bank_revision||get(r,12)!=state.solved||r[28]>1||
     r[29]||r[30]||r[31]||get(r,n-4)!=crc(r,n-4))return false;
  const unsigned points=r[18]+256u*r[19];
  if(points>kMaxInkPoints||n!=36+5*points)return false;
  for(unsigned i=0;i<points;++i){
    const unsigned j=32+5*i;
    if(r[j]+256u*r[j+1]>=unsigned(math_ink_card.w)||
       r[j+2]+256u*r[j+3]>=unsigned(math_ink_card.h)||
       r[j+4]>1||(!i&&!r[j+4]))return false;
  }
  auto next=state;const auto result=answer(next,r[16]);
  if(result==Result::Rejected||r[17]!=std::uint8_t(result)||
     get(r,8)!=next.attempts||get(r,20)!=next.solved||
     get(r,24)!=next.independent||r[28]!=next.retried)return false;
  state=next;return true;
}
}
