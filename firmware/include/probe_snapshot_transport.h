#pragma once
#include "learner_auth_transport.h"
#include "learner_protected_frame.h"
#include "synthetic_math_source.h"

namespace learner_auth {
// Isolated nonblocking binary probe. One fixed public snapshot per connection.
// Queue bound covers only the two synthetic records, not actual learner history.
class ProbeSnapshotTransport final:private SnapshotFrameWriter {
 public:
  ProbeSnapshotTransport():auth_(true),sink_(session_,*this){}
  ProbeSnapshotTransport(const ProbeSnapshotTransport&)=delete;
  ProbeSnapshotTransport& operator=(const ProbeSnapshotTransport&)=delete;
  template<class Stream>
  void poll(Stream& stream,Hmac hmac,Gcm gcm,const Bytes& key,NonceSource nonce,uint32_t now){
    now_=now;
    if(!stream.connected()){
      reset();return;
    }
    if(state_==State::Closed)return;
    if(state_==State::Auth){
      auth_.poll(stream,hmac,key,"probe-only",nonce,now);
      Bytes fresh{};
      if(auth_.take_session(fresh,now)){
        sink_.reset_for_new_connection();
        if(!session_.begin(hmac,gcm,fresh,false,now)){fresh.fill(0);close();return;}
        fresh.fill(0);state_=State::Request;started_=now;
      }
      return;
    }
    if(uint32_t(now-started_)>=10000){close();return;}
    if(state_==State::Request){
      // Exact fixed request size: 17 encrypted bytes plus 23 bytes overhead.
      for(unsigned i=0;i<32&&stream.available()>0;++i){
        const int c=stream.read();
        if(c<0)break;
        if(c>255){close();return;}
        request_[received_++]=uint8_t(c);
        if(received_==7&&(request_[0]!=0x81||request_[5]!=17||request_[6]!=0)){
          close();return;
        }
        if(received_==request_.size()){
          if(sink_.serve_synthetic(source_,request_.data(),received_,true)!=MathSnapshotResult::Complete){
            close();return;
          }
          request_.fill(0);state_=State::Drain;started_=now;
          return;
        }
      }
      return;
    }
    if(state_==State::Drain){
      const int capacity=stream.availableForWrite();
      if(capacity<=0)return;
      const auto n=std::min({std::size_t(32),std::size_t(capacity),queued_-sent_});
      const auto written=stream.write(queue_.data()+sent_,n);
      if(written>n){close();return;}
      sent_+=written;
      if(sent_==queued_)close();
    }
  }
 private:
  enum class State{Auth,Request,Drain,Closed};
  bool write(const uint8_t* p,std::size_t n) noexcept override {
    if(!p||!n||n>queue_.size()-queued_)return false;
    std::memcpy(queue_.data()+queued_,p,n);queued_+=n;return true;
  }
  uint32_t now() const noexcept override{return now_;}
  void close(){
    auth_.reset();sink_.abort();request_.fill(0);queue_.fill(0);
    received_=queued_=sent_=0;state_=State::Closed;
  }
  void reset(){close();state_=State::Auth;}
  AuthTransport auth_;
  ProtectedFrameSession session_;
  SyntheticMathSource source_;
  MathFrameSink<ProtectedFrameSession> sink_;
  std::array<uint8_t,40> request_{};
  std::array<uint8_t,512> queue_{};
  std::size_t received_=0,queued_=0,sent_=0;
  uint32_t now_=0,started_=0;
  State state_=State::Auth;
};
}
