#ifdef POCKET_LEARNER_PREVIEW
#include <Arduino.h>
#include <M5Unified.h>
#include "learner_layout.h"
#include "math_practice.h"
#include "math_storage.h"
#include <cstdio>

namespace {
LearnerNavigation navigation;
MathPractice math;
MathStorage storage;
bool storage_error=false;
MathResult feedback=MathResult::Ignored;
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

void render_math() {
  char line[96];
  M5.Display.setTextColor(TFT_WHITE,TFT_NAVY);
  M5.Display.setTextSize(3);
  if(storage_error) {
    M5.Display.drawString("Unable to save or restore practice.",48,200);
    M5.Display.drawString("Ask your teacher to restart and check storage.",48,260);
    return;
  }
  if(math.finished) {
    snprintf(line,sizeof(line),"Practice complete: %u independent answers",math.independent);
    M5.Display.drawString(line,48,200);
    M5.Display.drawString("Practice summary saved.",48,280);
    return;
  }
  const auto& q=math_questions[math.index];
  snprintf(line,sizeof(line),"Question %u of %u",unsigned(math.index+1),unsigned(math_questions.size()));
  M5.Display.drawString(line,48,140);
  snprintf(line,sizeof(line),"%d + %d = ?",q.a,q.b);
  M5.Display.setTextSize(7);
  M5.Display.drawString(line,48,195);
  for(int i=0;i<4;++i) {
    const int x=32+i*308;
    M5.Display.fillRoundRect(x,300,292,240,18,TFT_BLUE);
    M5.Display.setTextColor(TFT_WHITE,TFT_BLUE);
    snprintf(line,sizeof(line),"%d",q.choices[i]);
    M5.Display.drawString(line,x+100,390);
  }
  M5.Display.setTextSize(2);
  M5.Display.setTextColor(TFT_WHITE,TFT_NAVY);
  if(feedback==MathResult::TryAgain) M5.Display.drawString("Try again - you can do it.",48,550);
  else if(feedback==MathResult::Correct) M5.Display.drawString("Correct! Keep going.",48,550);
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
    if (navigation.screen==LearnerScreen::Math) {
      render_math();
    } else if (navigation.screen==LearnerScreen::Journey) {
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
  const auto loaded=storage.begin(math);
  storage_error=loaded==MathLoad::Error;
  if(storage_error) Serial.println("ERR,MATH_STORAGE_LOAD");
  else if(loaded==MathLoad::Restored) navigation.screen=LearnerScreen::Math;
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
    else if (!navigation.help_open && navigation.screen==LearnerScreen::Math) {
      int choice=math_choice_at(detail.x,detail.y);
      if(choice>=0 && !storage_error) {
        auto next=math;
        const auto result=math_answer(next,static_cast<std::size_t>(choice));
        if(result!=MathResult::Ignored) {
          if(storage.save(next)) { math=next; feedback=result; }
          else { storage_error=true; Serial.println("ERR,MATH_STORAGE_SAVE"); }
          render();
        }
      }
    }
  }
  was_touching=touching;
  M5.delay(10);
}
#endif
