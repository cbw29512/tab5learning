#pragma once
#include <array>
#include <cstdint>
#include "battery_monitor.h"
#include "learner_navigation.h"

// Preview diagnostics only: aggregate session time, no lesson content,
// no student identifier, no persistent writes. Not a remaining-life estimate.
class BatteryRuntime {
  bool started=false, sampled=false;
  std::uint32_t tick=0, sample_tick=0;
  BatteryStatus prior{};
  LearnerScreen screen=LearnerScreen::Home;
  bool help=false;
  std::array<std::uint64_t,7> elapsed{};
  std::uint64_t discharge=0;
  static unsigned slot(LearnerScreen s,bool overlay) {
    if(overlay) return 6;
    switch(s) {
      case LearnerScreen::Home: return 0;
      case LearnerScreen::Math: return 1;
      case LearnerScreen::Reading: return 2;
      case LearnerScreen::Spelling: return 3;
      case LearnerScreen::Flashcards: return 4;
      case LearnerScreen::Journey: return 5;
    }
    return 0;
  }
 public:
  void observe(std::uint32_t now,const LearnerNavigation& nav) {
    if(started) elapsed[slot(screen,help)]+=std::uint32_t(now-tick);
    started=true; tick=now; screen=nav.screen; help=nav.help_open;
  }
  void sample(std::uint32_t now,BatteryStatus value) {
    // Only credit bounded intervals with explicit discharge at both ends.
    // Unknown, charging, idle and large observation gaps are not counted.
    const auto delta=std::uint32_t(now-sample_tick);
    if(sampled && delta<=15000 && prior.percent>=0 && value.percent>=0 &&
       prior.charge==BatteryCharge::Discharging && value.charge==BatteryCharge::Discharging)
      discharge+=delta;
    prior=value; sample_tick=now; sampled=true;
  }
  std::uint64_t total_ms() const {
    std::uint64_t total=0;
    for(auto value:elapsed) total+=value;
    return total;
  }
  std::uint64_t screen_ms(LearnerScreen s) const { return elapsed[slot(s,false)]; }
  std::uint64_t discharge_ms() const { return discharge; }
};
