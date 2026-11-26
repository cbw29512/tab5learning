#ifdef POCKET_AUTH_COMPILE_PROBE
#include <Arduino.h>
#include "learner_mbedtls_auth.h"

// Isolated compile/runtime self-test. Never install over the learner by default.
void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println(learner_auth::platform_auth_self_test()
      ? "AUTH_CRYPTO_SELF_TEST,PASS" : "AUTH_CRYPTO_SELF_TEST,FAIL");
}
void loop() { delay(1000); }
#endif
