#pragma once
#include <M5Unified.h>
#include <cstdio>
#include "curriculum_achievements.h"
namespace curriculum_preview {
inline void render_achievements(const CompletionLedger* ledger){
  const auto lessons_done=grade_progress(ledger);
  const auto topics_done=topic_milestones(ledger);
  M5.Display.setTextSize(2);M5.Display.setTextColor(TFT_WHITE,TFT_NAVY);
  M5.Display.drawString("Learning milestones - not grades or mastery certificates",48,144);
  unsigned earned=0;
  for(unsigned i=0;i<achievements.size();++i){
    const auto& def=achievements[i];
    const auto p=achievement_progress(def,lessons_done,topics_done);
    const bool known=p.state!=AchievementState::Unknown;
    const bool complete=p.state==AchievementState::Earned;
    if(complete)++earned;
    const int x=48+604*static_cast<int>(i%2);
    const int y=180+112*static_cast<int>(i/2);
    const uint32_t color=complete?TFT_DARKGREEN:TFT_DARKGREY;
    M5.Display.fillRoundRect(x,y,580,100,12,color);
    M5.Display.setTextSize(3);M5.Display.setTextColor(TFT_WHITE,color);
    M5.Display.drawString(def.name,x+16,y+12);
    char label[64];
    if(!known)std::snprintf(label,sizeof(label),"History unavailable");
    else if(complete)std::snprintf(label,sizeof(label),"Earned! %u of %u",p.completed,p.target);
    else std::snprintf(label,sizeof(label),"%u of %u - keep learning",p.completed,p.target);
    M5.Display.setTextSize(2);M5.Display.drawString(label,x+16,y+46);
    if(known&&p.target){
      M5.Display.fillRect(x+16,y+78,548,8,TFT_NAVY);
      const auto width=548u*p.completed/p.target;
      if(width)M5.Display.fillRect(x+16,y+78,width,8,TFT_GREEN);
    }
  }
  char summary[64];
  if(lessons_done.known&&topics_done.known)
    std::snprintf(summary,sizeof(summary),"%u of %u milestones earned",earned,static_cast<unsigned>(achievements.size()));
  else std::snprintf(summary,sizeof(summary),"Milestones need verified lesson history.");
  M5.Display.setTextColor(TFT_WHITE,TFT_NAVY);M5.Display.setTextSize(2);
  M5.Display.drawString(summary,48,530);
}
}
