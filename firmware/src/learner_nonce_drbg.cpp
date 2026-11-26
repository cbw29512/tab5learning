#include "learner_nonce_drbg.h"
#include <bootloader_random.h>
#include <esp_random.h>

namespace learner_auth {
OfflineNonceGenerator::OfflineNonceGenerator() { mbedtls_ctr_drbg_init(&context_); }
OfflineNonceGenerator::~OfflineNonceGenerator() { mbedtls_ctr_drbg_free(&context_); }

int OfflineNonceGenerator::entropy(void* context,unsigned char* out,std::size_t size) {
  auto* self=static_cast<OfflineNonceGenerator*>(context);
  if(!self || !self->entropy_active_ || self->state_!=State::Seeding ||
     !out || size==0 || size>48) return MBEDTLS_ERR_CTR_DRBG_ENTROPY_SOURCE_FAILED;
  esp_fill_random(out,size);
  return 0;
}

bool OfflineNonceGenerator::seed_at_exclusive_startup() {
  if(state_!=State::New) return false;
  state_=State::Seeding;
  mbedtls_ctr_drbg_set_entropy_len(&context_,48);
  if(mbedtls_ctr_drbg_set_nonce_len(&context_,0)!=0) {
    state_=State::Failed; return false;
  }
  mbedtls_ctr_drbg_set_prediction_resistance(&context_,MBEDTLS_CTR_DRBG_PR_OFF);
  mbedtls_ctr_drbg_set_reseed_interval(&context_,max_nonces+1);
  constexpr unsigned char label[]="PocketLearning/USB-nonce/v1";
  // No other peripheral initialization is permitted during this scope.
  bootloader_random_enable();
  entropy_active_=true;
  const int result=mbedtls_ctr_drbg_seed(&context_,entropy,this,label,sizeof(label)-1);
  entropy_active_=false;
  bootloader_random_disable();
  state_=result==0?State::Ready:State::Failed;
  return state_==State::Ready;
}

bool OfflineNonceGenerator::next(Bytes& out) {
  out.fill(0);
  if(state_!=State::Ready) return false;
  if(generated_>=max_nonces) {
    state_=State::Exhausted; return false;
  }
  if(mbedtls_ctr_drbg_random(&context_,out.data(),out.size())!=0) {
    out.fill(0); state_=State::Failed; return false;
  }
  ++generated_;
  if(generated_==max_nonces) state_=State::Exhausted;
  return true;
}

bool platform_nonce_self_test() {
  OfflineNonceGenerator rng;
  Bytes first{},next{};
  if(rng.next(first) || first!=Bytes{} || !rng.seed_at_exclusive_startup()) return false;
  if(!rng.next(first) || !rng.next(next) || equal_proof(first,next)) return false;
  for(unsigned i=2;i<OfflineNonceGenerator::max_nonces;++i) {
    if(!rng.next(next)) return false;
  }
  return !rng.next(next) && next==Bytes{} && !rng.seed_at_exclusive_startup();
}
}
