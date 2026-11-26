#ifdef POCKET_MIC_COMPILE_PROBE
#include <Arduino.h>
#include "m5_reading_capture.h"

// Build/link probe only. Never initialize the board, microphone, speaker,
// networking, storage or student data. The pump remains permanently inactive.
namespace {
M5ReadingCapture microphone;
ReadingRecording recording;
ReadingCapturePump pump(recording,microphone);
}

void setup() {
  Serial.begin(115200);
  Serial.println("MIC_COMPILE_PROBE_ONLY,NO_CAPTURE");
}

void loop() {
  pump.tick(millis());
  delay(100);
}
#endif
