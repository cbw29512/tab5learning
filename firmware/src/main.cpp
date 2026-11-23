#include <Arduino.h>
#include <M5Unified.h>
#include "ink_capture.h"
#include "device_ui.h"
#include "usb_probe.h"

namespace {
InkCapture ink;
bool prior_touch = false;
bool drawing = false;

void add_touch_point(int x, int y, bool start) {
  if (x < 0 || y < 0 || x > 65535 || y > 65535) {
    Serial.println("ERR,INVALID_TOUCH_COORDINATE");
    return;
  }
  const std::size_t before = ink.count;
  if (!ink_append(ink, static_cast<std::uint16_t>(x),
                  static_cast<std::uint16_t>(y), start)) {
    ui_status("Ink buffer full - press CLEAR");
    Serial.println("ERR,INK_BUFFER_FULL");
    drawing = false;
    return;
  }
  if (ink.count != before) {
    const InkPoint* previous =
        ink.count >= 2 ? &ink.points[ink.count - 2] : nullptr;
    ui_draw_ink(ink.points[ink.count - 1], previous);
  }
}

void begin_touch(int x, int y) {
  if (ui_clear_button().contains(x, y)) {
    ink_clear(ink);
    ui_draw_screen();
    Serial.println("INFO,INK_CLEARED");
  } else if (ui_export_button().contains(x, y)) {
    usb_probe_export(ink);
    ui_status("Touch samples sent over USB serial");
  } else if (ui_canvas().contains(x, y)) {
    drawing = true;
    add_touch_point(x, y, true);
  }
}
}  // namespace

void setup() {
  // M5Unified selects the display/touch driver for supported Tab5 revisions.
  M5.begin();
  Serial.begin(115200);
  ink_clear(ink);
  ui_draw_screen();
  Serial.println("READY,TAB5_INK_PROBE,1");
}

void loop() {
  M5.update();
  const bool touching = M5.Touch.getCount() > 0;
  if (touching) {
    const auto detail = M5.Touch.getDetail(0);
    const int x = detail.x;
    const int y = detail.y;
    if (!prior_touch) {
      begin_touch(x, y);
    } else if (drawing) {
      if (ui_canvas().contains(x, y)) {
        add_touch_point(x, y, false);
      } else {
        drawing = false;  // A stroke cannot continue into a control button.
      }
    }
  } else {
    drawing = false;
  }
  prior_touch = touching;
  usb_probe_poll(ink);
  M5.delay(10);
}
