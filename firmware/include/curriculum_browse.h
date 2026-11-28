#pragma once
#include "curriculum_progress.h"
namespace curriculum_preview {
// Only revision-matched durable evidence may move a lesson to Finished.
// With unavailable history, show ALL lessons rather than hiding anything.
inline bool browse_known(const Preview& p,const CompletionLedger* ledger){
  return subject_progress(ledger,p.subject).known;
}
inline bool browse_matches(const Preview& p,const Lesson& l,const CompletionLedger* ledger,bool known){
  if(!p.matches(l))return false;
  if(!known)return true;
  return (lesson_progress(ledger,l)==CompletionLookup::Recorded)==p.finished_only;
}
inline unsigned browse_count(const Preview& p,const CompletionLedger* ledger){
  const bool known=browse_known(p,ledger);
  unsigned count=0;
  for(const auto& l:lessons)if(browse_matches(p,l,ledger,known))++count;
  return count;
}
inline const Lesson* browse_visible(const Preview& p,unsigned row,const CompletionLedger* ledger){
  if(row>=lessons_per_page)return nullptr;
  const bool known=browse_known(p,ledger);
  auto n=p.page*lessons_per_page+row;
  for(const auto& l:lessons)if(browse_matches(p,l,ledger,known)){
    if(n==0)return &l;
    --n;
  }
  return nullptr;
}
inline void browse_clamp(Preview& p,const CompletionLedger* ledger){
  if(p.screen!=Screen::Lessons)return;
  if(!browse_known(p,ledger))p.finished_only=false;
  const auto pages=lesson_pages(browse_count(p,ledger));
  if(p.page>=pages)p.page=pages-1;
}
inline bool browse_filter(Preview& p,bool finished,const CompletionLedger* ledger){
  if(p.screen!=Screen::Lessons||!browse_known(p,ledger)||p.finished_only==finished)return false;
  p.finished_only=finished;p.page=0;return true;
}
inline bool browse_page(Preview& p,int direction,const CompletionLedger* ledger){
  if(p.screen!=Screen::Lessons)return false;
  if(direction==-1&&p.page>0){--p.page;return true;}
  if(direction==1&&lesson_page_valid(p.page+1,browse_count(p,ledger))){++p.page;return true;}
  return false;
}
inline bool browse_choose(Preview& p,unsigned row,const CompletionLedger* ledger){
  if(p.screen!=Screen::Lessons)return false;
  const auto* l=browse_visible(p,row,ledger);
  if(!l)return false;
  p.lesson=static_cast<std::size_t>(l-lessons);p.step=0;p.selected=-1;
  p.practice_only=false;
  p.reviewing=lesson_progress(ledger,*l)==CompletionLookup::Recorded;
  p.screen=Screen::Activity;return true;
}
// Prefer another unfinished lesson in the selected topic. This is a catalog
// order suggestion, not a prerequisite, grade or mastery determination.
inline bool browse_after_finish(Preview& p,const CompletionLedger* ledger){
  if(p.screen!=Screen::Finished)return false;
  const bool review=p.reviewing;
  if(!p.back())return false;
  if(!review&&browse_known(p,ledger)){p.finished_only=false;p.page=0;}
  browse_clamp(p,ledger);
  return true;
}
}
