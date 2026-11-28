#include "probe_snapshot_transport.h"
#include "openssl_gcm_fixture.h"
#include <openssl/hmac.h>
#include <cassert>
#include <deque>
#include <string>
#include <vector>
using namespace learner_auth;
bool mac(const Bytes& key,const uint8_t* p,std::size_t n,Bytes& out){
  unsigned int size=0;
  return HMAC(EVP_sha256(),key.data(),int(key.size()),p,n,out.data(),&size)&&size==out.size();
}
unsigned nonces=0;
bool fresh(Bytes& out){out.fill(uint8_t(++nonces));return true;}
std::string hex(const Bytes& b){char h[65]{};encode_hex(b,h);return h;}
struct Stream {
  bool online=true;
  int capacity=7;
  std::deque<uint8_t> rx;
  std::vector<uint8_t> tx;
  bool connected()const{return online;}
  int available()const{return int(rx.size());}
  int read(){auto c=rx.front();rx.pop_front();return c;}
  int availableForWrite()const{return capacity;}
  std::size_t write(const uint8_t* p,std::size_t n){
    assert(n<=32&&n<=std::size_t(capacity));n=std::min(n,std::size_t(3));
    tx.insert(tx.end(),p,p+n);return n;
  }
  void push(const std::string& s){rx.insert(rx.end(),s.begin(),s.end());}
};
int main(){
  static ProbeSnapshotTransport probe;
  static ProtectedFrameSession host_session;
  static SessionFrame request;
  Stream stream;Bytes key{},host{},nonce{},hp{},ack{},derived{};
  key.fill(42);host.fill(31);
  auto pump=[&](uint32_t now){for(unsigned i=0;i<400;++i)probe.poll(stream,mac,test_gcm,key,fresh,now);};
  auto handshake=[&](uint32_t start){
    stream.push("AUTH1,probe-only,"+hex(host)+"\n");pump(start);
    assert(stream.tx.size()==136);nonce.fill(uint8_t(nonces));
    assert(make_proof(mac,key,Role::Host,"probe-only",host,nonce,hp));
    assert(make_proof(mac,key,Role::Ack,"probe-only",host,nonce,ack));
    assert(derive_session_key(mac,key,"probe-only",host,nonce,derived));
    stream.tx.clear();stream.push("AUTH3,"+hex(hp)+"\n");pump(start+1);
    assert(std::string(stream.tx.begin(),stream.tx.end())=="AUTH4,"+hex(ack)+"\n");
    stream.tx.clear();
    assert(host_session.begin(mac,test_gcm,derived,true,start+1));
  };
  auto send=[&](uint32_t now,bool corrupt){
    uint8_t payload[17]={snapshot_request};std::size_t size=0;
    assert(host_session.send(payload,sizeof(payload),now,request,size)&&size==40);
    if(corrupt)request[7]^=1;
    stream.rx.insert(stream.rx.end(),request.begin(),request.begin()+size);pump(now);
  };
  handshake(1);send(3,false);
  std::size_t pos=0;unsigned frames=0;const uint8_t* p=nullptr;std::size_t n=0;
  while(pos<stream.tx.size()){
    assert(stream.tx.size()-pos>=23);
    const auto size=std::size_t(stream.tx[pos+5])+256u*stream.tx[pos+6]+23;
    assert(size<=stream.tx.size()-pos&&host_session.receive(stream.tx.data()+pos,size,4,p,n));
    assert(p[0]==(frames==0?snapshot_begin:frames==3?snapshot_end:snapshot_record));
    pos+=size;++frames;
  }
  assert(frames==4);stream.tx.clear();pump(5);assert(stream.tx.empty());
  stream.online=false;pump(10);stream.online=true;
  handshake(600);send(602,true);assert(stream.tx.empty());
  stream.online=false;pump(610);stream.online=true;
  handshake(1200);pump(11201);send(11202,false);assert(stream.tx.empty());
}
