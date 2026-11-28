#pragma once
#include <M5Unified.h>
#include "curriculum_visuals.h"
namespace curriculum_preview {
inline void draw_visual(const Visual& visual){
  for(std::size_t i=0;i<visual.count;++i){
    const auto& r=visual.shapes[i];
    if(r.radius)M5.Display.fillRoundRect(visual_x+r.x,visual_y+r.y,r.w,r.h,r.radius,r.color);
    else M5.Display.fillRect(visual_x+r.x,visual_y+r.y,r.w,r.h,r.color);
  }
}
}
