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
// Conditional Home shortcut: shown only for a verified unfinished checkpoint.
inline constexpr LearnerTile learner_continue{32,580,592,108,LearnerAction::Curriculum,"Continue learning"};
inline constexpr LearnerTile learner_journey_compact{656,580,592,108,LearnerAction::Journey,"My Journey"};
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

// On choice activities only, move Back below all three enlarged answer rows.
inline constexpr LearnerTile learner_achievements{48,444,552,104,LearnerAction::Achievements,"My Achievements"};
inline constexpr LearnerTile learner_choice_back{32,632,1216,56,LearnerAction::Back,"Go back"};
inline const LearnerTile& learner_back_tile(const LearnerNavigation& state,bool large_answers){
  return !state.help_open&&state.screen==LearnerScreen::Curriculum&&large_answers?learner_choice_back:learner_back;
}
inline bool learner_hit(const LearnerNavigation& state,int x,int y,LearnerAction& action,bool can_continue=false,bool large_answers=false) {
  if (learner_home.hit(x,y)) { action=learner_home.action; return true; }
  if (learner_help.hit(x,y)||learner_storage_help.hit(x,y)) { action=LearnerAction::Help; return true; }
  if(!state.help_open&&state.screen==LearnerScreen::Journey&&learner_achievements.hit(x,y)){
    action=learner_achievements.action;return true;
  }
  if (state.help_open || state.screen != LearnerScreen::Home) {
    if (learner_back_tile(state,large_answers).hit(x,y)) { action=LearnerAction::Back; return true; }
    return false;
  }
  if(can_continue&&learner_tiles[4].hit(x,y)){
    if(learner_continue.hit(x,y)){action=learner_continue.action;return true;}
    if(learner_journey_compact.hit(x,y)){action=learner_journey_compact.action;return true;}
    return false; // gap is not a touch target
  }
  for (const auto& tile : learner_tiles) {
    if (tile.hit(x,y)) { action=tile.action; return true; }
  }
  return false;
}
