#pragma once
#include "learner_auth_wire.h"
#include <algorithm>

namespace learner_auth {
using NonceSource=bool (*)(Bytes&);

// Nonblocking transport reference, independent of Arduino and actual USB.
class AuthTransport {
 public:
  explicit AuthTransport(bool handoff=false):handoff_(handoff) {}
  AuthTransport(const AuthTransport&)=delete;
  AuthTransport& operator=(const AuthTransport&)=delete;

  // Stream contract: connected(), available(), read(), availableForWrite(),
  // write(const uint8_t*,size_t). Negative read means no byte; writes may be short.
  template<class Stream>
  void poll(Stream& stream,Hmac hmac,const Bytes& key,const char* id,
            NonceSource nonce,uint32_t now) {
    wire_.expire(now);
    if(!stream.connected()) { reset(); return; }
    if(tx_len_) {
      if(uint32_t(now-tx_start_)>=2000) { reset(); return; }
      const int capacity=stream.availableForWrite();
      if(capacity<=0) return;
      const auto n=std::min({std::size_t(32),std::size_t(capacity),tx_len_-sent_});
      const auto written=stream.write(reinterpret_cast<const uint8_t*>(tx_.data()+sent_),n);
      if(written>n) { reset(); return; }
      sent_+=written;
      if(sent_==tx_len_) { tx_.fill(0);tx_len_=sent_=0; }
      return;
    }
    if(handoff_ready_) {
      if(uint32_t(now-tx_start_)>=2000)reset();
      return; // Do not parse binary bytes as authentication lines.
    }
    if(partial_ && uint32_t(now-rx_start_)>=2000) {
      clear_rx();discard_=true;wire_.cancel();
    }
    for(unsigned i=0;i<32 && stream.available()>0;++i) {
      const int raw=stream.read();
      if(raw<0) break;
      if(raw>255) { clear_rx();discard_=true;wire_.cancel();continue; }
      const auto c=static_cast<unsigned char>(raw);
      if(c=='\n') {
        if(!discard_ && used_) dispatch(hmac,key,id,nonce,now);
        else wire_.cancel();
        clear_rx();discard_=false;
        if(tx_len_) break;
        continue;
      }
      if(discard_) continue;
      if(!partial_) { partial_=true;rx_start_=now; }
      if(cr_ || (c!='\r' && (c<32 || c>126 || used_>=max_auth_line))) {
        clear_rx();discard_=true;wire_.cancel();continue;
      }
      if(c=='\r') {cr_=true;continue;}
      rx_[used_++]=static_cast<char>(c);
    }
  }

  // Release only after the entire AUTH4 response was accepted by the stream.
  bool take_session(Bytes& key,uint32_t now) {
    key.fill(0);
    if(!handoff_ready_||tx_len_)return false;
    if(uint32_t(now-tx_start_)>=2000){reset();return false;}
    key=handoff_key_;handoff_key_.fill(0);handoff_ready_=false;return true;
  }
  // Never reset rate limiting on disconnect.
  void reset() {
    wire_.cancel();clear_rx();discard_=false;
    tx_.fill(0);tx_len_=sent_=0;
    handoff_key_.fill(0);handoff_ready_=false;
  }
 private:
  void clear_rx() {rx_.fill(0);used_=0;partial_=cr_=false;}
  void dispatch(Hmac hmac,const Bytes& key,const char* id,NonceSource nonce,uint32_t now) {
    AuthLine reply{};
    bool ok=false;
    if(used_>=6 && std::memcmp(rx_.data(),"AUTH1,",6)==0) {
      wire_.cancel();
      // Do not spend entropy on malformed or misaddressed challenges.
      if(!valid_id(id)||used_!=71+std::strlen(id)||
         std::memcmp(rx_.data()+6,id,std::strlen(id))!=0||
         rx_[6+std::strlen(id)]!=',') return;
      Bytes host{},fresh{};
      if(!decode_hex(rx_.data()+7+std::strlen(id),64,host)) return;
      if(have_started_ && uint32_t(now-last_start_)<500) return;
      have_started_=true;last_start_=now;
      if(!nonce || !nonce(fresh)) return;
      ok=wire_.begin(rx_.data(),used_,hmac,key,id,fresh,now,reply);
    } else if(used_>=6 && std::memcmp(rx_.data(),"AUTH3,",6)==0) {
      ok=wire_.finish(rx_.data(),used_,now,reply,handoff_?&handoff_key_:nullptr);
      handoff_ready_=handoff_&&ok;
    } else wire_.cancel();
    if(!ok) return;
    const auto n=std::strlen(reply.data());
    if(n==0||n>max_auth_line) {reset();return;}
    std::memcpy(tx_.data(),reply.data(),n);
    tx_[n]='\n';tx_len_=n+1;sent_=0;tx_start_=now;
  }
  const bool handoff_;
  bool handoff_ready_=false;
  Bytes handoff_key_{};
  DeviceAuthWire wire_;
  AuthLine rx_{};
  std::array<char,max_auth_line+1> tx_{};
  std::size_t used_=0,tx_len_=0,sent_=0;
  uint32_t rx_start_=0,tx_start_=0,last_start_=0;
  bool partial_=false,cr_=false,discard_=false,have_started_=false;
};
}
