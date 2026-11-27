#include "synthetic_incremental_transfer.h"
#include "openssl_gcm_fixture.h"
#include <openssl/hmac.h>
#include <cassert>
#include <vector>
using namespace learner_auth;
static bool mac(const Bytes& k,const uint8_t* p,std::size_t n,Bytes& out){
  unsigned int length=0;
  return HMAC(EVP_sha256(),k.data(),int(k.size()),p,n,out.data(),&length)&&length==out.size();
}
class Writer final:public SnapshotFrameWriter{
 public:
  std::vector<uint8_t> frame;
  uint32_t time=0;
  bool write(const uint8_t* p,std::size_t n) noexcept override{
    if(!frame.empty())return false;
    try{frame.assign(p,p+n);return true;}catch(...){return false;}
  }
  uint32_t now()const noexcept override{return time;}
};
static bool submit(MathJournal& journal,MathPractice& state){
  static MathScratchpad scratch;static MathAttempt attempt;
  return make_math_attempt(state,0,scratch,attempt)&&journal.append(attempt,state,state);
}
int main(){
  using S=MathExportState;
  FakeNvs::reset();MathJournal journal;MathPractice state;
  assert(journal.begin(state)==JournalLoad::Empty);
  for(unsigned i=0;i<100;++i)assert(submit(journal,state));
  const auto before=FakeNvs::records;
  static ProtectedFrameSession client,device;
  static SessionFrame request;
  Bytes key{};uint8_t message[17]={snapshot_request};std::size_t n=0;
  auto setup=[&](){
    // Fresh deterministic key per test session; never reset live counters.
    ++key[0];
    assert(client.begin(mac,test_gcm,key,true,0)&&device.begin(mac,test_gcm,key,false,0));
    assert(client.send(message,sizeof(message),0,request,n));
  };
  {setup();Writer writer;SyntheticIncrementalTransfer transfer(journal,device,writer);
  assert(transfer.start(request.data(),n,true));
  assert(transfer.poll(false)==S::Manifest&&writer.frame.empty());
  assert(!submit(journal,state));
  for(unsigned i=0;i<102;++i){
    const auto result=transfer.poll(true);
    assert(!writer.frame.empty());
    const uint8_t* p=nullptr;std::size_t len=0;
    assert(client.receive(writer.frame.data(),writer.frame.size(),writer.time,p,len));
    assert(p[0]==(i==0?snapshot_begin:i==101?snapshot_end:snapshot_record));
    if(i>0&&i<101){
      char name[12];math_journal_key(i,name);
      const auto& raw=before.at("pljournal").at(name);
      assert(len==21+raw.size()&&!std::memcmp(p+21,raw.data(),raw.size()));
    }
    if(i<101){
      auto held=writer.frame;
      assert(transfer.poll(false)==result&&writer.frame==held);
    }else assert(result==S::Complete);
    writer.frame.clear();
  }
  assert(FakeNvs::records==before&&submit(journal,state));}
  {setup();Writer deny;SyntheticIncrementalTransfer denied(journal,device,deny);
  assert(!denied.start(request.data(),n,false)&&deny.frame.empty());}
  {setup();Writer paused;SyntheticIncrementalTransfer timeout(journal,device,paused);
  assert(timeout.start(request.data(),n,true)&&timeout.poll(true)==S::Records);
  paused.time=110000;assert(timeout.poll(false)==S::Failed);
  assert(submit(journal,state));}
  {setup();Writer interrupted;SyntheticIncrementalTransfer cancelled(journal,device,interrupted);
  assert(cancelled.start(request.data(),n,true)&&cancelled.poll(true)==S::Records);
  cancelled.cancel();assert(cancelled.poll(true)==S::Failed&&submit(journal,state));}
  {setup();Writer corrupted;SyntheticIncrementalTransfer invalid(journal,device,corrupted);
  request[7]^=1;assert(!invalid.start(request.data(),n,true)&&corrupted.frame.empty());}
}
