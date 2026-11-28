#pragma once
#include <mbedtls/ctr_drbg.h>
inline void esp_fill_random(void* out,std::size_t n){
  assert(fake_nonce::active);
  ++fake_nonce::entropy_calls;
  auto* bytes=static_cast<unsigned char*>(out);
  for(std::size_t i=0;i<n;++i) bytes[i]=static_cast<unsigned char>(i);
}
