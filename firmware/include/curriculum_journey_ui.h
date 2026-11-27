#pragma once
#include <M5Unified.h>
#include "curriculum_progress.h"

namespace curriculum_preview {
// Right half of My Learning Journey, independent of math badges.
// No touch controls or disk reads during rendering.
inline void render_subject_progress(const CompletionLedger* ledger){
  M5.Display.setTextSize(3);M5.Display.setTextColor(TFT_WHITE,TFT_NAVY);
  M5.Display.drawString("Grade 1 lesson progress",656,158);
  for(unsigned s=0;s<4;++s){
    const auto p=subject_progress(ledger,s);
    const int y=208+76*static_cast<int>(s);
    M5.Display.setTextSize(2);M5.Display.setTextColor(TFT_WHITE,TFT_NAVY);
    M5.Display.drawString(subjects[s],656,y);
    char label[64];
    if(p.known)std::snprintf(label,sizeof(label),"%u of %u completed",p.completed,p.total);
    else std::snprintf(label,sizeof(label),"-- of %u (not checked)",p.total);
    M5.Display.drawString(label,656,y+22);
    M5.Display.fillRect(656,y+46,552,10,TFT_DARKGREY);
    if(p.known&&p.total){
      const auto width=552u*p.completed/p.total;
      if(width)M5.Display.fillRect(656,y+46,width,10,TFT_GREEN);
    }
  }
  M5.Display.setTextSize(2);M5.Display.setTextColor(TFT_WHITE,TFT_NAVY);
  M5.Display.drawString("Lesson completion is not mastery.",656,530);
}
}
