#include "learner_mbedtls_auth.h"
#include <mbedtls/md.h>

namespace learner_auth {
bool platform_hmac(const Bytes& key,const uint8_t* data,std::size_t size,Bytes& out) {
  out.fill(0);
  if(!data || size==0 || size>sizeof("TAB5-AUTH-V1")+sizeof("device")+65+64) return false;
  const auto* info=mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
  if(!info || mbedtls_md_get_size(info)!=out.size()) return false;
  if(mbedtls_md_hmac(info,key.data(),key.size(),data,size,out.data())!=0) {
    out.fill(0);
    return false;
  }
  return true;
}

bool platform_auth_self_test() {
  // Public deterministic test vectors, NEVER deployed credentials or random nonces.
  Bytes key{},host{},device{},dp{},hp{};
  for(unsigned i=0;i<32;++i) { key[i]=i; host[i]=i+32; device[i]=i+64; }
  const Bytes expected_device={0x67,0x45,0x6b,0x13,0x88,0xd3,0x02,0xd0,0x45,0xb1,0xdb,0xac,0x3a,0x5b,0xab,0x93,0x90,0x8a,0xd2,0xd5,0xc8,0x3b,0xaa,0x8d,0xd9,0xac,0x15,0xd8,0xe9,0xc7,0xef,0x6e};
  const Bytes expected_host={0x06,0xc1,0x5e,0x0a,0x3d,0xd2,0x3a,0xe9,0x29,0x0f,0xe5,0x9f,0xf7,0xcc,0xc0,0x03,0x05,0x68,0x83,0x5f,0xf8,0x67,0xeb,0x69,0xfc,0xd4,0xc3,0xc5,0x1b,0x3d,0x33,0x88};
  DeviceChallenge challenge;
  if(!challenge.begin(platform_hmac,key,"tab5-001",host,device,0,dp) ||
     !equal_proof(dp,expected_device) ||
     !make_proof(platform_hmac,key,Role::Host,"tab5-001",host,device,hp) ||
     !equal_proof(hp,expected_host) || !challenge.complete(hp,1) ||
     challenge.complete(hp,2)) return false;
  if(!challenge.begin(platform_hmac,key,"tab5-001",host,device,3,dp) ||
     challenge.complete(dp,4) || challenge.complete(hp,5)) return false;
  return true;
}
}
