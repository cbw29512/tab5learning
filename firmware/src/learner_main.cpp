#ifdef POCKET_LEARNER_PREVIEW
#include <Arduino.h>
#include <M5Unified.h>
#include "learner_layout.h"
#include "learner_usb_identity.h"
#include "display_startup_refresh.h"
#include "learner_display_policy.h"
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
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "math_submission.h"
#include "learner_battery_ui.h"
#include "learner_storage_ui.h"
#include "battery_notice.h"
#include "curriculum_preview_ui.h"
#include "curriculum_resume_store.h"
#include "curriculum_transition.h"
#include "curriculum_journey_ui.h"
#ifdef POCKET_LEARNER_SD_EVAL
#include "learner_sd_owner.h"
#else
#include "learner_sd_capacity.h"
#endif

namespace {
LearnerNavigation navigation;
curriculum_preview::Preview curriculum;
curriculum_preview::ResumeStore curriculum_resume;
// Non-owning view; only a qualified, replayed SD owner may bind this.
curriculum_preview::CompletionLedger* curriculum_completion=nullptr;
#ifdef POCKET_LEARNER_SD_EVAL
curriculum_preview::LearnerSdOwner curriculum_sd;
#endif
MathPractice math;
MathJournal storage;
AwardNotice notice;
MathScratchpad scratch;
MathSubmission submission;
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
DisplayStartupRefresh startup_refresh;
DisplayDiagnostics display_diagnostics;
BatteryMonitor battery;
BatteryRuntime battery_runtime;
// Normal preview: a manual capacity snapshot only; no SD evidence writes.
learner_storage::Status sd_status{};

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
    case LearnerScreen::Curriculum:
      if(curriculum.screen==curriculum_preview::Screen::Subjects)return "Grade 1";
      if(curriculum.unit>=0)return curriculum_preview::units[curriculum.unit].title;
      return curriculum_preview::subjects[curriculum.subject];
    default: return "Pocket Learning";
  }
}

void render_notice() {
  const auto& region=learner_notice_region;
  M5.Display.fillRect(region.x,region.y,region.w,region.h,TFT_NAVY);
  const auto warning=battery_notice(battery.status,navigation,storage_error);
  if(warning!=BatteryNotice::None) {
    const auto color=warning==BatteryNotice::Low?TFT_YELLOW:TFT_RED;
    M5.Display.fillRect(region.x,region.y,region.w,region.h,color);
    M5.Display.setTextSize(2);
    M5.Display.setTextColor(TFT_BLACK,color);
    M5.Display.drawString(battery_notice_text(warning),48,112);
    return;
  }
  if(!notice.visible || navigation.help_open ||
      navigation.screen!=LearnerScreen::Math) return;
  M5.Display.fillRect(region.x,region.y,region.w,region.h,TFT_DARKGREEN);
  M5.Display.setTextSize(2);
  M5.Display.setTextColor(TFT_WHITE,TFT_DARKGREEN);
  M5.Display.drawString(preview_badges[notice.sequence-1].name,48,112);
  M5.Display.drawString("Achievement unlocked!",400,112);
}
void refresh_notice() {
  const auto& r=learner_notice_region;
  M5.Display.startWrite();
  render_notice();
  M5.Display.endWrite();
  M5.Display.display(r.x,r.y,r.w,r.h);
}

void render_journey() {
  M5.Display.setTextSize(3);
  M5.Display.setTextColor(TFT_WHITE,TFT_NAVY);
  curriculum_preview::render_subject_progress(curriculum_completion);
  M5.Display.setTextSize(2);M5.Display.setTextColor(TFT_WHITE,TFT_NAVY);
  if(storage_error) {
    M5.Display.drawString("Math progress unavailable.",48,180);
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
  curriculum_preview::browse_clamp(curriculum,curriculum_completion);
  // Submit the complete frame together and explicitly flush its framebuffer.
  M5.Display.startWrite();
  M5.Display.fillScreen(TFT_NAVY);
  tile(learner_home,TFT_DARKGREY);
  tile(learner_help,TFT_DARKGREY);
  M5.Display.setTextColor(TFT_WHITE,TFT_NAVY);
  M5.Display.setTextSize(learner_heading_size);
  M5.Display.drawString(heading(),learner_heading_x,40);
  learner_storage::render_header(sd_status);
  render_learner_battery(battery.status);
  render_notice();
  M5.Display.setTextSize(4);
  M5.Display.setTextColor(TFT_WHITE,TFT_NAVY);
  if (navigation.help_open) {
    M5.Display.drawString("Choose a subject from Home.",48,180);
    M5.Display.drawString("Tap Go back to continue.",48,250);
#ifndef POCKET_LEARNER_SD_EVAL
    const auto& check=learner_sd_check;
    M5.Display.fillRoundRect(check.x,check.y,check.w,check.h,12,TFT_BLUE);
    M5.Display.setTextColor(TFT_WHITE,TFT_BLUE);
    M5.Display.setTextSize(3);
    M5.Display.drawString(check.label,check.x+24,check.y+14);
#endif
    render_battery_runtime(battery_runtime);
    learner_storage::render_help(sd_status);
    tile(learner_back,TFT_DARKGREY);
  } else if (navigation.screen==LearnerScreen::Home) {
    const uint32_t colors[]={TFT_BLUE,TFT_DARKGREEN,TFT_PURPLE,TFT_MAROON,TFT_DARKGREY};
    for (unsigned i=0;i<learner_tiles.size();++i) tile(learner_tiles[i],colors[i]);
  } else {
    M5.Display.setTextSize(3);
    if (navigation.screen==LearnerScreen::Math) {
      render_math();
    } else if (navigation.screen==LearnerScreen::Reading) {
      render_reading();
    } else if (navigation.screen==LearnerScreen::Curriculum) {
      curriculum_preview::render(curriculum,curriculum_resume.available(),curriculum_completion);
    } else if (navigation.screen==LearnerScreen::Journey) {
      render_journey();
    } else {
      M5.Display.drawString("Lesson integration pending.",48,180);
    }
    tile(learner_back,TFT_DARKGREY);
  }
  M5.Display.endWrite();
  M5.Display.display(0,0,M5.Display.width(),M5.Display.height());
}
}

