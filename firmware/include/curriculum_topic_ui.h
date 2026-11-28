#pragma once
#include <M5Unified.h>
#include <cstdio>
#include "curriculum_progress.h"
namespace curriculum_preview {
// Keep the lower progress line away from the existing picture thumbnail.
inline void render_topic_tile(const Unit& u,int y,const CompletionLedger* ledger){
  const auto p=topic_progress(ledger,static_cast<unsigned>(&u-units));
  const uint32_t color=topic_finished(p)?TFT_DARKGREEN:TFT_BLUE;
  M5.Display.fillRoundRect(48,y,1184,72,12,color);
  M5.Display.setTextSize(3);M5.Display.setTextColor(TFT_WHITE,color);
  M5.Display.drawString(u.title,72,y+8);
  char label[64];
  if(!p.known)std::snprintf(label,sizeof(label),"%u lessons - history unavailable",p.total);
  else if(topic_finished(p))std::snprintf(label,sizeof(label),"Topic finished! %u of %u lessons",p.completed,p.total);
  else std::snprintf(label,sizeof(label),"%u of %u lessons finished",p.completed,p.total);
  M5.Display.setTextSize(2);M5.Display.drawString(label,72,y+42);
  if(p.known&&p.total){
    constexpr unsigned width=168;
    M5.Display.fillRect(836,y+49,width,9,TFT_DARKGREY);
    const auto filled=width*p.completed/p.total;
    if(filled)M5.Display.fillRect(836,y+49,filled,9,TFT_GREEN);
  }
}
inline void render_topic_milestones(const CompletionLedger* ledger){
  const auto p=topic_milestones(ledger);
  char label[64];
  if(p.known)std::snprintf(label,sizeof(label),"Topics finished: %u of %u",p.completed,p.total);
  else std::snprintf(label,sizeof(label),"Topics finished: -- of %u",p.total);
  M5.Display.setTextSize(2);M5.Display.setTextColor(TFT_WHITE,TFT_NAVY);
  M5.Display.drawString(label,656,518);
  M5.Display.drawString("Completion is not mastery.",656,544);
}
}
