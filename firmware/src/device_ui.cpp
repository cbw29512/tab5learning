#include "device_ui.h"
#include <M5Unified.h>

// Screen areas derive from detected orientation; no board-specific touch pins.
UiRegion ui_canvas() {
  return {20, 135, M5.Display.width() - 290, M5.Display.height() - 190};
}

UiRegion ui_clear_button() {
  return {M5.Display.width() - 250, 160, 225, 85};
}

UiRegion ui_export_button() {
  return {M5.Display.width() - 250, 285, 225, 85};
}

void ui_status(const char* message) {
  const int y = M5.Display.height() - 40;
  M5.Display.fillRect(0, y, M5.Display.width(), 40, TFT_WHITE);
  M5.Display.setTextSize(2);
  M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
  M5.Display.drawString(message, 22, y + 6);
}

void ui_draw_screen() {
  const UiRegion canvas = ui_canvas();
  const UiRegion clear = ui_clear_button();
  const UiRegion export_button = ui_export_button();
  M5.Display.fillScreen(TFT_WHITE);
  M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
  M5.Display.setTextSize(3);
  M5.Display.drawString("TAB5 LEARNING - INK PROBE", 22, 18);
  M5.Display.setTextSize(4);
  M5.Display.drawString("7 + 5 = ?", 22, 65);
  M5.Display.drawRoundRect(canvas.x, canvas.y, canvas.w, canvas.h,
                           15, TFT_DARKGREY);
  M5.Display.setTextSize(2);
  M5.Display.drawString("DRAW 12 WITH YOUR FINGER", canvas.x + 18,
                        canvas.y + 18);

  M5.Display.fillRoundRect(clear.x, clear.y, clear.w, clear.h, 12, TFT_NAVY);
  M5.Display.setTextColor(TFT_WHITE, TFT_NAVY);
  M5.Display.setTextSize(3);
  M5.Display.drawString("CLEAR", clear.x + 45, clear.y + 25);
  M5.Display.fillRoundRect(export_button.x, export_button.y,
                           export_button.w, export_button.h, 12, TFT_NAVY);
  M5.Display.drawString("EXPORT", export_button.x + 35,
                        export_button.y + 25);
  ui_status("USB diagnostic only - no grade is assigned");
}

void ui_draw_ink(const InkPoint& point, const InkPoint* previous) {
  // Only connect points within the same stroke; lifted fingers start new lines.
  if (previous && !point.stroke_start) {
    M5.Display.drawLine(previous->x, previous->y, point.x, point.y, TFT_BLUE);
  }
  M5.Display.fillCircle(point.x, point.y, 5, TFT_BLUE);
}
