#include "usb_probe.h"
#include <Arduino.h>
#include <cstring>

namespace {
constexpr std::size_t kMaxCommand = 24;
char command[kMaxCommand]{};
std::size_t position = 0;
bool discarding = false;

void handle_command(const InkCapture& capture) {
  if (std::strcmp(command, "PING") == 0) {
    Serial.println("PONG,TAB5_INK_PROBE,1");
  } else if (std::strcmp(command, "STATUS") == 0) {
    Serial.printf("STATUS,1,%u,%u,%u\n",
                  static_cast<unsigned>(capture.count),
                  static_cast<unsigned>(ink_stroke_count(capture)),
                  capture.overflow ? 1U : 0U);
  } else if (std::strcmp(command, "EXPORT") == 0) {
    usb_probe_export(capture);
  } else {
    Serial.println("ERR,UNKNOWN_COMMAND");
  }
}
}  // namespace

void usb_probe_export(const InkCapture& capture) {
  Serial.printf("BEGIN,INK_V1,%u,%u\n",
                static_cast<unsigned>(capture.count),
                static_cast<unsigned>(ink_stroke_count(capture)));
  for (std::size_t i = 0; i < capture.count; ++i) {
    const InkPoint& point = capture.points[i];
    Serial.printf("PT,%u,%u,%u,%u\n", static_cast<unsigned>(i),
                  static_cast<unsigned>(point.x),
                  static_cast<unsigned>(point.y),
                  point.stroke_start ? 1U : 0U);
  }
  Serial.println(capture.overflow ? "END,TRUNCATED" : "END,OK");
}

void usb_probe_poll(const InkCapture& capture) {
  // Bound work per frame so a noisy serial connection cannot freeze drawing.
  for (int n = 0; n < 32 && Serial.available() > 0; ++n) {
    const int raw = Serial.read();
    if (raw < 0) break;
    const char c = static_cast<char>(raw);
    if (c == '\r') continue;
    if (c == '\n') {
      if (discarding) {
        Serial.println("ERR,COMMAND_TOO_LONG");
      } else if (position > 0) {
        command[position] = '\0';
        handle_command(capture);
      }
      position = 0;
      discarding = false;
      continue;
    }
    if (discarding) continue;
    if (position + 1 >= kMaxCommand) {
      discarding = true;
      continue;
    }
    command[position++] = c;
  }
}
