#include "learner_device_auth.h"
#include <openssl/hmac.h>
#include <cassert>
#include <cstdio>
#include <string>

using namespace learner_auth;
bool openssl_hmac(const Bytes& key,const uint8_t* data,std::size_t size,Bytes& out) {
  unsigned int written=0;
  return HMAC(EVP_sha256(),key.data(),int(key.size()),data,size,out.data(),&written)
      && written==out.size();
}
bool fail_hmac(const Bytes&,const uint8_t*,std::size_t,Bytes&) { return false; }
std::string hex(const Bytes& bytes) {
  std::string result;
  for(auto b:bytes) { char tmp[3]; std::snprintf(tmp,sizeof(tmp),"%02x",unsigned(b)); result+=tmp; }
  return result;
}
int main() {
  Bytes key{},host{},device{},dp{},hp{},wrong{};
  for(unsigned i=0;i<32;++i) { key[i]=i; host[i]=i+32; device[i]=i+64; }
  DeviceChallenge auth;
  assert(!auth.complete(hp,0));
  assert(auth.begin(openssl_hmac,key,"tab5-001",host,device,0,dp));
  assert(hex(dp)=="67456b1388d302d045b1dbac3a5bab93908ad2d5c83baa8dd9ac15d8e9c7ef6e");
  assert(make_proof(openssl_hmac,key,Role::Host,"tab5-001",host,device,hp));
  assert(hex(hp)=="06c15e0a3dd23ae9290fe59ff7ccc0030568835ff867eb69fcd4c3c51b3d3388");
  assert(auth.complete(hp,10000));
  assert(!auth.complete(hp,10000));
  assert(auth.begin(openssl_hmac,key,"tab5-001",host,device,0,dp));
  assert(!auth.complete(dp,1)); // reflection fails, consumes challenge
  assert(!auth.complete(hp,1));
  assert(auth.begin(openssl_hmac,key,"tab5-001",host,device,0,dp));
  assert(!auth.complete(hp,10001));
  assert(auth.begin(openssl_hmac,key,"tab5-001",host,device,UINT32_MAX-5,dp));
  assert(auth.complete(hp,4)); // millis wraps
  assert(auth.begin(openssl_hmac,key,"tab5-001",host,device,0,dp));
  assert(!auth.begin(openssl_hmac,key,"BAD ID",host,device,1,dp));
  assert(!auth.pending());
  assert(!auth.complete(hp,2));
  assert(!auth.begin(fail_hmac,key,"tab5-001",host,device,0,dp));
  assert(dp==Bytes{});
  assert(!auth.pending());
  assert(auth.begin(openssl_hmac,key,"tab5-001",host,device,0,dp));
  wrong=hp; wrong[31]^=1;
  assert(!auth.complete(wrong,1));
  assert(auth.begin(openssl_hmac,key,"tab5-002",host,device,0,dp));
  assert(!auth.complete(hp,1));
  assert(auth.begin(openssl_hmac,key,"tab5-001",host,device,0,dp));
  device[0]^=1;
  assert(auth.begin(openssl_hmac,key,"tab5-001",host,device,1,dp));
  assert(!auth.complete(hp,2));
  assert(auth.begin(openssl_hmac,key,"tab5-001",host,device,0,dp));
  auth.cancel();
  assert(!auth.complete(hp,1));
}
