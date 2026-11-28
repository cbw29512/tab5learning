#pragma once
#include "learner_device_auth.h"
#include <mbedtls/ctr_drbg.h>

namespace learner_auth {
// Isolated probe only. Caller must own ADC entropy exclusively at startup.
class OfflineNonceGenerator {
 public:
  OfflineNonceGenerator();
  ~OfflineNonceGenerator();
  OfflineNonceGenerator(const OfflineNonceGenerator&)=delete;
  OfflineNonceGenerator& operator=(const OfflineNonceGenerator&)=delete;
  bool seed_at_exclusive_startup();
  bool next(Bytes& out);
  static constexpr unsigned max_nonces=1024;

 private:
  enum class State { New, Seeding, Ready, Failed, Exhausted };
  static int entropy(void* context,unsigned char* out,std::size_t size);
  mbedtls_ctr_drbg_context context_{};
  State state_=State::New;
  bool entropy_active_=false;
  unsigned generated_=0;
};
bool platform_nonce_self_test();
}
