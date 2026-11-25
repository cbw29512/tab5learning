#pragma once
#include <array>
#include <cstddef>

struct ReadingPreviewWord {
  const char* text;
  int x, y, w;
  bool hit(int px, int py) const {
    return px >= x && px < x+w && py >= y && py < y+64;
  }
};

// Original practice passage. No audio or assessment is implied by selection.
inline constexpr std::array<ReadingPreviewWord, 10> reading_words{{
  {"A",64,220,64}, {"little",144,220,144}, {"fox",304,220,96},
  {"ran",416,220,96}, {"home.",528,220,144},
  {"It",64,310,64}, {"sat",144,310,96}, {"by",256,310,64},
  {"a",336,310,48}, {"tree.",400,310,144}
}};

struct ReadingPreview {
  int selected = -1;
  bool select(int x, int y) {
    for (std::size_t i=0; i<reading_words.size(); ++i) {
      if (!reading_words[i].hit(x,y)) continue;
      selected=static_cast<int>(i);
      return true;
    }
    return false;
  }
  void reset() { selected=-1; }
};
