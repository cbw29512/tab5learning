#ifdef POCKET_AUTH_COMPILE_PROBE
#include <Arduino.h>
#include "learner_mbedtls_auth.h"
#include "learner_mbedtls_gcm.h"
#include "learner_nonce_drbg.h"
#include "learner_secure_credential_nvs.h"

// Isolated compile/runtime self-test. Never install over the learner by default.
void setup() {
  // Isolated probe: run before M5.begin, ADC or application audio setup.
  const bool nonce_ok=learner_auth::platform_nonce_self_test();
  Serial.begin(115200);
  delay(2000);
  // Read-only readiness check; no key generation, storage initialization or writes.
  Serial.println(learner_auth::SecureCredentialNvs::prerequisites()?
      "SECURE_STORAGE_PREREQUISITES,PRESENT":"SECURE_STORAGE_PREREQUISITES,ABSENT");
  Serial.println(nonce_ok ? "AUTH_NONCE_SELF_TEST,PASS" : "AUTH_NONCE_SELF_TEST,FAIL");
  static learner_auth::ProtectedFrameSession sender,receiver;
  static learner_auth::SessionFrame frame;
  const learner_auth::Bytes public_test_key{};
  const uint8_t sample[]={1,2,3};
  const uint8_t* received=nullptr;std::size_t used=0,size=0;
  const bool protected_ok=sender.begin(learner_auth::platform_hmac,learner_auth::platform_gcm,public_test_key,true,0)&&
      receiver.begin(learner_auth::platform_hmac,learner_auth::platform_gcm,public_test_key,false,0)&&
      sender.send(sample,sizeof(sample),1,frame,used)&&receiver.receive(frame.data(),used,1,received,size)&&
      size==sizeof(sample)&&std::memcmp(received,sample,size)==0;
  sender.close();receiver.close();
  Serial.println(protected_ok?"AUTH_GCM_SELF_TEST,PASS":"AUTH_GCM_SELF_TEST,FAIL");
  Serial.println(learner_auth::platform_auth_self_test()
      ? "AUTH_CRYPTO_SELF_TEST,PASS" : "AUTH_CRYPTO_SELF_TEST,FAIL");
}
void loop() { delay(1000); }
#endif
