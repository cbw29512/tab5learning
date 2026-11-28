#pragma once
#include "curriculum_browse.h"
namespace curriculum_preview {
// Reconstruct the same topic browser around a saved activity without changing
// the on-flash CPR1 schema. Never offer a finished checkpoint as unfinished.
template<class Store>
bool continue_target(const Store& store,const CompletionLedger* ledger,Preview& out){
  Preview saved{};
  if(!store.restore(saved)||saved.screen!=Screen::Activity||
     saved.lesson>=lesson_count||saved.step>=saved.current().count)return false;
  if(lesson_progress(ledger,saved.current())==CompletionLookup::Recorded)return false;
  Preview target{};
  if(!target.choose_subject_grouped(saved.current().subject))return false;
  const auto unit=lesson_units[saved.lesson];
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
    if(browse_visible(target,n%lessons_per_page,ledger)==&saved.current()){
      if(!browse_choose(target,n%lessons_per_page,ledger))return false;
      target.step=saved.step;out=target;return true;
    }
  }
  return false;
}
}
