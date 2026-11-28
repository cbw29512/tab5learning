#pragma once
#include <M5Unified.h>
#include "curriculum_feedback.h"
namespace curriculum_preview {
// Fixed regions are disjoint from the activity label, picture marker,
// prompt, art and the larger answer controls. No animation, timers or writes.
inline void render_activity_progress(const Preview& p){
  const auto progress=activity_progress(p);
  if(!progress.valid)return;
  M5.Display.fillRoundRect(650,220,progress_bar_width,12,6,TFT_DARKGREY);
  const auto width=activity_bar_width(progress);
  if(width>=12)M5.Display.fillRoundRect(650,220,width,12,6,TFT_GREEN);
  else if(width)M5.Display.fillRect(650,220,width,12,TFT_GREEN);
}
inline void render_feedback_heading(const Preview& p,int x){
  const auto tone=feedback_tone(p);
  const char* label="Ask an adult for help";
  uint32_t color=TFT_WHITE;
  switch(tone){
    case FeedbackTone::Correct:label="Great job!";color=TFT_GREEN;break;
    case FeedbackTone::Learn:label="Keep learning!";color=TFT_YELLOW;break;
    case FeedbackTone::Guidance:label="Work with an adult";break;
    default:break;
  }
  M5.Display.setTextSize(3);M5.Display.setTextColor(color,TFT_NAVY);
  M5.Display.drawString(label,x,250);
  M5.Display.setTextColor(TFT_WHITE,TFT_NAVY);
}
inline void render_correct_answer(const Preview& p){
  const auto tone=feedback_tone(p);
  if(tone!=FeedbackTone::Learn)return;
  // Current catalog labels fit within the full-width card at size 2.
  const auto& a=p.activity();
  M5.Display.fillRoundRect(48,448,1184,32,6,TFT_DARKGREY);
  M5.Display.setTextSize(2);M5.Display.setTextColor(TFT_WHITE,TFT_DARKGREY);
  M5.Display.drawString("Answer:",64,456);
  M5.Display.drawString(a.choices[a.answer],172,456);
}
inline void render_finish_heading(bool review){
  M5.Display.fillRoundRect(48,224,1184,92,16,TFT_DARKGREEN);
  M5.Display.setTextSize(4);M5.Display.setTextColor(TFT_WHITE,TFT_DARKGREEN);
  M5.Display.drawString(review?"Review finished!":"You finished the lesson!",76,250);
  M5.Display.setTextSize(3);M5.Display.setTextColor(TFT_WHITE,TFT_NAVY);
}
}
