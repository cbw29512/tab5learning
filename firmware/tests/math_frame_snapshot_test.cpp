#include "math_frame_snapshot.h"
#include <openssl/hmac.h>
#include <cassert>
#include <fstream>
#include <vector>
using namespace learner_auth;
bool mac(const Bytes& k,const uint8_t* p,std::size_t n,Bytes& out){
  unsigned int length=0;
  return HMAC(EVP_sha256(),k.data(),int(k.size()),p,n,out.data(),&length)&&length==out.size();
}
class Writer final:public SnapshotFrameWriter{
 public:
  std::vector<std::vector<uint8_t>> frames;
  unsigned fail_after=999;
  bool write(const uint8_t* p,std::size_t n) noexcept override {
    try{if(frames.size()>=fail_after)return false;frames.emplace_back(p,p+n);return true;}
    catch(...){return false;}
  }
  uint32_t now() const noexcept override{return 1;}
};
int main(int argc,char** argv){
  FakeNvs::reset();MathJournal journal;MathPractice state;
  assert(journal.begin(state)==JournalLoad::Empty);
  static MathScratchpad scratch;static MathAttempt attempt;
  assert(scratch_touch(scratch,656,184,true));
  assert(make_math_attempt(state,0,scratch,attempt)&&journal.append(attempt,state,state));
  scratch_clear(scratch);
  assert(make_math_attempt(state,1,scratch,attempt)&&journal.append(attempt,state,state));
  const auto before=FakeNvs::records;
  Bytes key{};for(unsigned i=0;i<32;++i)key[i]=i;
  static FrameSession client,device;static SessionFrame request;
  std::size_t n=0;uint8_t message[17]={snapshot_request};
  for(unsigned i=0;i<16;++i)message[1+i]=i;
  auto setup=[&](){
    client.begin(mac,key,true,0);device.begin(mac,key,false,0);
    assert(client.send(message,sizeof(message),0,request,n));
  };
  setup();Writer writer;MathFrameSink sink(device,writer);
  assert(sink.serve_synthetic(journal,request.data(),n,true)==MathSnapshotResult::Complete);
  assert(writer.frames.size()==4&&FakeNvs::records==before);
  if(argc==2){
    std::ofstream out(argv[1],std::ios::binary);
    for(const auto& f:writer.frames){
      const char size[2]={static_cast<char>(f.size()&255),static_cast<char>(f.size()>>8)};
      out.write(size,2);out.write(reinterpret_cast<const char*>(f.data()),f.size());
    }
    out.close();assert(out);
  }
  const uint8_t* p=nullptr;std::size_t len=0;
  for(unsigned i=0;i<4;++i){
    const auto& f=writer.frames[i];
    assert(client.receive(f.data(),f.size(),1,p,len));
    assert(p[0]==(i==0?snapshot_begin:i==3?snapshot_end:snapshot_record));
    if(i==1||i==2){
      const auto& raw=before.at("pljournal").at(i==1?"a001":"a002");
      assert(len==21+raw.size()&&!std::memcmp(p+21,raw.data(),raw.size()));
    }
  }
  setup();Writer interrupted;interrupted.fail_after=2;MathFrameSink failed(device,interrupted);
  assert(failed.serve_synthetic(journal,request.data(),n,true)==MathSnapshotResult::SinkFailure);
  assert(interrupted.frames.size()==2&&FakeNvs::records==before);
  setup();Writer denied;MathFrameSink disabled(device,denied);
  assert(disabled.serve_synthetic(journal,request.data(),n,false)==MathSnapshotResult::Unavailable);
  assert(denied.frames.empty());
  setup();request[7]^=1;Writer corrupt;MathFrameSink bad(device,corrupt);
  assert(bad.serve_synthetic(journal,request.data(),n,true)==MathSnapshotResult::Unavailable);
  assert(corrupt.frames.empty());
}
