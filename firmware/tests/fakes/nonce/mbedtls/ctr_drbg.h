#pragma once
#include <cstddef>
#include <cassert>
#define MBEDTLS_ERR_CTR_DRBG_ENTROPY_SOURCE_FAILED -1
#define MBEDTLS_CTR_DRBG_PR_OFF 0
// TEST DOUBLE ONLY: deliberately deterministic, not cryptography.
namespace fake_nonce {
inline bool active=false, fail_seed=false, fail_random=false, fail_nonce=false;
inline unsigned enables=0, disables=0, entropy_calls=0;
}
struct mbedtls_ctr_drbg_context {
  int (*entropy)(void*,unsigned char*,std::size_t)=nullptr;
  void* user=nullptr;
  std::size_t entropy_size=0;
  unsigned calls=0;
};
inline void mbedtls_ctr_drbg_init(mbedtls_ctr_drbg_context* c) {*c={};}
inline void mbedtls_ctr_drbg_free(mbedtls_ctr_drbg_context*) {}
inline void mbedtls_ctr_drbg_set_entropy_len(mbedtls_ctr_drbg_context* c,std::size_t n) {c->entropy_size=n;}
inline int mbedtls_ctr_drbg_set_nonce_len(mbedtls_ctr_drbg_context*,std::size_t n) {assert(n==0);return fake_nonce::fail_nonce?-1:0;}
inline void mbedtls_ctr_drbg_set_prediction_resistance(mbedtls_ctr_drbg_context*,int n) {assert(n==0);}
inline void mbedtls_ctr_drbg_set_reseed_interval(mbedtls_ctr_drbg_context*,int n) {assert(n==1025);}
inline int mbedtls_ctr_drbg_seed(mbedtls_ctr_drbg_context* c,int (*f)(void*,unsigned char*,std::size_t),void* p,const unsigned char*,std::size_t) {
  assert(fake_nonce::active && c->entropy_size==48);
  c->entropy=f;c->user=p;
  unsigned char buf[48]{};
  if(f(p,buf,sizeof(buf))!=0) return -1;
  return fake_nonce::fail_seed?-1:0;
}
inline int mbedtls_ctr_drbg_random(mbedtls_ctr_drbg_context* c,unsigned char* out,std::size_t n) {
  assert(!fake_nonce::active);
  unsigned char probe[48]{};
  // An unexpected late reseed must be refused by the real adapter.
  assert(c->entropy(c->user,probe,sizeof(probe))!=0);
  ++c->calls;
  for(std::size_t i=0;i<n;++i) out[i]=static_cast<unsigned char>(c->calls+i);
  return fake_nonce::fail_random?-1:0;
}
