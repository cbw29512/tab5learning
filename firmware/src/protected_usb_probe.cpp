#ifdef POCKET_PROTECTED_USB_PROBE
#include <Arduino.h>
#include <HWCDC.h>
#include "probe_snapshot_transport.h"
#include "learner_mbedtls_gcm.h"
#include "learner_mbedtls_auth.h"
#include "learner_nonce_drbg.h"

namespace {
using namespace learner_auth;
// Deliberately public test key. This entire environment is NOT a release.
Bytes synthetic_key{};
OfflineNonceGenerator rng;
ProbeSnapshotTransport transport;
bool ready=false;

bool fresh_nonce(Bytes& out) {return rng.next(out);}

struct ProbeStream {
  HWCDC& serial;
  bool connected() {return serial.isConnected();}
  int available() {return serial.available();}
  int read() {return serial.read();}
  int availableForWrite() {return connected()?serial.availableForWrite():0;}
  std::size_t write(const uint8_t* data,std::size_t n) {
    if(!connected()) return 0;
    const auto written=serial.write(data,n);
    return connected()?written:0;
  }
};
ProbeStream stream{HWCDCSerial};
}

void setup() {
  // Isolated startup only. No M5/ADC initialization, storage, or radio.
  const bool seeded=rng.seed_at_exclusive_startup();
  for(unsigned i=0;i<synthetic_key.size();++i) synthetic_key[i]=i;
  ready=seeded && platform_auth_self_test();
  HWCDCSerial.begin(115200);
  HWCDCSerial.setDebugOutput(false);
  HWCDCSerial.setTxTimeoutMs(1);
  delay(500);
  // No diagnostic text is mixed with the protocol stream.
}
void loop() {
  if(ready) transport.poll(stream,platform_hmac,platform_gcm,synthetic_key,fresh_nonce,millis());
  delay(1);
}
#endif
