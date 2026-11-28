#include "battery_runtime.h"
#include <cassert>
int main() {
  BatteryRuntime r;
  LearnerNavigation n;
  r.observe(100,n);
  r.observe(1100,n);
  n.screen=LearnerScreen::Math;
  r.observe(1100,n);
  r.observe(6100,n);
  assert(r.total_ms()==6000 && r.screen_ms(LearnerScreen::Math)==5000);
  n.help_open=true;
  r.observe(6100,n);
  r.observe(7100,n);
  assert(r.total_ms()==7000 && r.screen_ms(LearnerScreen::Math)==5000);
  r.sample(100,battery_status(80,BatteryCharge::Discharging));
  r.sample(5100,battery_status(79,BatteryCharge::Discharging));
  assert(r.discharge_ms()==5000);
  r.sample(10100,battery_status(79,BatteryCharge::NotCharging));
  r.sample(15100,battery_status(78,BatteryCharge::Discharging));
  r.sample(40100,battery_status(77,BatteryCharge::Discharging));
  r.sample(45100,battery_status(-1,BatteryCharge::Discharging));
  r.sample(50100,battery_status(76,BatteryCharge::Discharging));
  assert(r.discharge_ms()==5000);
  r.sample(55100,battery_status(75,BatteryCharge::Discharging));
  assert(r.discharge_ms()==10000);
  BatteryRuntime wrap;
  wrap.observe(UINT32_MAX-99,n);
  wrap.observe(100,n);
  assert(wrap.total_ms()==200);
  wrap.sample(UINT32_MAX-99,battery_status(70,BatteryCharge::Discharging));
  wrap.sample(100,battery_status(70,BatteryCharge::Discharging));
  assert(wrap.discharge_ms()==200);
}
