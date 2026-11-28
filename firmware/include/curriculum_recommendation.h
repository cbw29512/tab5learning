#pragma once
#include "curriculum_continue.h"
namespace curriculum_preview {
static_assert(sizeof(lesson_prerequisite_offsets)/sizeof(unsigned)==lesson_count+1);
static_assert(lesson_prerequisite_offsets[lesson_count]+1==sizeof(lesson_prerequisites)/sizeof(unsigned));
// Recommendation is advisory, not an admission gate or mastery assessment.
inline bool prerequisites_completed(std::size_t index,const CompletionLedger* ledger){
  if(index>=lesson_count||!ledger||!ledger->available())return false;
  const auto first=lesson_prerequisite_offsets[index],last=lesson_prerequisite_offsets[index+1];
  if(first>last||last>lesson_prerequisite_offsets[lesson_count])return false;
  for(auto i=first;i<last;++i){
    const auto prior=lesson_prerequisites[i];
    if(prior>=lesson_count||prior==index||lesson_progress(ledger,lessons[prior])!=CompletionLookup::Recorded)return false;
  }
  return true;
}
inline const Lesson* recommended_after(const Preview& p,const CompletionLedger* ledger){
  if(p.screen!=Screen::Finished||p.reviewing||p.lesson>=lesson_count||
     !ledger||!ledger->available()||lesson_progress(ledger,p.current())!=CompletionLookup::Recorded)return nullptr;
  // Prefer same topic, then any eligible lesson in the same subject.
  // Stable catalog order breaks ties; don't automatically change subjects.
  for(unsigned pass=0;pass<2;++pass){
    for(std::size_t i=0;i<lesson_count;++i){
      if(lessons[i].subject!=p.current().subject)continue;
      if((lesson_units[i]==lesson_units[p.lesson])!=(pass==0))continue;
      if(lesson_progress(ledger,lessons[i])==CompletionLookup::NotRecorded&&prerequisites_completed(i,ledger))return &lessons[i];
    }
  }
  return nullptr;
}
inline bool start_recommended(Preview& p,const CompletionLedger* ledger){
  const auto* next=recommended_after(p,ledger);
  if(!next)return false;
  Preview target{};
  if(!open_unfinished_lesson(static_cast<std::size_t>(next-lessons),0,ledger,target))return false;
  p=target;return true;
}
}
