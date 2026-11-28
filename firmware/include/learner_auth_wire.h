#pragma once
#include "learner_device_auth.h"
#include "learner_frame_session.h"
#include <cstdio>

namespace learner_auth {
constexpr std::size_t max_auth_line=136;
using AuthLine=std::array<char,max_auth_line+1>;

inline bool decode_hex(const char* text,std::size_t len,Bytes& out) {
  out.fill(0);
  if(!text||len!=64) return false;
  auto digit=[](char c)->int {
    if(c>='0'&&c<='9') return c-'0';
    if(c>='a'&&c<='f') return c-'a'+10;
    return -1;
  };
  Bytes value{};
  for(std::size_t i=0;i<32;++i) {
    const int a=digit(text[2*i]),b=digit(text[2*i+1]);
    if(a<0||b<0) return false;
    value[i]=static_cast<uint8_t>((a<<4)|b);
  }
  out=value; return true;
}

inline void encode_hex(const Bytes& value,char (&out)[65]) {
  constexpr char digits[]="0123456789abcdef";
  for(std::size_t i=0;i<32;++i) {
    out[2*i]=digits[value[i]>>4]; out[2*i+1]=digits[value[i]&15];
  }
  out[64]='\0';
}

// Portable complete-frame handler, not a serial transport or write permission.
class DeviceAuthWire {
 public:
  void cancel() { challenge_.cancel(); ack_.fill(0); session_.fill(0); }
  void expire(uint32_t now) {
    challenge_.expire(now);
    if(!challenge_.pending()) {ack_.fill(0);session_.fill(0);}
  }
  bool begin(const char* frame,std::size_t len,Hmac hmac,const Bytes& key,
             const char* expected_id,const Bytes& fresh_nonce,uint32_t now,AuthLine& reply) {
    cancel(); reply.fill(0);
    if(!frame||len<72||len>max_auth_line||!valid_id(expected_id)||
       std::memcmp(frame,"AUTH1,",6)!=0) return false;
    const std::size_t id_len=len-6-1-64;
    if(id_len!=std::strlen(expected_id)||frame[6+id_len]!=','||
       std::memcmp(frame+6,expected_id,id_len)!=0) return false;
    Bytes host{},device_proof{};
    if(!decode_hex(frame+7+id_len,64,host)||
       !challenge_.begin(hmac,key,expected_id,host,fresh_nonce,now,device_proof)||
       !make_proof(hmac,key,Role::Ack,expected_id,host,fresh_nonce,ack_)||
       !derive_session_key(hmac,key,expected_id,host,fresh_nonce,session_)) {
      cancel(); return false;
    }
    char nonce_hex[65]{},proof_hex[65]{};
    encode_hex(fresh_nonce,nonce_hex); encode_hex(device_proof,proof_hex);
    const int n=std::snprintf(reply.data(),reply.size(),"AUTH2,%s,%s",nonce_hex,proof_hex);
    if(n!=135) { cancel(); reply.fill(0); return false; }
    return true;
  }
  bool finish(const char* frame,std::size_t len,uint32_t now,AuthLine& reply,Bytes* authenticated_key=nullptr) {
    reply.fill(0);
    if(authenticated_key) authenticated_key->fill(0);
    Bytes proof{};
    if(!frame||len!=70||std::memcmp(frame,"AUTH3,",6)!=0||
       !decode_hex(frame+6,64,proof)||!challenge_.complete(proof,now)) {
      cancel(); return false;
    }
    char hex[65]{};
    encode_hex(ack_,hex);
    const int n=std::snprintf(reply.data(),reply.size(),"AUTH4,%s",hex);
    if(n!=70) { cancel(); reply.fill(0); return false; }
    if(authenticated_key) *authenticated_key=session_;
    cancel();
    return true;
  }
 private:
  DeviceChallenge challenge_;
  Bytes ack_{},session_{};
};
}
