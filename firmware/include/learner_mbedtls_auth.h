#pragma once
#include "learner_device_auth.h"

// ESP-IDF Mbed TLS adapter. No key storage, entropy changes, or serial access.
namespace learner_auth {
bool platform_hmac(const Bytes& key,const uint8_t* data,std::size_t size,Bytes& out);
bool platform_auth_self_test();
}
