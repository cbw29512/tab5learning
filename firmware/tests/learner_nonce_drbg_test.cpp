#include "learner_nonce_drbg.h"
#include <cassert>
#include <type_traits>
using namespace learner_auth;
static_assert(!std::is_copy_constructible<OfflineNonceGenerator>::value);
int main(){
  Bytes out{};out.fill(99);
  assert(platform_nonce_self_test());
  assert(fake_nonce::enables==1 && fake_nonce::disables==1 && fake_nonce::entropy_calls==1);
  fake_nonce::fail_seed=true;
  OfflineNonceGenerator failed;
  assert(!failed.seed_at_exclusive_startup());
  assert(!fake_nonce::active && fake_nonce::enables==2 && fake_nonce::disables==2);
  assert(!failed.next(out) && out==Bytes{});
  assert(!failed.seed_at_exclusive_startup());
  fake_nonce::fail_seed=false;
  fake_nonce::fail_nonce=true;
  OfflineNonceGenerator invalid;
  assert(!invalid.seed_at_exclusive_startup());
  assert(fake_nonce::enables==2 && fake_nonce::disables==2);
  fake_nonce::fail_nonce=false;
  OfflineNonceGenerator generator;
  assert(generator.seed_at_exclusive_startup());
  fake_nonce::fail_random=true;
  assert(!generator.next(out) && out==Bytes{});
  fake_nonce::fail_random=false;
  assert(!generator.next(out) && !generator.seed_at_exclusive_startup());
  assert(fake_nonce::enables==3 && fake_nonce::disables==3 && !fake_nonce::active);
}
