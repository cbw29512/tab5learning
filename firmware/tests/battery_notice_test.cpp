#include "battery_notice.h"
#include "learner_layout.h"
#include <cassert>
#include <cstring>
int main() {
  LearnerNavigation nav;
  const auto empty=battery_status(-1,BatteryCharge::Unknown);
  const auto low=battery_status(20,BatteryCharge::Discharging);
  const auto critical=battery_status(10,BatteryCharge::Discharging);
  assert(battery_notice(empty,nav,false)==BatteryNotice::None);
  assert(battery_notice(low,nav,false)==BatteryNotice::Low);
  assert(battery_notice(critical,nav,false)==BatteryNotice::Critical);
  nav.screen=LearnerScreen::Math;
  assert(battery_notice(critical,nav,false)==BatteryNotice::CriticalMath);
  assert(battery_notice(critical,nav,true)==BatteryNotice::CriticalStorageError);
  assert(std::strstr(battery_notice_text(BatteryNotice::CriticalStorageError),"unavailable"));
  nav.help_open=true;
  assert(battery_notice(critical,nav,false)==BatteryNotice::Critical);
  assert(battery_notice(battery_status(5,BatteryCharge::Charging),nav,false)==BatteryNotice::None);
  assert(battery_notice(battery_status(15,BatteryCharge::NotCharging),nav,false)==BatteryNotice::Low);
  const auto& r=learner_notice_region;
  assert(r.y>=learner_battery_region.y+learner_battery_region.h);
  assert(r.y>=learner_home.y+learner_home.h);
  assert(r.y+r.h<learner_tiles[0].y);
  LearnerAction action;
  assert(!learner_hit(nav,r.x+1,r.y+1,action));
}
