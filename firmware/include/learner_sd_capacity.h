#pragma once
#include <SPI.h>
#include <SD.h>
#include <cstdio>
#include "learner_storage_status.h"

namespace learner_storage {
// Operator-triggered adult preview check. Use only a backed-up card.
// The mounted filesystem is not a guaranteed read-only block device.
// No application file writes, formatting or automatic retries.
inline Status check_capacity() {
  constexpr int cs=42,sck=43,mosi=44,miso=39;
  SPI.begin(sck,miso,mosi,cs);
  if(!SD.begin(cs,SPI,4000000,"/sd",5,false)){
    SPI.end();std::printf("ERR,SD_CAPACITY_MOUNT\n");return unavailable();
  }
  const auto card=SD.cardSize();
  const auto total=SD.totalBytes();
  const auto used=SD.usedBytes();
  const auto result=card?measured(total,used):unavailable();
  SD.end();SPI.end();
  if(!result.known())std::printf("ERR,SD_CAPACITY_INVALID\n");
  else std::printf("SD,CAPACITY,%llu,%llu\n",
    static_cast<unsigned long long>(total),static_cast<unsigned long long>(used));
  return result;
}
}
