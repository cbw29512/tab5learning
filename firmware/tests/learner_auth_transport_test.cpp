#include "learner_auth_transport.h"
#include <openssl/hmac.h>
#include <cassert>
#include <string>
#include <deque>
using namespace learner_auth;

bool mac(const Bytes& key,const uint8_t* data,std::size_t n,Bytes& out) {
  unsigned int size=0;
  return HMAC(EVP_sha256(),key.data(),int(key.size()),data,n,out.data(),&size)
      && size==out.size();
}
unsigned nonces=0;
bool fresh(Bytes& out) {out.fill(static_cast<uint8_t>(++nonces));return true;}
std::string hex(const Bytes& value) {char out[65]{};encode_hex(value,out);return out;}
struct Stream {
  bool online=true;
  int capacity=7;
  unsigned reads=0,writes=0;
  std::deque<unsigned char> rx;
  std::string tx;
  bool connected() const {return online;}
  int available() const {return static_cast<int>(rx.size());}
  int read() {++reads;auto c=rx.front();rx.pop_front();return c;}
  int availableForWrite() const {return capacity;}
  std::size_t write(const uint8_t* data,std::size_t n) {
    assert(n<=32 && n<=static_cast<std::size_t>(capacity));
    ++writes;
    n=std::min(n,std::size_t(3));
    tx.append(reinterpret_cast<const char*>(data),n);
    return n;
  }
  void push(const std::string& s) {for(unsigned char c:s)rx.push_back(c);}
};
int main() {
  AuthTransport transport;
  Stream stream;
  Bytes key{},host{},nonce{},proof{},receipt{};
  key.fill(42);host.fill(31);
  const std::string start="AUTH1,tab5-001,"+hex(host)+"\n";
  auto poll=[&](uint32_t now){transport.poll(stream,mac,key,"tab5-001",fresh,now);};
  auto pump=[&](uint32_t now){for(unsigned i=0;i<100;++i)poll(now);};
  stream.push(start);
  poll(0);assert(stream.reads==32 && stream.tx.empty());
  pump(1);assert(nonces==1 && stream.tx.size()==136);
  nonce.fill(1);
  assert(make_proof(mac,key,Role::Host,"tab5-001",host,nonce,proof));
  assert(make_proof(mac,key,Role::Ack,"tab5-001",host,nonce,receipt));
  stream.tx.clear();
  const auto finish="AUTH3,"+hex(proof)+"\r\n";
  stream.push(finish);pump(2);
  assert(stream.tx=="AUTH4,"+hex(receipt)+"\n");
  stream.tx.clear();stream.push(finish);pump(3);assert(stream.tx.empty());
  stream.push(start);pump(100);assert(nonces==1 && stream.tx.empty());
  stream.push("AUTH1,wrong,"+hex(host)+"\n");pump(600);assert(nonces==1);
  stream.push(std::string(137,'x')+start);pump(700);assert(nonces==1);
  stream.push(start.substr(0,20));pump(800);
  stream.push(start.substr(20));pump(2800);assert(nonces==1);
  stream.push(start);pump(3000);assert(nonces==2);
  stream.tx.clear();
  stream.online=false;poll(3001);
  stream.online=true;stream.push(start);pump(3100);assert(nonces==2);
  nonce.fill(2);assert(make_proof(mac,key,Role::Host,"tab5-001",host,nonce,proof));
  stream.push("AUTH3,"+hex(proof)+"\n");pump(3101);assert(stream.tx.empty());
  stream.capacity=0;stream.push(start);pump(4000);assert(nonces==3);
  poll(6000);stream.capacity=7;pump(6001);assert(stream.tx.empty());
  nonce.fill(3);assert(make_proof(mac,key,Role::Host,"tab5-001",host,nonce,proof));
  stream.push("AUTH3,"+hex(proof)+"\n");pump(6002);assert(stream.tx.empty());
  stream.push(start);pump(7000);assert(nonces==4 && stream.tx.size()==136);
}
