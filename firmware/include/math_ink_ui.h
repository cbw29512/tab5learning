#pragma once
#include <M5Unified.h>
#include "math_scratchpad.h"

// Coordinates are shared with hit testing; no nested drawable panel.
inline void math_ink_point(const InkPoint& p,const InkPoint* prev) {
  if(p.stroke_start || !prev) M5.Display.drawPixel(p.x,p.y,TFT_BLACK);
  else M5.Display.drawLine(prev->x,prev->y,p.x,p.y,TFT_BLACK);
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
  } else if(s.ink.overflow) {
    Serial.println("ERR,SCRATCHPAD_FULL");
    M5.Display.setTextSize(2);
    M5.Display.setTextColor(TFT_WHITE,TFT_NAVY);
    M5.Display.drawString("Scratchpad full - tap Clear",660,552);
  }
}
