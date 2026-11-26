#include "learner_mbedtls_gcm.h"
#include <mbedtls/gcm.h>

namespace learner_auth {
bool platform_gcm(bool encrypt,const Bytes& key,const GcmNonce& nonce,
                  const uint8_t* aad,std::size_t aad_len,const uint8_t* in,
                  std::size_t len,uint8_t* out,GcmTag& tag){
  if(!aad||aad_len!=7||!in||!out||!len||len>session_max_payload)return false;
  mbedtls_gcm_context ctx;
  mbedtls_gcm_init(&ctx);
  int rc=mbedtls_gcm_setkey(&ctx,MBEDTLS_CIPHER_ID_AES,key.data(),256);
  if(rc==0){
    if(encrypt)
      rc=mbedtls_gcm_crypt_and_tag(&ctx,MBEDTLS_GCM_ENCRYPT,len,nonce.data(),nonce.size(),
                                  aad,aad_len,in,out,tag.size(),tag.data());
    else
      rc=mbedtls_gcm_auth_decrypt(&ctx,len,nonce.data(),nonce.size(),aad,aad_len,
                                  tag.data(),tag.size(),in,out);
  }
  mbedtls_gcm_free(&ctx);
  if(rc!=0){std::memset(out,0,len);return false;}
  return true;
}
}
