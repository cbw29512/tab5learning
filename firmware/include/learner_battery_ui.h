#pragma once
#include <M5Unified.h>
#include <cstdio>
#include "battery_monitor.h"
#include "learner_layout.h"

// The pinned M5Unified driver owns all PMIC operations. Treat unsupported
// and failed readings as unknown rather than a false charging indication.
inline BatteryCharge learner_charge_state() {
  switch(M5.Power.getChargeState()) {
    case m5::charge_state_t::charging: return BatteryCharge::Charging;
    case m5::charge_state_t::full: return BatteryCharge::Full;
    case m5::charge_state_t::not_charging:
    case m5::charge_state_t::disabled:
    case m5::charge_state_t::discharging:
    case m5::charge_state_t::idle: return BatteryCharge::NotCharging;
    default: return BatteryCharge::Unknown;
  }
}
inline bool sample_learner_battery(BatteryMonitor& monitor,std::uint32_t now) {
  const auto percent=M5.Power.getBatteryLevel();
  const auto charge=learner_charge_state();
  const bool changed=monitor.update(now,percent,charge);
  if(changed) std::printf("BATTERY,LEVEL,%d,CHARGE,%u\n",monitor.status.percent,
                         static_cast<unsigned>(charge));
  return changed;
}
inline void render_learner_battery(const BatteryStatus& status) {
  const auto& r=learner_battery_region;
  const auto color=status.critical()?TFT_RED:status.low()?TFT_YELLOW:
    status.charge==BatteryCharge::Charging?TFT_GREEN:TFT_WHITE;
  M5.Display.fillRect(r.x,r.y,r.w,r.h,TFT_NAVY);
  M5.Display.drawRect(r.x+4,r.y+6,48,24,TFT_WHITE);
  M5.Display.fillRect(r.x+52,r.y+12,4,12,TFT_WHITE);
  if(status.percent>=0) {
    const int width=44*status.percent/100;
    if(width) M5.Display.fillRect(r.x+6,r.y+8,width,20,color);
  }
  char level[20];
  if(status.percent<0) std::snprintf(level,sizeof(level),"--%%");
  else std::snprintf(level,sizeof(level),"%d%%",status.percent);
  M5.Display.setTextSize(3);
  M5.Display.setTextColor(color,TFT_NAVY);
  M5.Display.drawString(level,r.x+64,r.y+5);
  const char* text=status.critical()?"Charge now":status.low()?"Charge soon":
    status.charge==BatteryCharge::Charging?"Charging":
    status.charge==BatteryCharge::Full?"Charged":
    status.charge==BatteryCharge::Unknown?"Status unknown":"Not charging";
  M5.Display.setTextSize(2);
  M5.Display.drawString(text,r.x+4,r.y+42);
}
inline void refresh_learner_battery(const BatteryStatus& status) {
  const auto& r=learner_battery_region;
  M5.Display.startWrite();
  render_learner_battery(status);
  M5.Display.endWrite();
  M5.Display.display(r.x,r.y,r.w,r.h);
}
