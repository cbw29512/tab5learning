#include "learner_auth_wire.h"
#include <openssl/hmac.h>
#include <cassert>
#include <string>

using namespace learner_auth;
bool openssl_hmac(const Bytes& key,const uint8_t* data,std::size_t size,Bytes& out) {
  unsigned int n=0;
  return HMAC(EVP_sha256(),key.data(),int(key.size()),data,size,out.data(),&n)
      && n==out.size();
}
std::string hex(const Bytes& value) {
  char text[65]{};
  encode_hex(value,text);
  return text;
}
int main() {
  Bytes key{},host{},nonce{},hp{},ack{};
  for(unsigned i=0;i<32;++i) {key[i]=i;host[i]=i+32;nonce[i]=i+64;}
  const std::string start="AUTH1,tab5-001,"+hex(host);
  assert(make_proof(openssl_hmac,key,Role::Host,"tab5-001",host,nonce,hp));
  assert(make_proof(openssl_hmac,key,Role::Ack,"tab5-001",host,nonce,ack));
  const std::string finish="AUTH3,"+hex(hp);
  DeviceAuthWire wire;
  AuthLine reply{};
  auto begin=[&](uint32_t now){return wire.begin(start.data(),start.size(),openssl_hmac,key,"tab5-001",nonce,now,reply);};
  auto complete=[&](uint32_t now){return wire.finish(finish.data(),finish.size(),now,reply);};
  assert(begin(0));
  assert(std::string(reply.data())=="AUTH2,"+hex(nonce)+",67456b1388d302d045b1dbac3a5bab93908ad2d5c83baa8dd9ac15d8e9c7ef6e");
  assert(complete(1));
  assert(std::string(reply.data())=="AUTH4,"+hex(ack));
  assert(!complete(2) && reply[0]==0);
  assert(begin(0));
  assert(!complete(10001));
  assert(begin(0));
  assert(!wire.finish("AUTH3,x",7,1,reply));
  assert(!complete(2));
  assert(begin(0));
  assert(!wire.begin(start.data(),start.size(),openssl_hmac,key,"tab5-002",nonce,1,reply));
  assert(!complete(2));
  assert(begin(UINT32_MAX-5));
  assert(complete(4));
  assert(begin(0));
  wire.expire(10001);
  assert(!complete(1));
  assert(begin(0));
  std::string altered=finish;altered[69]=altered[69]=='a'?'b':'a';
  assert(!wire.finish(altered.data(),altered.size(),1,reply));
  assert(!complete(2));
  const std::string long_id(64,'a');
  const std::string long_start="AUTH1,"+long_id+","+hex(host);
  assert(long_start.size()==135);
  assert(wire.begin(long_start.data(),long_start.size(),openssl_hmac,key,long_id.c_str(),nonce,0,reply));
  const std::string oversize(137,'a');
  assert(!wire.begin(oversize.data(),oversize.size(),openssl_hmac,key,long_id.c_str(),nonce,0,reply));
}
