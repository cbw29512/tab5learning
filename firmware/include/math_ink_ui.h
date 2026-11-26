#pragma once
#include <M5Unified.h>
#include "math_ink_brush.h"

// Live samples and redraws use the same clipped brush.
inline void math_ink_point(const InkPoint& p,const InkPoint* prev) {
  math_ink_brush(p,prev,[](int left,int right,int y) {
    M5.Display.drawLine(left,y,right,y,TFT_BLACK);
  });
}
inline void math_ink_render(const MathScratchpad& s) {
  const auto& c=math_ink_card;
  M5.Display.fillRect(c.x,c.y,c.w,c.h,TFT_WHITE);
  for(std::size_t i=0;i<s.ink.count;++i)
    math_ink_point(s.ink.points[i],i?&s.ink.points[i-1]:nullptr);
}
inline void math_ink_sample(MathScratchpad& s,int x,int y,bool start) {
  const auto before=s.ink.count;
  if(scratch_touch(s,x,y,start) && s.ink.count>before) {
    const auto i=s.ink.count-1;
    math_ink_point(s.ink.points[i],i?&s.ink.points[i-1]:nullptr);
    // The Tab5 panel uses a framebuffer. Flush the modified canvas explicitly,
    // just as the full learner renderer flushes a complete frame.
    const auto& c=math_ink_card;
    M5.Display.display(c.x,c.y,c.w,c.h);
    if(start) Serial.println("MATH,INK_STROKE_STARTED");
  } else if(s.ink.overflow) {
    Serial.println("ERR,SCRATCHPAD_FULL");
    M5.Display.setTextSize(2);
    M5.Display.setTextColor(TFT_WHITE,TFT_NAVY);
    M5.Display.drawString("Scratchpad full - tap Clear",660,552);
    M5.Display.display(656,548,592,30);
  }
}
