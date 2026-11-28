#pragma once
#include "learner_frame_session.h"

namespace learner_auth {
constexpr std::size_t protected_overhead=23;
using GcmNonce=std::array<uint8_t,12>;
using GcmTag=std::array<uint8_t,16>;
// Provider must authenticate before returning plaintext success.
using Gcm=bool (*)(bool encrypt,const Bytes&,const GcmNonce&,
                   const uint8_t* aad,std::size_t aad_len,const uint8_t* in,
                   std::size_t len,uint8_t* out,GcmTag& tag);

// Distinct encrypted mode, no fallback to unencrypted frames. A fresh
// handshake key is required for every connection; never restart counters
// using the same key. The caller owns disconnect and expiry polling.
class ProtectedFrameSession {
 public:
  ProtectedFrameSession()=default;
  ProtectedFrameSession(const ProtectedFrameSession&)=delete;
  ProtectedFrameSession& operator=(const ProtectedFrameSession&)=delete;
  ~ProtectedFrameSession(){close();}
  bool begin(Hmac hmac,Gcm gcm,const Bytes& session,bool host,uint32_t now){
    close();
    constexpr char domain[]="TAB5-GCM-V1";
    if(!hmac||!gcm||!hmac(session,reinterpret_cast<const uint8_t*>(domain),sizeof(domain),key_)){
      close();return false;
    }
    gcm_=gcm;tx_=host?0x81:0x82;rx_=host?0x82:0x81;
    started_=now;return true;
  }
  void close(){gcm_=nullptr;key_.fill(0);plain_.fill(0);sent_=received_=0;}
  void expire(uint32_t now){if(gcm_&&uint32_t(now-started_)>=120000)close();}
  bool send(const uint8_t* p,std::size_t len,uint32_t now,SessionFrame& out,std::size_t& used){
    used=0;
    if(!check(now,sent_)||!p||!len||len>session_max_payload){close();return false;}
    out[0]=tx_;put32(out.data()+1,sent_);out[5]=uint8_t(len);out[6]=uint8_t(len>>8);
    GcmTag tag{};
    if(!gcm_(true,key_,nonce(tx_,sent_),out.data(),7,p,len,out.data()+7,tag)){
      close();out.fill(0);return false;
    }
    std::memcpy(out.data()+7+len,tag.data(),tag.size());
    ++sent_;used=len+protected_overhead;return true;
  }
  bool receive(const uint8_t* f,std::size_t len,uint32_t now,const uint8_t*& p,std::size_t& size){
    p=nullptr;size=0;plain_.fill(0);
    if(!check(now,received_)||!f||len<=protected_overhead||
       len>session_max_payload+protected_overhead||f[0]!=rx_||
       get32(f+1)!=received_||std::size_t(f[5])+256u*f[6]!=len-protected_overhead){
      close();return false;
    }
    GcmTag tag{};std::memcpy(tag.data(),f+len-tag.size(),tag.size());
    const auto n=len-protected_overhead;
    if(!gcm_(false,key_,nonce(rx_,received_),f,7,f+7,n,plain_.data(),tag)){
      close();return false;
    }
    ++received_;p=plain_.data();size=n;return true;
  }
 private:
  bool check(uint32_t now,uint32_t seq){expire(now);return gcm_&&seq<session_max_sequence;}
  static void put32(uint8_t* p,uint32_t x){for(unsigned i=0;i<4;++i)p[i]=uint8_t(x>>(8*i));}
  static uint32_t get32(const uint8_t* p){
    uint32_t x=0;for(unsigned i=0;i<4;++i)x|=uint32_t(p[i])<<(8*i);return x;
  }
  static GcmNonce nonce(uint8_t direction,uint32_t seq){
    GcmNonce n{};n[0]=direction;put32(n.data()+1,seq);return n;
  }
  Gcm gcm_=nullptr;Bytes key_{};
  std::array<uint8_t,session_max_payload> plain_{};
  uint32_t started_=0,sent_=0,received_=0;
  uint8_t tx_=0,rx_=0;
};
}
