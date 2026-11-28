#pragma once
#include "curriculum_completion_ledger.h"

namespace curriculum_preview {
// Read-only projection of current catalog revisions. Historic revisions
// remain in the ledger but cannot inflate current lesson completion.
// A missing or failed backend is unknown, never an apparent zero score.
struct SubjectProgress {
  unsigned total=0,completed=0;
  bool known=false;
};
inline CompletionLookup lesson_progress(const CompletionLedger* ledger,const Lesson& lesson){
  return ledger?ledger->lookup(lesson):CompletionLookup::Unavailable;
}
inline SubjectProgress subject_progress(const CompletionLedger* ledger,unsigned subject){
  SubjectProgress result{};
  if(subject>=4)return result;
  result.known=ledger&&ledger->available();
  for(const auto& l:lessons){
    if(l.subject!=subject)continue;
    ++result.total;
    if(!result.known)continue;
    const auto state=lesson_progress(ledger,l);
    if(state==CompletionLookup::Unavailable)result.known=false;
    else if(state==CompletionLookup::Recorded)++result.completed;
  }
  if(!result.known)result.completed=0;
  return result;
}
// Read-only topic milestones: current revision completion, never mastery.
inline SubjectProgress topic_progress(const CompletionLedger* ledger,unsigned unit){
  SubjectProgress result{};
  if(unit>=unit_count)return result;
  result.known=ledger&&ledger->available();
  for(std::size_t i=0;i<lesson_count;++i){
    if(lesson_units[i]!=unit)continue;
    ++result.total;
    if(!result.known)continue;
    const auto state=lesson_progress(ledger,lessons[i]);
    if(state==CompletionLookup::Unavailable)result.known=false;
    else if(state==CompletionLookup::Recorded)++result.completed;
  }
  if(!result.known)result.completed=0;
  return result;
}
inline bool topic_finished(const SubjectProgress& p){
  return p.known&&p.total>0&&p.completed==p.total;
}
inline SubjectProgress topic_milestones(const CompletionLedger* ledger){
  SubjectProgress result{};
  result.known=ledger&&ledger->available();
  for(unsigned u=0;u<unit_count;++u){
    const auto p=topic_progress(ledger,u);
    if(!p.total)continue;
    ++result.total;
    if(!p.known)result.known=false;
    if(topic_finished(p))++result.completed;
  }
  if(!result.known)result.completed=0;
  return result;
}
inline const char* completion_label(CompletionLookup state){
  switch(state){
    case CompletionLookup::Recorded:return "Completed";
    case CompletionLookup::NotRecorded:return "Not completed";
    default:return "History unavailable";
  }
}
}
