#pragma once
#include "math_journal.h"
#include "learner_frame_session.h"

namespace learner_auth {
// Synthetic read-only snapshot messages inside an authenticated frame.
using SnapshotId=std::array<uint8_t,16>;
constexpr uint8_t snapshot_request=0x31,snapshot_begin=0x32,
                  snapshot_record=0x33,snapshot_end=0x34;
inline void snapshot_u32(uint8_t* p,uint32_t x){
  for(unsigned i=0;i<4;++i)p[i]=uint8_t(x>>(8*i));
}
class SnapshotFrameWriter {
 public:
  virtual ~SnapshotFrameWriter()=default;
  // Caller must accept the entire frame or fail. No blocking hardware adapter
  // or production data access is provided by this synchronous test reference.
  virtual bool write(const uint8_t* p,std::size_t n) noexcept=0;
  virtual uint32_t now() const noexcept=0;
};
class MathFrameSink final : public MathSnapshotSink {
 public:
  MathFrameSink(FrameSession& session,SnapshotFrameWriter& writer):
    session_(session),writer_(writer){}
  // Exactly one operation per sink. No export until a valid request is
  // received. A real dispatcher must also authenticate teacher permissions
  // and provide encrypted transport before calling an export.
  MathSnapshotResult serve_synthetic(MathJournal& journal,const uint8_t* frame,
                                     std::size_t n,bool synthetic_only) {
    if(used_||!synthetic_only){abort();return MathSnapshotResult::Unavailable;}
    used_=true;
    const uint8_t* p=nullptr;std::size_t size=0;
    if(!session_.receive(frame,n,writer_.now(),p,size)||size!=17||p[0]!=snapshot_request){
      abort();return MathSnapshotResult::Unavailable;
    }
    std::memcpy(id_.data(),p+1,id_.size());
    requested_=true;
    const auto result=journal.snapshot(*this);
    if(result!=MathSnapshotResult::Complete)abort();
    // One snapshot per authenticated connection; reconnect for a retry.
    session_.close();requested_=false;
    return result;
  }
  bool begin(const MathSnapshot& m) noexcept override {
    if(!requested_||begun_||m.count>math_journal_max_records)return false;
    count_=m.count;prefix(snapshot_begin);
    std::memcpy(payload_.data()+17,m.base.data(),16);
    std::memcpy(payload_.data()+33,m.final.data(),16);
    snapshot_u32(payload_.data()+49,count_);
    begun_=send(53);return begun_;
  }
  bool record(uint32_t slot,const AttemptBytes& bytes,std::size_t n) noexcept override {
    if(!begun_||committed_||seen_>=count_||slot!=seen_+1||n<28||n>bytes.size())return false;
    prefix(snapshot_record);snapshot_u32(payload_.data()+17,slot);
    std::memcpy(payload_.data()+21,bytes.data(),n);
    if(!send(21+n))return false;
    ++seen_;return true;
  }
  bool commit() noexcept override {
    if(!begun_||committed_||seen_!=count_)return false;
    prefix(snapshot_end);snapshot_u32(payload_.data()+17,count_);
    committed_=send(21);return committed_;
  }
  void abort() noexcept override {
    requested_=begun_=committed_=false;seen_=count_=0;
    payload_.fill(0);frame_.fill(0);session_.close();
  }
 private:
  void prefix(uint8_t kind){payload_[0]=kind;std::memcpy(payload_.data()+1,id_.data(),16);}
  bool send(std::size_t n) noexcept {
    std::size_t length=0;
    const bool ok=session_.send(payload_.data(),n,writer_.now(),frame_,length)&&
                  writer_.write(frame_.data(),length);
    payload_.fill(0);frame_.fill(0);
    if(!ok)session_.close();
    return ok;
  }
  FrameSession& session_;SnapshotFrameWriter& writer_;
  SnapshotId id_{};
  std::array<uint8_t,21+AttemptBytes{}.size()> payload_{};
  SessionFrame frame_{};
  uint32_t count_=0,seen_=0;
  bool used_=false,requested_=false,begun_=false,committed_=false;
};
}
