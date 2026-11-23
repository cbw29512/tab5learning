#pragma once
#include "ink_capture.h"

struct UiRegion {
  int x = 0;
  int y = 0;
  int w = 0;
  int h = 0;

  bool contains(int px, int py) const {
    return px >= x && py >= y && px < x + w && py < y + h;
  }
};

UiRegion ui_canvas();
UiRegion ui_clear_button();
UiRegion ui_export_button();
void ui_draw_screen();
void ui_draw_ink(const InkPoint& point, const InkPoint* previous);
void ui_status(const char* message);
