#include "learner_usb_identity.h"
#include <Arduino.h>
namespace {
LearnerUsbIdentity parser;
}
void learner_usb_identity_poll() {
  // Keep touch and rendering responsive even if USB receives a noisy stream.
  for(unsigned i=0;i<32 && Serial.available()>0;++i) {
    const int raw=Serial.read();
    if(raw<0) break;
    char nonce[33]{};
    if(parser.feed(static_cast<unsigned char>(raw),nonce)) {
      // A disconnected or stalled host must not block the learner loop.
      char reply[80]{};
      const int n=snprintf(reply,sizeof(reply),"LEARNER,1,%s,TAB5_LEARNER\n",nonce);
      if(n>0 && n<static_cast<int>(sizeof(reply)) && Serial.availableForWrite()>=n) {
        Serial.write(reinterpret_cast<const uint8_t*>(reply),static_cast<size_t>(n));
      }
    }
  }
}
