#pragma once
#include "learner_protected_frame.h"
#include <openssl/evp.h>

inline bool test_gcm(bool encrypt,const learner_auth::Bytes& key,
                     const learner_auth::GcmNonce& nonce,const uint8_t* aad,
                     std::size_t aad_len,const uint8_t* in,std::size_t len,
                     uint8_t* out,learner_auth::GcmTag& tag){
  auto* ctx=EVP_CIPHER_CTX_new();
  if(!ctx)return false;
  int n=0,total=0;
  bool ok=EVP_CipherInit_ex(ctx,EVP_aes_256_gcm(),nullptr,nullptr,nullptr,encrypt)==1&&
    EVP_CIPHER_CTX_ctrl(ctx,EVP_CTRL_GCM_SET_IVLEN,int(nonce.size()),nullptr)==1&&
    EVP_CipherInit_ex(ctx,nullptr,nullptr,key.data(),nonce.data(),encrypt)==1&&
    EVP_CipherUpdate(ctx,nullptr,&n,aad,int(aad_len))==1&&
    EVP_CipherUpdate(ctx,out,&n,in,int(len))==1;
  total=n;
  if(ok&&!encrypt)ok=EVP_CIPHER_CTX_ctrl(ctx,EVP_CTRL_GCM_SET_TAG,int(tag.size()),tag.data())==1;
  uint8_t tail[16]{};
  if(ok)ok=EVP_CipherFinal_ex(ctx,tail,&n)==1&&n==0&&total==int(len);
  if(ok&&encrypt)ok=EVP_CIPHER_CTX_ctrl(ctx,EVP_CTRL_GCM_GET_TAG,int(tag.size()),tag.data())==1;
  EVP_CIPHER_CTX_free(ctx);
  if(!ok)std::memset(out,0,len);
  return ok;
}