void setup() {
  auto cfg=M5.config();
  M5.begin(cfg);
  Serial.begin(115200);
  Serial.printf("BOOT,RESET_REASON,%d\n",int(esp_reset_reason()));
  // Match the physical display probe: let the panel settle before drawing.
  delay(2000);
  if (M5.Display.width()<M5.Display.height()) M5.Display.setRotation(1);
  M5.Display.setBrightness(128);
  // Probe B (rotation 3) was confirmed upright on the user's Tab5 stand.
  // Select it explicitly instead of toggling relative to an unknown default.
  M5.Display.setRotation(3);
  Serial.printf("DISPLAY_ROTATION,%u,brightness=%u\n",
      unsigned(M5.Display.getRotation()), unsigned(M5.Display.getBrightness()));
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
  // Restore math evidence without reopening a completed prototype. Home is
  // always the four-subject dashboard, including after a restart.
  curriculum_resume.begin(); // Separate bounded resume record; no math writes.
#ifdef POCKET_LEARNER_SD_EVAL
  if(curriculum_sd.begin(sd_status)) curriculum_completion=curriculum_sd.view();
#endif
  notice.newest=preview_badge_count(math); // Never replay restored awards.
  battery_runtime.observe(millis(),navigation);
  sample_learner_battery(battery,battery_runtime,millis());
  render();
  startup_refresh.begin(millis());
  Serial.println("READY,POCKET_LEARNER_PREVIEW,1");
}

