#include "battery_monitor.h"
#include "learner_layout.h"
#include <cassert>
int main() {
  BatteryMonitor monitor;
  assert(monitor.due(0)&&monitor.status.percent==-1);
  assert(monitor.update(0,73,BatteryCharge::NotCharging));
  assert(!monitor.due(4999)&&monitor.due(5000));
  assert(!monitor.update(5000,73,BatteryCharge::NotCharging));
  assert(monitor.update(10000,20,BatteryCharge::NotCharging));
  assert(monitor.status.low()&&!monitor.status.critical());
  assert(monitor.update(15000,10,BatteryCharge::NotCharging));
  assert(monitor.status.critical());
  assert(monitor.update(20000,10,BatteryCharge::Charging));
  assert(!monitor.status.low());
  assert(monitor.update(25000,-1,BatteryCharge::Unknown));
  assert(monitor.status.percent==-1&&!monitor.status.low());
  assert(battery_status(101,BatteryCharge::Unknown).percent==-1);
  assert(battery_status(0,BatteryCharge::Unknown).critical());
  assert(!battery_status(100,BatteryCharge::Full).low());
  monitor.update(UINT32_MAX-1000,50,BatteryCharge::Unknown);
  assert(!monitor.due(3998)&&monitor.due(3999));
  const auto& r=learner_battery_region;
  assert(r.x>=820&&r.x+r.w<=learner_help.x&&r.y+r.h<=108);
  LearnerNavigation navigation;
  LearnerAction action;
  assert(!learner_hit(navigation,r.x+1,r.y+1,action));
}
