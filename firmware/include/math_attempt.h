#pragma once
#include "math_checkpoint.h"
#include "math_scratchpad.h"

// Bounded synthetic attempt record. No names or unrelated touch events.
// Coordinates are normalized to the original canvas, not the whole display.
struct MathAttempt {
  std::uint32_t sequence=0;
  std::uint8_t question=0, choice=0;
  MathResult result=MathResult::Ignored;
  MathCheckpoint after{};
  InkCapture ink{};
};
inline bool make_math_attempt(const MathPractice& before,unsigned choice,
                              const MathScratchpad& scratch,MathAttempt& out) {
  if(!math_state_valid(before)||before.finished||choice>=4||
     before.attempts==UINT32_MAX||scratch.ink.overflow||
     scratch.ink.count>kMaxInkPoints) return false;
  MathAttempt a;
  auto next=before;
  a.result=math_answer(next,choice);
  if(a.result==MathResult::Ignored||!math_encode(next,a.after)) return false;
  a.sequence=next.attempts;
  a.question=static_cast<std::uint8_t>(before.index);
  a.choice=static_cast<std::uint8_t>(choice);
  a.ink.count=scratch.ink.count;
  for(std::size_t i=0;i<a.ink.count;++i) {
    const auto& p=scratch.ink.points[i];
    if(!scratch_inside(p.x,p.y)||(!i&&!p.stroke_start)) return false;
    a.ink.points[i]={static_cast<std::uint16_t>(p.x-math_ink_card.x),
                     static_cast<std::uint16_t>(p.y-math_ink_card.y),p.stroke_start};
  }
  out=a;
  return true;
}
