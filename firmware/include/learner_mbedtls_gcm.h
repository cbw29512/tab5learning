#pragma once
#include "learner_protected_frame.h"

namespace learner_auth {
bool platform_gcm(bool encrypt,const Bytes& key,const GcmNonce& nonce,
                  const uint8_t* aad,std::size_t aad_len,const uint8_t* in,
                  std::size_t len,uint8_t* out,GcmTag& tag);
}
