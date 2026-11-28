#pragma once
#include <cstdint>

// Read-only status; never command the charger or infer a remaining runtime.
enum class BatteryCharge { Unknown, Charging, NotCharging, Full, Discharging };
struct BatteryStatus {
  int percent=-1;
  BatteryCharge charge=BatteryCharge::Unknown;
  bool low() const {
    return percent>=0 && percent<=20 &&
      charge!=BatteryCharge::Charging && charge!=BatteryCharge::Full;
  }
  bool critical() const { return low() && percent<=10; }
};
inline BatteryStatus battery_status(int percent,BatteryCharge charge) {
  return {percent>=0&&percent<=100?percent:-1,charge};
}
class BatteryMonitor {
  bool sampled=false;
  std::uint32_t last=0;
 public:
  static constexpr std::uint32_t interval_ms=5000;
  BatteryStatus status{};
  bool due(std::uint32_t now) const {
    return !sampled || std::uint32_t(now-last)>=interval_ms;
  }
  bool update(std::uint32_t now,int percent,BatteryCharge charge) {
    const auto next=battery_status(percent,charge);
    const bool changed=!sampled || next.percent!=status.percent || next.charge!=status.charge;
    status=next; last=now; sampled=true;
    return changed;
  }
};
