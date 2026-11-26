#include "learner_auth_wire.h"
#include <openssl/hmac.h>
#include <cassert>
#include <string>
using namespace learner_auth;
bool mac(const Bytes& k,const uint8_t* p,std::size_t n,Bytes& out){
  unsigned int length=0;
  return HMAC(EVP_sha256(),k.data(),int(k.size()),p,n,out.data(),&length)&&length==out.size();
}
std::string hex(const Bytes& x){char s[65]{};encode_hex(x,s);return s;}
int main(){
  Bytes key{},host{},nonce{},hp{},expected{},issued{};
  for(unsigned i=0;i<32;++i){key[i]=i;host[i]=i+32;nonce[i]=i+64;}
  assert(derive_session_key(mac,key,"tab5-001",host,nonce,expected));
  assert(make_proof(mac,key,Role::Host,"tab5-001",host,nonce,hp));
  DeviceAuthWire wire;AuthLine reply{};
  auto start="AUTH1,tab5-001,"+hex(host),finish="AUTH3,"+hex(hp);
  assert(wire.begin(start.data(),start.size(),mac,key,"tab5-001",nonce,0,reply));
  assert(wire.finish(finish.data(),finish.size(),1,reply,&issued)&&issued==expected);
  assert(!wire.finish(finish.data(),finish.size(),2,reply,&issued)&&issued==Bytes{});
  static FrameSession client,device;
  static SessionFrame frame;
  const uint8_t request[]={1,2,3};
  const uint8_t* p=nullptr;std::size_t n=0,used=0;
  client.begin(mac,expected,true,0);device.begin(mac,expected,false,0);
  assert(client.send(request,3,1,frame,used)&&used==42);
  assert(device.receive(frame.data(),used,1,p,n)&&n==3&&!std::memcmp(p,request,n));
  assert(!device.receive(frame.data(),used,2,p,n)&&p==nullptr&&n==0);
  assert(!device.send(request,3,3,frame,used));
  client.begin(mac,expected,true,0);device.begin(mac,expected,false,0);
  assert(client.send(request,3,1,frame,used));
  frame[7]^=1;
  assert(!device.receive(frame.data(),used,1,p,n));
  device.begin(mac,expected,false,UINT32_MAX-10);
  assert(device.send(request,3,2,frame,used));
  client.begin(mac,expected,true,UINT32_MAX-10);
  assert(client.receive(frame.data(),used,2,p,n));
  assert(!client.send(request,3,120000,frame,used));
}
