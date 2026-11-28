#pragma once
#include <M5Unified.h>
#include "reading_preview.h"

// Half-open touch bounds match the rendered controls.
inline int reading_audio_action(int x,int y) {
  if(y<400 || y>=448) return 0;
  if(x>=64 && x<264) return 1;
  if(x>=280 && x<480) return 2;
  if(x>=496 && x<696) return 3;
  return 0;
}

inline void render_reading_preview(const ReadingPreview& state, const char* audio_status="Audio not packaged", bool paused=false) {
  M5.Display.fillRoundRect(32,156,1216,384,18,TFT_DARKGREY);
  M5.Display.setTextColor(TFT_WHITE,TFT_DARKGREY);
  M5.Display.setTextSize(3);
  M5.Display.drawString("The little fox",64,176);
  M5.Display.setTextSize(4);
  for (std::size_t i=0;i<reading_words.size();++i) {
    const auto& word=reading_words[i];
    const bool selected=state.selected==static_cast<int>(i);
    const uint32_t bg=selected?TFT_YELLOW:TFT_DARKGREY;
    M5.Display.fillRoundRect(word.x,word.y,word.w,64,8,bg);
    M5.Display.setTextColor(selected?TFT_BLACK:TFT_WHITE,bg);
    M5.Display.drawString(word.text,word.x+4,word.y+12);
  }
  M5.Display.setTextSize(2);
  M5.Display.setTextColor(TFT_WHITE,TFT_DARKGREY);
  M5.Display.fillRoundRect(64,400,200,48,8,TFT_BLUE);
  M5.Display.fillRoundRect(280,400,200,48,8,TFT_MAROON);
  M5.Display.setTextColor(TFT_WHITE,TFT_BLUE);
  M5.Display.drawString("Listen",84,414);
  M5.Display.setTextColor(TFT_WHITE,TFT_MAROON);
  M5.Display.drawString("Stop",300,414);
  M5.Display.setTextColor(TFT_WHITE,TFT_DARKGREY);
  M5.Display.fillRoundRect(496,400,200,48,8,TFT_BLUE);
  M5.Display.setTextColor(TFT_WHITE,TFT_BLUE);
  M5.Display.drawString(paused?"Resume":"Pause",516,414);
  M5.Display.setTextColor(TFT_WHITE,TFT_DARKGREY);
  M5.Display.drawString(audio_status,716,414);
  M5.Display.drawString("Tap a word to replay it. Highlight timing estimated.",64,470);
}
