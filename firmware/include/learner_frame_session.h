#pragma once
#include "learner_device_auth.h"

namespace learner_auth {
constexpr std::size_t session_max_payload=4096;
constexpr std::size_t session_overhead=39;
constexpr uint32_t session_max_sequence=65536;
using SessionFrame=std::array<uint8_t,session_max_payload+session_overhead>;

inline bool derive_session_key(Hmac hmac,const Bytes& key,const char* id,
                               const Bytes& host,const Bytes& device,Bytes& out) {
  out.fill(0);
  if(!hmac||!valid_id(id)) return false;
  constexpr char domain[]="TAB5-SESSION-V1";
  std::array<uint8_t,sizeof(domain)+65+64> data{};
  std::size_t pos=0;
  auto add=[&](const void* ptr,std::size_t n){std::memcpy(data.data()+pos,ptr,n);pos+=n;};
  add(domain,sizeof(domain));add(id,std::strlen(id)+1);
  add(host.data(),32);add(device.data(),32);
  if(!hmac(key,data.data(),pos,out)) {out.fill(0);return false;}
  return true;
}

// Portable frame codec. Caller supplies complete frames; no encryption,
// physical connection, operation dispatch or persistent write authorization.
class FrameSession {
 public:
  FrameSession()=default;
  FrameSession(const FrameSession&)=delete;
  FrameSession& operator=(const FrameSession&)=delete;
  ~FrameSession(){close();}
  void begin(Hmac hmac,const Bytes& key,bool host,uint32_t now) {
    close(); hmac_=hmac;key_=key;tx_=host?1:2;rx_=host?2:1;
    started_=now;active_=hmac!=nullptr;
  }
  void close(){active_=false;key_.fill(0);sent_=received_=0;hmac_=nullptr;}
  void expire(uint32_t now){if(active_&&uint32_t(now-started_)>=120000)close();}
  bool send(const uint8_t* payload,std::size_t len,uint32_t now,SessionFrame& out,std::size_t& used) {
    used=0;
    if(!check(now,sent_)||!payload||!len||len>session_max_payload){close();return false;}
    // Payload must not alias output.
    out[0]=tx_;put32(out.data()+1,sent_);out[5]=uint8_t(len);out[6]=uint8_t(len>>8);
    std::memcpy(out.data()+7,payload,len);
    Bytes tag{};
    if(!mac(out.data(),7+len,tag)){close();return false;}
    std::memcpy(out.data()+7+len,tag.data(),tag.size());
    ++sent_;used=len+session_overhead;return true;
  }
  bool receive(const uint8_t* frame,std::size_t len,uint32_t now,
               const uint8_t*& payload,std::size_t& size) {
    payload=nullptr;size=0;
    if(!check(now,received_)||!frame||len<=session_overhead||
       len>session_overhead+session_max_payload||frame[0]!=rx_||
       get32(frame+1)!=received_||std::size_t(frame[5])+256u*frame[6]!=len-session_overhead){
      close();return false;
    }
    Bytes tag{},supplied{};
    std::memcpy(supplied.data(),frame+len-32,32);
    if(!mac(frame,len-32,tag)||!equal_proof(tag,supplied)){close();return false;}
    ++received_;payload=frame+7;size=len-session_overhead;return true;
  }
 private:
  static void put32(uint8_t* p,uint32_t x){for(unsigned i=0;i<4;++i)p[i]=uint8_t(x>>(8*i));}
  static uint32_t get32(const uint8_t* p){
    uint32_t x=0;for(unsigned i=0;i<4;++i)x|=uint32_t(p[i])<<(8*i);return x;
  }
  bool check(uint32_t now,uint32_t seq){expire(now);return active_&&seq<session_max_sequence;}
  bool mac(const uint8_t* p,std::size_t n,Bytes& out){
    constexpr char domain[]="TAB5-FRAME-V1";
    std::memcpy(work_.data(),domain,sizeof(domain));
    std::memcpy(work_.data()+sizeof(domain),p,n);
    const bool ok=hmac_(key_,work_.data(),sizeof(domain)+n,out);
    // Member workspace avoids large stack allocation on learner loop.
    work_.fill(0);return ok;
  }
  Hmac hmac_=nullptr;Bytes key_{};
  std::array<uint8_t,sizeof("TAB5-FRAME-V1")+7+session_max_payload> work_{};
  uint32_t started_=0,sent_=0,received_=0;
  uint8_t tx_=0,rx_=0;bool active_=false;
};
}
