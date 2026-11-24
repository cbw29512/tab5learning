#pragma once
#include "ink_capture.h"

// Shared outer card geometry: equal dimensions, fixed regardless of content.
struct MathCard {
  int x,y,w,h;
  constexpr bool contains(int px,int py) const {
    return px>=x && px<x+w && py>=y && py<y+h;
  }
};
inline constexpr MathCard math_problem_card{32,150,592,134};
inline constexpr MathCard math_ink_card{656,150,592,134};
static_assert(math_problem_card.w==math_ink_card.w &&
              math_problem_card.h==math_ink_card.h,"Cards must match");
struct MathScratchpad {
  InkCapture ink;
  bool drawing=false;
};
inline bool scratch_inside(int x,int y) {
  return math_ink_card.contains(x,y);
}
inline bool scratch_clear_hit(int x,int y) {
  return x>=1100 && x<1248 && y>=108 && y<146;
}
// A stroke that leaves the canvas cannot resume until the finger lifts.
inline bool scratch_touch(MathScratchpad& s,int x,int y,bool start) {
  if(start) s.drawing=scratch_inside(x,y);
  if(!s.drawing || !scratch_inside(x,y)) { s.drawing=false; return false; }
  if(!ink_append(s.ink,static_cast<std::uint16_t>(x),static_cast<std::uint16_t>(y),start)) {
    s.drawing=false; return false;
  }
  return true;
}
inline void scratch_release(MathScratchpad& s) { s.drawing=false; }
inline void scratch_clear(MathScratchpad& s) { ink_clear(s.ink); s.drawing=false; }
