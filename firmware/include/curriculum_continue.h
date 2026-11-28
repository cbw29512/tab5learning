#pragma once
#include "curriculum_browse.h"
namespace curriculum_preview {
// Shared route reconstruction for resume and recommended lessons.
inline bool open_unfinished_lesson(std::size_t index,std::size_t step,const CompletionLedger* ledger,Preview& out){
  if(index>=lesson_count||step>=lessons[index].count||
     lesson_progress(ledger,lessons[index])==CompletionLookup::Recorded)return false;
  Preview target{};
  if(!target.choose_subject_grouped(lessons[index].subject))return false;
  const auto unit=lesson_units[index];
  bool found=false;
  for(unsigned n=0;n<target.units_in_subject();++n){
    target.unit_page=n/lessons_per_page;
    const auto* visible=target.visible_unit(n%lessons_per_page);
    if(visible&&static_cast<unsigned>(visible-units)==unit){
      found=target.choose_unit(n%lessons_per_page);break;
    }
  }
  if(!found)return false;
  for(unsigned n=0;n<browse_count(target,ledger);++n){
    target.page=n/lessons_per_page;
    if(browse_visible(target,n%lessons_per_page,ledger)==&lessons[index]){
      if(!browse_choose(target,n%lessons_per_page,ledger))return false;
      target.step=step;out=target;return true;
    }
  }
  return false;
}
// Resume is still read-only until the normal transition saves the next activity.
template<class Store>
bool continue_target(const Store& store,const CompletionLedger* ledger,Preview& out){
  Preview saved{};
  if(!store.restore(saved)||saved.screen!=Screen::Activity)return false;
  return open_unfinished_lesson(saved.lesson,saved.step,ledger,out);
}
}
