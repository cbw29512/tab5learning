#pragma once
#include "battery_monitor.h"
#include "learner_navigation.h"

// Presentation policy only. Never delay or block a learner's durable save
// based on an approximate battery gauge. No automatic power-off or writes.
enum class BatteryNotice { None, Low, Critical, CriticalMath, CriticalStorageError };
inline BatteryNotice battery_notice(BatteryStatus status,const LearnerNavigation& nav,bool storage_error) {
  if(!status.low()) return BatteryNotice::None;
  if(!status.critical()) return BatteryNotice::Low;
  if(nav.screen==LearnerScreen::Math && !nav.help_open)
    return storage_error?BatteryNotice::CriticalStorageError:BatteryNotice::CriticalMath;
  return BatteryNotice::Critical;
}
inline const char* battery_notice_text(BatteryNotice notice) {
  switch(notice) {
    case BatteryNotice::Low: return "Battery low - please connect your charger soon.";
    case BatteryNotice::Critical: return "Charge now - connect the charger before continuing.";
    case BatteryNotice::CriticalMath: return "Charge now. Submit your answer to save scratchpad work.";
    case BatteryNotice::CriticalStorageError: return "Charge now. Ask your teacher: saving is unavailable.";
    default: return "";
  }
}