void loop() {
  M5.update();
  battery_runtime.observe(millis(),navigation);
  learner_usb_identity_poll();
  const bool touching=M5.Touch.getCount()>0;
  // Stop retries immediately when a learner interacts; never overwrite ink.
  if (touching) startup_refresh.cancel();
  if (geometry_ok && startup_refresh.due(millis())) {
    M5.Display.setBrightness(128);
    render();
    Serial.printf("DISPLAY_STARTUP_REFRESH,%u\n", unsigned(startup_refresh.next));
  }
  if (geometry_ok && touching && !was_touching) {
    const auto brightness=M5.Display.getBrightness();
    Serial.printf("DISPLAY,TOUCH,BRIGHTNESS,%u,SCREEN,%u\n",unsigned(brightness),unsigned(navigation.screen));
    // A zero reported backlight is a concrete reason to restore the existing
    // brightness setting; never infer panel health from a nonzero reading.
    if(brightness==0){
      M5.Display.setBrightness(128);
      Serial.println("DISPLAY,BACKLIGHT_RESTORE");
      render();
    }
    auto detail=M5.Touch.getDetail(0);
    LearnerAction action;
#ifndef POCKET_LEARNER_SD_EVAL
    if(learner_sd_check_hit(navigation,detail.x,detail.y)) {
      Serial.println("SD,MANUAL_CHECK_REQUEST");
      sd_status=learner_storage::check_capacity();
      render();
    } else
#endif
    if (navigation.screen==LearnerScreen::Curriculum && !navigation.help_open &&
        curriculum.screen!=curriculum_preview::Screen::Units &&
        curriculum.screen!=curriculum_preview::Screen::Subjects &&
        (curriculum.grouped||curriculum.screen!=curriculum_preview::Screen::Lessons) &&
        learner_back.hit(detail.x,detail.y) && curriculum.back()) {
      render();
    } else if (learner_hit(navigation,detail.x,detail.y,action) &&
        learner_navigate(navigation,action)) {
      const auto subject=learner_subject(action);
      if(subject>=0) {
        curriculum.reset();
        curriculum.choose_subject_grouped(static_cast<unsigned>(subject));
      } else if(action==LearnerAction::Curriculum || navigation.screen==LearnerScreen::Home) {
        curriculum.reset();
        if(action==LearnerAction::Curriculum) curriculum_resume.restore(curriculum);
      }
      battery_runtime.observe(millis(),navigation);
      if (navigation.screen!=LearnerScreen::Reading || navigation.help_open) reading_leave();
      render();
    } else if (!navigation.help_open && navigation.screen==LearnerScreen::Curriculum) {
      // Stage navigation before acknowledging a new resume location. Feedback
      // is transient; a power loss repeats its activity, never awards credit.
      auto candidate=curriculum;
      if(curriculum_preview::touch(candidate,detail.x,detail.y,curriculum_completion)) {
        // Only resume the exact lesson selected, never redirect a subject or
        // reopen a finished lesson. Earlier math journal remains unchanged.
        if(curriculum.screen==curriculum_preview::Screen::Lessons &&
            candidate.screen==curriculum_preview::Screen::Activity&&!candidate.reviewing) {
          curriculum_preview::Preview saved;
          if(curriculum_resume.restore(saved) &&
              saved.screen==curriculum_preview::Screen::Activity &&
              saved.lesson==candidate.lesson) candidate.step=saved.step; // Retain selected unit and page.
        }
        // Completion backend remains explicitly disabled until the
        // isolated SD write and reboot test is physically accepted.
#ifdef POCKET_LEARNER_SD_EVAL
        // A failed SD owner is not permission to silently fall back to
        // resume-only completion. Navigation remains available.
        if(candidate.screen==curriculum_preview::Screen::Finished&&
            (!curriculum_completion||!curriculum_completion->available())){
          Serial.println("ERR,CURRICULUM_SD_REQUIRED");
        }else
#endif
        {
          const auto result=curriculum_preview::commit_transition(curriculum,candidate,curriculum_resume,curriculum_completion);
#ifdef POCKET_LEARNER_SD_EVAL
          if(result==curriculum_preview::TransitionResult::CompletionError)
            sd_status=learner_storage::unavailable();
#else
          (void)result;
#endif
        }
        render();
      }
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
        Serial.printf("MATH,SUBMIT,STACK_WATERMARK,%u\n",unsigned(uxTaskGetStackHighWaterMark(nullptr)));
        const auto before=preview_badge_count(math);
        const auto status=submission.submit(storage,math,scratch,static_cast<unsigned>(choice),feedback);
        if(status==MathSubmitStatus::Saved) {
          const auto after=preview_badge_count(math);
          if(after>before) award_notice_offer(notice,after,millis(),true);
          Serial.printf("MATH,SAVED,STACK_WATERMARK,%u\n",unsigned(uxTaskGetStackHighWaterMark(nullptr)));
          render();
        } else if(status==MathSubmitStatus::StorageError) {
          storage_error=true; Serial.println("ERR,MATH_STORAGE_SAVE"); render();
        } else {
          Serial.println("ERR,MATH_SUBMISSION_REJECTED");
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
  // Defer PMIC reads and regional flush while a finger is drawing or tapping.
  if(geometry_ok && !touching && battery.due(millis())) {
    const bool changed=sample_learner_battery(battery,battery_runtime,millis());
    // Home, curriculum, journey and Help use one full-frame commit instead
    // of independent battery/notice/runtime flushes. Preserve the legacy
    // reading and scratchpad incremental paths to avoid interrupting ink/audio.
    switch(display_refresh_policy(navigation,changed)){
      case DisplayRefresh::Full:
        Serial.printf("DISPLAY,FULL_REFRESH,BATTERY,%d,BRIGHTNESS,%u\n",battery.status.percent,unsigned(M5.Display.getBrightness()));
        render();break;
      case DisplayRefresh::Regions:
        refresh_learner_battery(battery.status);refresh_notice();break;
      case DisplayRefresh::None:break;
    }
    if(display_diagnostics.due(millis())){
      Serial.printf("DISPLAY,HEARTBEAT,SCREEN,%u,BRIGHTNESS,%u,STACK,%u\n",
        unsigned(navigation.screen),unsigned(M5.Display.getBrightness()),
        unsigned(uxTaskGetStackHighWaterMark(nullptr)));
    }
  }
  const bool visible=notice.visible;
  award_notice_tick(notice,millis(),navigation.screen==LearnerScreen::Math && !navigation.help_open);
  if(geometry_ok && visible!=notice.visible && navigation.screen==LearnerScreen::Math && !navigation.help_open) refresh_notice();
  reading_tick(geometry_ok && navigation.screen==LearnerScreen::Reading && !navigation.help_open);
  was_touching=touching;
  M5.delay(10);
}
#endif
