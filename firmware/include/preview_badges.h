#pragma once
#include "math_checkpoint.h"

struct PreviewBadge { unsigned threshold; const char* name; };
// Only practice recognition, never mastery or certificates.
inline constexpr std::array<PreviewBadge,2> preview_badges{{
  {1,"First Steps"}, {5,"Number Explorer"}
}};
inline unsigned preview_badge_count(const MathPractice& saved) {
  if(!math_state_valid(saved)) return 0;
  unsigned count=0;
  for(const auto& badge:preview_badges)
    if(saved.independent>=badge.threshold) ++count;
  return count;
}
