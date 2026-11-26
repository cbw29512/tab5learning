#pragma once
#include "learner_device_auth.h"

// Prototype record only. CRC protects against accidental corruption,
// not tampering, disclosure, or replacement by a privileged attacker.
namespace learner_auth {
using CredentialRecord=std::array<uint8_t,105>;

inline uint32_t credential_crc(const CredentialRecord& record) {
  uint32_t crc=0xffffffffu;
  for(std::size_t i=0;i<101;++i) {
    crc^=record[i];
    for(unsigned bit=0;bit<8;++bit)
      crc=(crc>>1)^((crc&1u)?0xedb88320u:0u);
  }
  return ~crc;
}

inline bool credential_encode(const char* id,const Bytes& key,CredentialRecord& out) {
  out.fill(0);
  if(!valid_id(id)||key==Bytes{}) return false;
  const auto size=std::strlen(id);
  std::memcpy(out.data(),"PLC1",4);
  out[4]=static_cast<uint8_t>(size);
  std::memcpy(out.data()+5,id,size);
  std::memcpy(out.data()+69,key.data(),key.size());
  const auto crc=credential_crc(out);
  for(unsigned i=0;i<4;++i) out[101+i]=static_cast<uint8_t>(crc>>(8*i));
  return true;
}

inline bool credential_decode(const CredentialRecord& in,char (&id)[65],Bytes& key) {
  std::memset(id,0,sizeof(id)); key.fill(0);
  if(std::memcmp(in.data(),"PLC1",4)!=0||in[4]<1||in[4]>64) return false;
  uint32_t crc=0;
  for(unsigned i=0;i<4;++i) crc|=uint32_t(in[101+i])<<(8*i);
  if(crc!=credential_crc(in)) return false;
  const auto size=in[4];
  char candidate[65]{};
  std::memcpy(candidate,in.data()+5,size);
  if(!valid_id(candidate)||std::strlen(candidate)!=size) return false;
  for(unsigned i=size;i<64;++i) if(in[5+i]!=0) return false;
  Bytes secret{};
  std::memcpy(secret.data(),in.data()+69,secret.size());
  if(secret==Bytes{}) return false;
  std::memcpy(id,candidate,sizeof(id));key=secret;
  return true;
}

inline bool valid_credential(const CredentialRecord& record) {
  char id[65]{};
  Bytes key{};
  return credential_decode(record,id,key);
}
}
