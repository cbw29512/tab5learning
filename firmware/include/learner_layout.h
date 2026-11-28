#pragma once
#include "learner_navigation.h"
#include <array>

struct LearnerTile {
  int x, y, w, h;
  LearnerAction action;
  const char* label;
  bool hit(int px, int py) const {
    return px >= x && py >= y && px < x+w && py < y+h;
  }
};

// Fixed landscape reference coordinates; touch and render share this layout.
inline constexpr std::array<LearnerTile, 5> learner_tiles{{
  {32,156,592,192,LearnerAction::SubjectMath,"Mathematics"},
  {656,156,592,192,LearnerAction::SubjectEla,"Reading and writing"},
  {32,368,592,192,LearnerAction::SubjectScience,"Science"},
  {656,368,592,192,LearnerAction::SubjectSocial,"Social studies"},
  {32,580,1216,108,LearnerAction::Journey,"My Learning Journey"}
}};
inline constexpr LearnerTile learner_home{32,24,180,80,LearnerAction::Home,"Home"};
inline constexpr LearnerTile learner_help{1068,24,180,80,LearnerAction::Help,"Help"};
// Header regions are outside the navigation buttons and lesson cards.
struct LearnerRegion { int x,y,w,h; };
// Keep a 12 px gap between the storage and existing battery indicators.
inline constexpr LearnerRegion learner_storage_region{636,24,172,80};
// This shortcut opens Help only. It must never mount a card or write data.
inline constexpr LearnerTile learner_storage_help{learner_storage_region.x,learner_storage_region.y,
  learner_storage_region.w,learner_storage_region.h,LearnerAction::Help,"SD details"};
inline constexpr LearnerTile learner_sd_check{48,280,1184,52,LearnerAction::Help,"Check backed-up SD card"};
inline bool learner_sd_check_hit(const LearnerNavigation& state,int x,int y){
  return state.help_open&&learner_sd_check.hit(x,y);
}
inline constexpr int learner_heading_x=228;
inline constexpr unsigned learner_heading_size=3;
inline constexpr LearnerRegion learner_battery_region{820,24,236,80};
inline constexpr LearnerRegion learner_notice_region{32,108,1216,28};
inline constexpr LearnerTile learner_back{32,580,1216,108,LearnerAction::Back,"Go back"};

inline bool learner_hit(const LearnerNavigation& state,int x,int y,LearnerAction& action) {
  if (learner_home.hit(x,y)) { action=learner_home.action; return true; }
  if (learner_help.hit(x,y)||learner_storage_help.hit(x,y)) { action=LearnerAction::Help; return true; }
  if (state.help_open || state.screen != LearnerScreen::Home) {
    if (learner_back.hit(x,y)) { action=learner_back.action; return true; }
    return false;
  }
  for (const auto& tile : learner_tiles) {
    if (tile.hit(x,y)) { action=tile.action; return true; }
  }
  return false;
}
