#ifdef POCKET_SD_WRITE_PROBE
#include <Arduino.h>
#include <M5Unified.h>
#include <SPI.h>
#include <SD.h>
#include "curriculum_sd_fixture.h"
#include "learner_storage_ui.h"
#include "learner_battery_ui.h"

namespace {
using namespace curriculum_preview;
constexpr char path[]="/sd/PLTST01.BIN";
constexpr int cs=42,sck=43,mosi=44,miso=39;
void message(const char* text){
  M5.Display.println(text);Serial.println(text);
}
bool verify_readonly(){
  // Existing path is read-only, never truncated, appended to or removed.
  const int fd=open(path,O_RDONLY);
  if(fd<0)return false;
  struct stat info{};
  bool ok=fstat(fd,&info)==0&&S_ISREG(info.st_mode)&&info.st_size==192;
  for(unsigned i=0;ok&&i<2;++i){
    CompletionBytes expected{},actual{};
    ok=probe_expected(i,expected)&&
      ::read(fd,actual.data(),actual.size())==static_cast<ssize_t>(actual.size())&&actual==expected;
  }
  char tail=0;
  if(ok)ok=::read(fd,&tail,1)==0;
  return close(fd)==0&&ok;
}
void exercise(){
  // A successful exclusive create is the only authority to write this file.
  // EEXIST is the second-boot/read-only path, not permission to modify it.
  const int fd=open(path,O_CREAT|O_EXCL|O_RDWR|O_APPEND,0600);
  if(fd<0){
    const int error=errno;
    if(error!=EEXIST){message("FAIL: exclusive create unavailable");return;}
    if(verify_readonly())message("EXISTING FIXTURE VERIFIED (read only)");
    else message("FAIL: existing fixture differs; preserved");
    return;
  }
  static CompletionFile file(fd);
  // 256-entry cache belongs in static storage, not the ESP loop stack.
  static CompletionLedger ledger(file);
  const bool saved=file.begin()&&probe_initialize(file,ledger);
  const bool closed=close(fd)==0;
  if(!saved||!closed){
    message("FAIL: write/sync uncertain; preserve test file");return;
  }
  if(!verify_readonly()){message("FAIL: reopen/readback");return;}
  message("WRITE AND REOPEN PASSED: 192 bytes");
  message("Restart once to verify persistence.");
}
}
void setup(){
  M5.begin(M5.config());Serial.begin(115200);delay(2000);
  M5.Display.setRotation(3);M5.Display.setBrightness(128);
  M5.Display.fillScreen(TFT_NAVY);M5.Display.setCursor(32,144);
  M5.Display.setTextSize(3);M5.Display.setTextColor(TFT_WHITE,TFT_NAVY);
  message("Pocket Learning: synthetic SD write probe");
  message("Only /PLTST01.BIN may be created.");
  SPI.begin(sck,miso,mosi,cs);
  if(!SD.begin(cs,SPI,4000000,"/sd",5,false)){
    message("FAIL: SD mount; never format");
    learner_storage::render_header(learner_storage::unavailable());
    SPI.end();return;
  }
  exercise();
  // One post-operation sample on the already-mounted card, never a
  // background FAT scan during touch, audio or learner evidence capture.
  const auto status=learner_storage::measured(SD.totalBytes(),SD.usedBytes());
  char summary[128];learner_storage::format_summary(status,summary,sizeof(summary));
  message(summary);message(learner_storage::state_label(status.state));
  SD.end();SPI.end();
  message("SD unmounted. No other files touched by app.");
  // Render adjacent live readings after diagnostic messages; no added mount
  // or learner writes, and existing charger code is unchanged.
  BatteryMonitor battery;BatteryRuntime runtime;
  sample_learner_battery(battery,runtime,millis());
  M5.Display.startWrite();
  learner_storage::render_header(status);
  render_learner_battery(battery.status);
  M5.Display.endWrite();
  M5.Display.display(0,0,M5.Display.width(),M5.Display.height());
}
void loop(){M5.update();M5.delay(100);}
#endif
