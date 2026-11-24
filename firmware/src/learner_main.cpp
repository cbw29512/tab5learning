#ifdef POCKET_LEARNER_PREVIEW
#include <Arduino.h>
#include <M5Unified.h>
#include "learner_layout.h"

namespace {
LearnerNavigation navigation;
bool was_touching=false;
bool geometry_ok=false;

void tile(const LearnerTile& t, uint32_t color) {
  M5.Display.fillRoundRect(t.x,t.y,t.w,t.h,18,color);
  M5.Display.setTextColor(TFT_WHITE,color);
  M5.Display.setTextSize(4);
  M5.Display.drawString(t.label,t.x+24,t.y+(t.h-32)/2);
}

const char* heading() {
  switch (navigation.screen) {
    case LearnerScreen::Math: return "Math";
    case LearnerScreen::Reading: return "Reading";
    case LearnerScreen::Spelling: return "Spelling";
    case LearnerScreen::Flashcards: return "Flashcards";
    case LearnerScreen::Journey: return "My Learning Journey";
    default: return "Pocket Learning";
  }
}

void render() {
  M5.Display.fillScreen(TFT_NAVY);
  tile(learner_home,TFT_DARKGREY);
  tile(learner_help,TFT_DARKGREY);
  M5.Display.setTextColor(TFT_WHITE,TFT_NAVY);
  M5.Display.setTextSize(4);
  M5.Display.drawString(heading(),260,40);
  if (navigation.help_open) {
    M5.Display.drawString("Choose a subject from Home.",48,180);
    M5.Display.drawString("Tap Go back to continue.",48,250);
    tile(learner_back,TFT_DARKGREY);
  } else if (navigation.screen==LearnerScreen::Home) {
    const uint32_t colors[]={TFT_BLUE,TFT_DARKGREEN,TFT_PURPLE,TFT_MAROON,TFT_DARKGREY};
    for (unsigned i=0;i<learner_tiles.size();++i) tile(learner_tiles[i],colors[i]);
  } else {
    M5.Display.setTextSize(3);
    if (navigation.screen==LearnerScreen::Journey) {
      M5.Display.drawString("Current | Completed | Upcoming",48,180);
      M5.Display.drawString("No learning records loaded in this preview.",48,260);
    } else {
      M5.Display.drawString("Lesson integration pending.",48,180);
    }
    tile(learner_back,TFT_DARKGREY);
  }
}
}

void setup() {
  auto cfg=M5.config();
  M5.begin(cfg);
  Serial.begin(115200);
  // Use the board library for orientation and touch transformation.
  if (M5.Display.width()<M5.Display.height()) M5.Display.setRotation(1);
  geometry_ok=M5.Display.width()==1280 && M5.Display.height()==720;
  if (!geometry_ok) {
    Serial.println("ERR,UNEXPECTED_DISPLAY_GEOMETRY");
    M5.Display.fillScreen(TFT_BLACK);
    M5.Display.setTextColor(TFT_WHITE,TFT_BLACK);
    M5.Display.drawString("Unsupported display geometry",10,10);
    return;
  }
  render();
  Serial.println("READY,POCKET_LEARNER_PREVIEW,1");
}

void loop() {
  M5.update();
  const bool touching=M5.Touch.getCount()>0;
  if (geometry_ok && touching && !was_touching) {
    auto detail=M5.Touch.getDetail(0);
    LearnerAction action;
    if (learner_hit(navigation,detail.x,detail.y,action) &&
        learner_navigate(navigation,action)) render();
  }
  was_touching=touching;
  M5.delay(10);
}
#endif
