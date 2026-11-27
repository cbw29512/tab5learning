#pragma once
#include <M5Unified.h>
#include "learner_layout.h"
#include "learner_storage_format.h"

namespace learner_storage {
inline void render_header(const Status& s){
  const auto& r=learner_storage_region;
  const auto color=s.state==State::Normal?TFT_GREEN:
    s.state==State::Low?TFT_YELLOW:s.state==State::Critical?TFT_RED:TFT_LIGHTGREY;
  M5.Display.fillRect(r.x,r.y,r.w,r.h,TFT_NAVY);
  char text[16];format_header_percent(s,text,sizeof(text));
  M5.Display.setTextColor(color,TFT_NAVY);
  M5.Display.setTextSize(3);M5.Display.drawString(text,r.x+4,r.y+5);
  M5.Display.setTextSize(2);M5.Display.drawString(header_label(s.state),r.x+4,r.y+42);
}
inline void render_help(const Status& s){
  M5.Display.setTextSize(2);M5.Display.setTextColor(TFT_WHITE,TFT_NAVY);
  char line[128];format_summary(s,line,sizeof(line));
  M5.Display.drawString(line,48,510);
  if(s.known()){
    char used[48];format_capacity(s.used,used,sizeof(used));
    std::snprintf(line,sizeof(line),"SD used: %s | %s",used,state_label(s.state));
    M5.Display.drawString(line,48,540);
  }
}
}
