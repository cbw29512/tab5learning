#pragma once
#include <M5Unified.h>
#include "reading_preview.h"

inline void render_reading_preview(const ReadingPreview& state) {
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
  M5.Display.drawString("Tap a word to highlight it.",64,412);
  M5.Display.drawString("Read aloud at your own pace. Audio not connected yet.",64,454);
}
