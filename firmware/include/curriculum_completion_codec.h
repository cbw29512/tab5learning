#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include "curriculum_preview.h"

namespace curriculum_preview {
// CPL1: magic/version[4], sequence u32 LE, lesson ID[80] zero padded,
// lesson revision u32 LE, CRC32 LE. Exactly one first-completion event
// per immutable lesson revision. No answers, scores, student ID or mastery.
using CompletionBytes=std::array<std::uint8_t,96>;
struct CompletionKey {
  std::array<char,80> id{};
  std::uint32_t revision=0;
  bool operator==(const CompletionKey& other)const{
    return id==other.id&&revision==other.revision;
  }
};
struct CompletionEvent { std::uint32_t sequence=0; CompletionKey key{}; };
inline bool completion_key_valid(const CompletionKey& k){
  if(!k.revision||k.id[0]<'0'||
     !((k.id[0]>='0'&&k.id[0]<='9')||(k.id[0]>='a'&&k.id[0]<='z')))return false;
  bool ended=false;
  for(const char c:k.id){
    if(!c){ended=true;continue;}
    if(ended||!((c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='-'||c=='_'))return false;
  }
  return true;
}
inline bool completion_key(const Lesson& lesson,CompletionKey& out){
  if(!lesson.id)return false;
  const auto length=std::strlen(lesson.id);
  if(!length||length>80)return false;
  CompletionKey key{};
  std::memcpy(key.id.data(),lesson.id,length);
  key.revision=lesson.revision;
  if(!completion_key_valid(key))return false;
  out=key;return true;
}
inline std::uint32_t completion_crc(const CompletionBytes& bytes){
  std::uint32_t crc=0xffffffffu;
  for(unsigned i=0;i<92;++i){
    crc^=bytes[i];
    for(unsigned bit=0;bit<8;++bit)
      crc=(crc>>1)^((crc&1u)?0xedb88320u:0u);
  }
  return ~crc;
}
inline void completion_put(CompletionBytes& b,unsigned at,std::uint32_t v){
  for(unsigned i=0;i<4;++i)b[at+i]=std::uint8_t(v>>(8*i));
}
inline std::uint32_t completion_get(const CompletionBytes& b,unsigned at){
  std::uint32_t v=0;
  for(unsigned i=0;i<4;++i)v|=std::uint32_t(b[at+i])<<(8*i);
  return v;
}
inline bool completion_encode(const CompletionEvent& e,CompletionBytes& out){
  if(!e.sequence||!completion_key_valid(e.key))return false;
  CompletionBytes b{};
  b[0]='C';b[1]='P';b[2]='L';b[3]=1;
  completion_put(b,4,e.sequence);
  std::memcpy(b.data()+8,e.key.id.data(),80);
  completion_put(b,88,e.key.revision);
  completion_put(b,92,completion_crc(b));out=b;return true;
}
inline bool completion_decode(const CompletionBytes& b,CompletionEvent& out){
  if(b[0]!='C'||b[1]!='P'||b[2]!='L'||b[3]!=1||
     completion_get(b,92)!=completion_crc(b))return false;
  CompletionEvent e{};e.sequence=completion_get(b,4);
  std::memcpy(e.key.id.data(),b.data()+8,80);
  e.key.revision=completion_get(b,88);
  if(!e.sequence||!completion_key_valid(e.key))return false;
  out=e;return true;
}
}
