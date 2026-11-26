#ifdef POCKET_LEARNER_PREVIEW
#include <Arduino.h>
#include <M5Unified.h>
#include "learner_layout.h"
#include "math_practice.h"
#include "math_journal.h"
#include "preview_badges.h"
#include "award_notice.h"
#include "math_ink_ui.h"
#include "reading_preview_ui.h"
#include "reading_preview_playback.h"
#include "reading_estimated_highlight.h"
#include "m5_reading_speaker.h"
#if __has_include("preview_narration.generated.h")
#include "preview_narration.generated.h"
#define TAB5_PREVIEW_AUDIO 1
#endif
#include <cstdio>

namespace {
LearnerNavigation navigation;
MathPractice math;
MathJournal storage;
AwardNotice notice;
MathScratchpad scratch;
ReadingPreview reading;
#ifdef TAB5_PREVIEW_AUDIO
ReadingEstimatedHighlight reading_highlight;
bool reading_follow=false;
size_t reading_restart_word=0;
M5ReadingSpeaker reading_backend;
ReadingSpeakerOutput reading_output(reading_backend);
ReadingPreviewPlayback reading_audio(reading_output,narration_pcm,
  sizeof(narration_pcm)/sizeof(narration_pcm[0]),narration_rate,
  narration_cues,sizeof(narration_cues)/sizeof(narration_cues[0]));
static_assert(sizeof(narration_cues)/sizeof(narration_cues[0])==reading_words.size());
#endif

void render_reading() {
#ifdef TAB5_PREVIEW_AUDIO
  render_reading_preview(reading,reading_audio.status(),reading_audio.paused());
#else
  render_reading_preview(reading);
#endif
}
void reading_touch(int x,int y) {
  const int action=reading_audio_action(x,y);
#ifdef TAB5_PREVIEW_AUDIO
  if(action==2) {
    reading_audio.stop(); reading_highlight.stop(); reading_follow=false; reading.reset();
  } else if(action==1) {
    if(reading_audio.listen()) {
      reading_restart_word=0;
      reading.reset();
      reading_follow=reading_highlight.start(narration_cues,reading_words.size(),narration_duration,millis());
    }
  } else if(action==3) {
    if(reading_audio.paused()) {
      const auto offset=reading_audio.resume_offset();
      if(reading_audio.resume()) {
        reading_follow=reading_highlight.start(narration_cues,reading_words.size(),narration_duration,millis(),offset);
      }
    } else if(reading_follow && reading_audio.pause(reading_restart_word)) {
      reading_highlight.stop(); reading_follow=false;
      reading.selected=static_cast<int>(reading_restart_word);
    }
  }
  else if(!reading_audio.busy() && reading.select(x,y))
    reading_audio.replay(static_cast<size_t>(reading.selected));
#else
  if(!action) reading.select(x,y);
#endif
  render_reading();
}
void reading_leave() {
#ifdef TAB5_PREVIEW_AUDIO
  reading_audio.stop(); reading_highlight.stop(); reading_follow=false;
#endif
  reading.reset();
}
void reading_tick(bool visible) {
#ifdef TAB5_PREVIEW_AUDIO
  bool changed=reading_audio.poll();
  if(reading_follow) {
    const int next=reading_highlight.update(millis(),reading_output.state()==SpeakerOutputState::Queued);
    if(next>=0) reading_restart_word=static_cast<size_t>(next);
    if(next!=reading.selected) { reading.selected=next; changed=true; }
    if(!reading_audio.busy()) reading_follow=false;
  }
  if(changed && visible) render_reading();
#else
  (void)visible;
#endif
}
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

void render_notice() {
  M5.Display.fillRect(32,108,1216,28,TFT_NAVY);
  if(!notice.visible || navigation.help_open ||
      navigation.screen!=LearnerScreen::Math) return;
  M5.Display.fillRect(32,108,1216,28,TFT_DARKGREEN);
  M5.Display.setTextSize(2);
  M5.Display.setTextColor(TFT_WHITE,TFT_DARKGREEN);
  M5.Display.drawString(preview_badges[notice.sequence-1].name,48,112);
  M5.Display.drawString("Achievement unlocked!",400,112);
}

void render_journey() {
  M5.Display.setTextSize(3);
  M5.Display.setTextColor(TFT_WHITE,TFT_NAVY);
  if(storage_error) {
    M5.Display.drawString("Progress unavailable - ask your teacher.",48,180);
    return;
  }
  const auto count=preview_badge_count(math);
  for(unsigned i=0;i<preview_badges.size();++i) {
    char line[100];
    const auto& badge=preview_badges[i];
    snprintf(line,sizeof(line),"%s: %s (%u/%u)",i<count?"Completed":i==count?"Current":"Upcoming",
      badge.name,math.independent<badge.threshold?math.independent:badge.threshold,badge.threshold);
    M5.Display.drawString(line,48,180+80*i);
  }
  M5.Display.drawString("Practice badges - not mastery certificates.",48,400);
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
    M5.Display.drawString("Practice answers saved.",48,280);
    return;
  }
  const auto& q=math_questions[math.index];
  snprintf(line,sizeof(line),"Question %u of %u",unsigned(math.index+1),unsigned(math_questions.size()));
  M5.Display.drawString(line,48,144);
  M5.Display.setTextSize(2);
  M5.Display.drawString("Scratchpad",656,150);
  M5.Display.fillRect(1100,140,148,40,TFT_DARKGREY);
  M5.Display.setTextColor(TFT_WHITE,TFT_DARKGREY);
  M5.Display.drawString("Clear",1130,150);
  const auto& card=math_problem_card;
  M5.Display.fillRect(card.x,card.y,card.w,card.h,TFT_DARKGREY);
  M5.Display.setTextColor(TFT_WHITE,TFT_DARKGREY);
  math_ink_render(scratch);
  snprintf(line,sizeof(line),"%d + %d = ?",q.a,q.b);
  M5.Display.setTextSize(7);
  M5.Display.drawString(line,120,270);
  for(int i=0;i<4;++i) {
    const int x=32+i*308;
    M5.Display.fillRoundRect(x,444,292,96,18,TFT_BLUE);
    M5.Display.setTextColor(TFT_WHITE,TFT_BLUE);
    snprintf(line,sizeof(line),"%d",q.choices[i]);
    M5.Display.drawString(line,x+100,465);
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
      render_notice();
    } else if (navigation.screen==LearnerScreen::Reading) {
      render_reading();
    } else if (navigation.screen==LearnerScreen::Journey) {
      render_journey();
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
  // Physical Tab5 acceptance: default landscape was upside down on the stand.
  // Rotate by 180 degrees while keeping M5GFX's touch transformation.
  M5.Display.setRotation((M5.Display.getRotation()+2)%4);
  Serial.printf("DISPLAY_ROTATION,%u\n",unsigned(M5.Display.getRotation()));
  geometry_ok=M5.Display.width()==1280 && M5.Display.height()==720;
  if (!geometry_ok) {
    Serial.println("ERR,UNEXPECTED_DISPLAY_GEOMETRY");
    M5.Display.fillScreen(TFT_BLACK);
    M5.Display.setTextColor(TFT_WHITE,TFT_BLACK);
    M5.Display.drawString("Unsupported display geometry",10,10);
    return;
  }
  const auto loaded=storage.begin(math);
  storage_error=loaded==JournalLoad::Error;
  if(storage_error) Serial.println("ERR,MATH_STORAGE_LOAD");
  else if(loaded==JournalLoad::Restored) navigation.screen=LearnerScreen::Math;
  notice.newest=preview_badge_count(math); // Never replay restored awards.
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
        learner_navigate(navigation,action)) {
      if (navigation.screen!=LearnerScreen::Reading || navigation.help_open) reading_leave();
      render();
    } else if (!navigation.help_open && navigation.screen==LearnerScreen::Reading) {
      reading_touch(detail.x,detail.y);
    }
    else if (!navigation.help_open && navigation.screen==LearnerScreen::Math) {
      if(!storage_error && !math.finished) {
        if(scratch_clear_hit(detail.x,detail.y)) {
          scratch_clear(scratch); render();
        } else if(scratch_inside(detail.x,detail.y)) {
          math_ink_sample(scratch,detail.x,detail.y,true);
        }
      }
      int choice=math_choice_at(detail.x,detail.y);
      if(choice>=0 && !storage_error) {
        auto next=math;
        MathAttempt attempt;
        if(make_math_attempt(math,static_cast<unsigned>(choice),scratch,attempt)) {
          const auto result=attempt.result;
          if(storage.append(attempt,math,next)) {
            const auto before=preview_badge_count(math);
            math=next; feedback=result;
            if(result!=MathResult::TryAgain) scratch_clear(scratch);
            const auto after=preview_badge_count(math);
            if(after>before) award_notice_offer(notice,after,millis(),true);
          }
          else { storage_error=true; Serial.println("ERR,MATH_STORAGE_SAVE"); }
          render();
        }
      }
    }
  }
  if(geometry_ok && touching && was_touching && scratch.drawing &&
      !navigation.help_open && navigation.screen==LearnerScreen::Math &&
      !storage_error && !math.finished) {
    const auto detail=M5.Touch.getDetail(0);
    math_ink_sample(scratch,detail.x,detail.y,false);
  }
  if(!touching) scratch_release(scratch);
  const bool visible=notice.visible;
  award_notice_tick(notice,millis(),navigation.screen==LearnerScreen::Math && !navigation.help_open);
  if(visible!=notice.visible && navigation.screen==LearnerScreen::Math && !navigation.help_open) render_notice();
  reading_tick(geometry_ok && navigation.screen==LearnerScreen::Reading && !navigation.help_open);
  was_touching=touching;
  M5.delay(10);
}
#endif
