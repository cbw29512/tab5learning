#pragma once
#include "curriculum_preview.h"
namespace curriculum_preview {
// Presentation only: visiting an activity is not proof of correctness or mastery.
// The filled segments describe position in this lesson, not earned grades.
struct LessonProgress {
  unsigned total=0,visited=0;
  bool valid=false;
};
inline LessonProgress activity_progress(const Preview& p){
  if(p.screen!=Screen::Activity&&p.screen!=Screen::Feedback&&p.screen!=Screen::Finished)return {};
  if(p.lesson>=lesson_count||p.step>=p.current().count)return {};
  return {static_cast<unsigned>(p.current().count),static_cast<unsigned>(p.step+1),true};
}
inline constexpr unsigned progress_bar_width=300;
inline unsigned activity_bar_width(const LessonProgress& progress){
  if(!progress.valid||!progress.total||progress.visited>progress.total)return 0;
  return progress_bar_width*progress.visited/progress.total;
}
enum class FeedbackTone { Guidance, Correct, Learn, Invalid };
inline FeedbackTone feedback_tone(const Preview& p){
  if(p.screen!=Screen::Feedback||p.lesson>=lesson_count||p.step>=p.current().count)return FeedbackTone::Invalid;
  const auto& a=p.activity();
  if(a.kind==Kind::AdultReview)return FeedbackTone::Guidance;
  if(a.kind!=Kind::Choice||p.selected<0||static_cast<unsigned>(p.selected)>=a.count||
     a.answer<0||static_cast<unsigned>(a.answer)>=a.count)return FeedbackTone::Invalid;
  return p.selected==a.answer?FeedbackTone::Correct:FeedbackTone::Learn;
}
}
