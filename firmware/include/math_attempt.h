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
  // Validate every point before touching the caller-owned output. Avoid a
  // second 3 KiB automatic MathAttempt on the embedded loop task stack.
  for(std::size_t i=0;i<scratch.ink.count;++i) {
    const auto& p=scratch.ink.points[i];
    if(!scratch_inside(p.x,p.y)||(!i&&!p.stroke_start)) return false;
  }
  auto next=before;
  const auto result=math_answer(next,choice);
  MathCheckpoint after{};
  if(result==MathResult::Ignored||!math_encode(next,after)) return false;
  out.sequence=next.attempts;
  out.question=static_cast<std::uint8_t>(before.index);
  out.choice=static_cast<std::uint8_t>(choice);
  out.result=result; out.after=after;
  out.ink.count=scratch.ink.count; out.ink.overflow=false;
  for(std::size_t i=0;i<out.ink.count;++i) {
    const auto& p=scratch.ink.points[i];
    out.ink.points[i]={static_cast<std::uint16_t>(p.x-math_ink_card.x),
                       static_cast<std::uint16_t>(p.y-math_ink_card.y),p.stroke_start};
  }
  // Preserve the previous zero-initialized output contract for unused points.
  for(std::size_t i=out.ink.count;i<kMaxInkPoints;++i) out.ink.points[i]={};
  return true;
}
