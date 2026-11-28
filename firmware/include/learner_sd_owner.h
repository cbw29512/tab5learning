#pragma once
#include <SPI.h>
#include <SD.h>
#include "curriculum_completion_owner.h"
#include "learner_storage_status.h"

namespace curriculum_preview {
// Compile-only adult evaluation candidate. Card must remain installed
// throughout the session. No automatic retry, hot-swap, formatting or erase.
class LearnerSdOwner {
  CompletionOwner completion;
  bool attempted=false;
 public:
  bool begin(learner_storage::Status& status){
    status=learner_storage::unavailable();
    if(attempted)return false;
    attempted=true;
    constexpr int cs=42,sck=43,mosi=44,miso=39;
    SPI.begin(sck,miso,mosi,cs);
    if(!SD.begin(cs,SPI,4000000,"/sd",5,false)){
      SPI.end();std::printf("ERR,LEARNER_SD_MOUNT\n");return false;
    }
    // Separate from PLTST01.BIN; this file contains only draft completion
    // evidence, not student identity, grades, ink or audio.
    if(!completion.open_once("/sd/PLEVAL01.BIN")){
      SD.end();SPI.end();return false;
    }
    status=learner_storage::measured(SD.totalBytes(),SD.usedBytes());
    if(!status.known()){
      completion.close_owner();SD.end();SPI.end();return false;
    }
    return true;
  }
  CompletionLedger* view(){return completion.view();}
};
}
