#pragma once
#include <M5Unified.h>
#include "curriculum_visuals.h"
namespace curriculum_preview {
// Integer scale keeps the thumbnail inside its tile and uses the same art.
inline void draw_visual_at(const Visual& visual,int x,int y,int divisor){
  if(divisor<=0)return;
  for(std::size_t i=0;i<visual.count;++i){
    const auto& r=visual.shapes[i];
    const int w=(r.x+r.w)/divisor-r.x/divisor;
    const int h=(r.y+r.h)/divisor-r.y/divisor;
    if(w<=0||h<=0)continue;
    const int radius=r.radius/divisor;
    if(radius>0&&2*radius<=w&&2*radius<=h)
      M5.Display.fillRoundRect(x+r.x/divisor,y+r.y/divisor,w,h,radius,r.color);
    else M5.Display.fillRect(x+r.x/divisor,y+r.y/divisor,w,h,r.color);
  }
}
inline void draw_visual(const Visual& visual){draw_visual_at(visual,visual_x,visual_y,1);}
}
