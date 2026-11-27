#pragma once
#include <M5Unified.h>
#include <cstdio>
#include "battery_monitor.h"
#include "learner_layout.h"
#include "battery_runtime.h"

// Read-only diagnostic for the operator-reported inverted charging status.
// Do not infer USB attachment or charge completion from voltage/percent alone.
struct BatteryDiagnostic {
  int raw_state=0;
  int32_t current_ma=0;
  int16_t voltage_mv=0;
};
inline BatteryDiagnostic learner_battery_diagnostic{};
inline bool sample_learner_battery(BatteryMonitor& monitor,BatteryRuntime& runtime,std::uint32_t now) {
  const auto percent=M5.Power.getBatteryLevel();
  const auto raw=M5.Power.getChargeState();
  learner_battery_diagnostic={static_cast<int>(raw),
      M5.Power.getBatteryCurrent(),M5.Power.getBatteryVoltage()};
  // Physical report contradicts the driver on this Tab5. Until the current
  // polarity is verified on hardware, withhold the charging claim and keep
  // low-battery warnings conservative. This does not control the charger.
  const auto charge=BatteryCharge::Unknown;
  const bool changed=monitor.update(now,percent,charge);
  runtime.sample(now,monitor.status);
  std::printf("BATTERY,LEVEL,%d,RAW_STATE,%d,CURRENT_MA,%ld,VOLTAGE_MV,%d\n",
      monitor.status.percent,learner_battery_diagnostic.raw_state,
      static_cast<long>(learner_battery_diagnostic.current_ma),
      static_cast<int>(learner_battery_diagnostic.voltage_mv));
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
    status.charge==BatteryCharge::Unknown?"Check power (Help)":
    status.charge==BatteryCharge::Discharging?"On battery draw":"Not charging";
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

inline void render_battery_runtime(const BatteryRuntime& runtime) {
  M5.Display.fillRect(48,340,1184,208,TFT_NAVY);
  M5.Display.setTextSize(3);
  M5.Display.setTextColor(TFT_WHITE,TFT_NAVY);
  char line[100];
  const auto minutes=[](std::uint64_t ms) { return static_cast<unsigned long long>(ms/60000); };
  std::snprintf(line,sizeof(line),"Session: %llu min | Observed discharge: %llu min",
                minutes(runtime.total_ms()),minutes(runtime.discharge_ms()));
  M5.Display.drawString(line,48,350);
  std::snprintf(line,sizeof(line),"Math: %llu min | Reading: %llu min",
                minutes(runtime.screen_ms(LearnerScreen::Math)),
                minutes(runtime.screen_ms(LearnerScreen::Reading)));
  M5.Display.drawString(line,48,400);
  M5.Display.setTextSize(2);
  M5.Display.drawString("Session diagnostics only. Not estimated battery life.",48,450);
  std::snprintf(line,sizeof(line),"Driver state: %d | Current: %ld mA | Voltage: %d mV",
      learner_battery_diagnostic.raw_state,
      static_cast<long>(learner_battery_diagnostic.current_ma),
      static_cast<int>(learner_battery_diagnostic.voltage_mv));
  M5.Display.drawString(line,48,480);
  M5.Display.drawString("Raw driver readings; charge direction not verified.",48,510);
}
inline void refresh_battery_runtime(const BatteryRuntime& runtime) {
  M5.Display.startWrite();
  render_battery_runtime(runtime);
  M5.Display.endWrite();
  M5.Display.display(48,340,1184,208);
}
