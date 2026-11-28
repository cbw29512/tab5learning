// Developer-only display diagnostic; never included in the learner image.
#ifdef POCKET_DISPLAY_RECOVERY_PROBE
#include <Arduino.h>
#include <M5Unified.h>

namespace {
uint8_t baseline_rotation = 1;
uint32_t last_switch = 0;
bool reversed = false;

void show_pattern() {
  const uint8_t rotation = reversed ? ((baseline_rotation + 2) % 4) : baseline_rotation;
  M5.Display.setRotation(rotation);
  M5.Display.fillScreen(reversed ? TFT_BLUE : TFT_DARKGREEN);
  M5.Display.setTextColor(TFT_WHITE);
  M5.Display.setTextSize(5);
  M5.Display.drawString(reversed ? "B - ROTATED" : "A - BASELINE", 80, 80);
  M5.Display.drawString("TOP LEFT", 20, 20);
  M5.Display.fillRect(20, M5.Display.height()-100, 80, 80, TFT_RED);
  M5.Display.fillRect(M5.Display.width()-100, 20, 80, 80, TFT_YELLOW);
  Serial.printf("DISPLAY_PROBE,%s,rotation=%u,width=%d,height=%d,brightness=%u\n",
    reversed ? "B" : "A", unsigned(M5.Display.getRotation()),
    int(M5.Display.width()), int(M5.Display.height()), unsigned(M5.Display.getBrightness()));
}
}

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  Serial.begin(115200);
  // Give the panel time to settle before changing orientation.
  delay(2000);
  if (M5.Display.width() < M5.Display.height()) M5.Display.setRotation(1);
  baseline_rotation = M5.Display.getRotation() & 3;
  M5.Display.setBrightness(128);
  show_pattern();
  last_switch = millis();
}

void loop() {
  M5.update();
  if (uint32_t(millis() - last_switch) >= 12000) {
    reversed = !reversed;
    show_pattern();
    last_switch = millis();
  }
  delay(20);
}
#endif
