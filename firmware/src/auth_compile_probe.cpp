#ifdef POCKET_AUTH_COMPILE_PROBE
#include <Arduino.h>
#include "learner_mbedtls_auth.h"
#include "learner_nonce_drbg.h"

// Isolated compile/runtime self-test. Never install over the learner by default.
void setup() {
  // Isolated probe: run before M5.begin, ADC or application audio setup.
  const bool nonce_ok=learner_auth::platform_nonce_self_test();
  Serial.begin(115200);
  delay(2000);
  Serial.println(nonce_ok ? "AUTH_NONCE_SELF_TEST,PASS" : "AUTH_NONCE_SELF_TEST,FAIL");
  Serial.println(learner_auth::platform_auth_self_test()
      ? "AUTH_CRYPTO_SELF_TEST,PASS" : "AUTH_CRYPTO_SELF_TEST,FAIL");
}
void loop() { delay(1000); }
#endif
