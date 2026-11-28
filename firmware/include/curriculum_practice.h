#pragma once
#include "curriculum_session_evidence.h"
namespace curriculum_preview {
// Only the existing remediation and review suffix, never a new assessment.
// All 127 current lessons use these phases; fail closed for future catalogs.
inline bool practice_suffix(const Lesson& l){
  return l.count>=2&&std::strcmp(l.activities[l.count-2].phase,"remediation")==0&&
    std::strcmp(l.activities[l.count-1].phase,"review")==0;
}
inline bool practice_available(const Preview& p,const SessionEvidence* evidence){
  return evidence&&p.screen==Screen::Finished&&!p.reviewing&&!p.practice_only&&
    p.lesson<lesson_count&&practice_suffix(p.current())&&
    practice_hint(evidence->skill(p.lesson))==PracticeHint::PracticeSuggested;
}
inline bool start_practice(Preview& p,const SessionEvidence* evidence){
  if(!practice_available(p,evidence))return false;
  p.practice_only=true;p.step=p.current().count-2;p.selected=-1;p.screen=Screen::Activity;
  return true;
}
// Validate transient route: enter only from a finished lesson, keep the
// same lesson, and never skip beyond the existing two-activity suffix.
inline bool valid_practice_transition(const Preview& before,const Preview& after){
  if(!after.practice_only||after.reviewing||before.reviewing||before.lesson!=after.lesson||
     after.lesson>=lesson_count||!practice_suffix(after.current()))return false;
  const auto first=after.current().count-2;
  if(after.step<first||after.step>=after.current().count)return false;
  if(!before.practice_only)return before.screen==Screen::Finished&&
    after.screen==Screen::Activity&&after.step==first;
  if(before.screen==Screen::Finished)return false;
  auto expected=before;
  const bool moved=after.screen==Screen::Feedback&&after.selected>=0?
    expected.answer(static_cast<unsigned>(after.selected)):expected.next();
  return moved&&expected.screen==after.screen&&expected.step==after.step&&expected.selected==after.selected;
}
}
