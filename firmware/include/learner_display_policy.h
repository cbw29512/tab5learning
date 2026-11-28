#pragma once
#include <cstdint>
#include "learner_navigation.h"

// Display scheduling only. No power management, storage or input mutation.
// Called only while no finger is touching the screen.
enum class DisplayRefresh { None, Full, Regions };
inline DisplayRefresh display_refresh_policy(const LearnerNavigation& nav,bool battery_changed){
  if(nav.help_open)return DisplayRefresh::Full; // live elapsed-time information
  if(!battery_changed)return DisplayRefresh::None;
  if(nav.screen==LearnerScreen::Math||nav.screen==LearnerScreen::Reading)
    return DisplayRefresh::Regions;
  return DisplayRefresh::Full;
}
struct DisplayDiagnostics {
  std::uint32_t last=0;
  bool sampled=false;
  static constexpr std::uint32_t interval_ms=30000;
  bool due(std::uint32_t now){
    if(sampled&&std::uint32_t(now-last)<interval_ms)return false;
    sampled=true;last=now;return true;
  }
};
