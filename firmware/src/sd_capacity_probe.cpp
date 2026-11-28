#ifdef POCKET_SD_CAPACITY_PROBE
#include <Arduino.h>
#include <M5Unified.h>
#include <SPI.h>
#include <SD.h>
#include "learner_storage_format.h"

// Manufacturer-documented Tab5 SPI wiring. Isolated compile/physical probe.
// No file opens, enumeration, create/delete, format or student storage.
// Arduino's mounted filesystem is not a guaranteed read-only block device;
// use only an independently backed-up development card.
namespace {
constexpr int cs=42,sck=43,mosi=44,miso=39;
void message(const char* text){
  M5.Display.setTextColor(TFT_WHITE,TFT_NAVY);
  M5.Display.setTextSize(3);M5.Display.println(text);
  Serial.println(text);
}
}
void setup(){
  M5.begin(M5.config());Serial.begin(115200);
  delay(2000);M5.Display.setRotation(3);M5.Display.setBrightness(128);
  M5.Display.fillScreen(TFT_NAVY);M5.Display.setCursor(32,40);
  message("Pocket Learning: SD capacity probe");
  message("No format or application file writes.");
  SPI.begin(sck,miso,mosi,cs);
  // Explicit format_if_empty=false; lower clock for initial compatibility.
  if(!SD.begin(cs,SPI,4000000,"/sd",5,false)){
    message("SD mount failed. Do NOT format the card.");SPI.end();return;
  }
  const auto card=SD.cardSize();
  const auto total=SD.totalBytes();
  const auto used=SD.usedBytes();
  if(!card||!total||used>total){
    message("Capacity unavailable or inconsistent.");
  }else{
    char line[96];
    snprintf(line,sizeof(line),"Card bytes: %llu",static_cast<unsigned long long>(card));message(line);
    snprintf(line,sizeof(line),"Filesystem bytes: %llu",static_cast<unsigned long long>(total));message(line);
    snprintf(line,sizeof(line),"Used bytes: %llu",static_cast<unsigned long long>(used));message(line);
    const auto status=learner_storage::measured(total,used);
    char summary[128];learner_storage::format_summary(status,summary,sizeof(summary));
    message(summary);message(learner_storage::state_label(status.state));
    message("SD CAPACITY PROBE PASSED");
  }
  SD.end();SPI.end();
}
void loop(){M5.update();M5.delay(100);}
#endif
