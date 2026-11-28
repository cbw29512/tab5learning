#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include "curriculum_preview.h"
namespace curriculum_preview {
// Volatile first-response evidence for the current boot only. The v16
// catalog has one distinct skill per lesson. No storage, grades, or mastery.
// A lesson's index identifies its skill only in this fixed compiled catalog.
struct SkillEvidence {
  unsigned observed=0,correct=0,possible=0;
};
inline bool independent_choice(const Activity& a){
  return a.kind==Kind::Choice&&
    (std::strcmp(a.phase,"independent")==0||std::strcmp(a.phase,"assessment")==0);
}
inline constexpr unsigned evidence_step_limit=32;
// Shared admission rule: only genuine independent choice transitions.
inline bool independent_response(const Preview& before,const Preview& after){
  if(before.screen!=Screen::Activity||after.screen!=Screen::Feedback||
     before.reviewing||after.reviewing||before.practice_only||after.practice_only||
     before.lesson!=after.lesson||before.step!=after.step||after.lesson>=lesson_count||
     after.step>=after.current().count||after.step>=evidence_step_limit)return false;
  const auto& a=after.activity();
  return independent_choice(a)&&a.answer>=0&&unsigned(a.answer)<a.count&&
    after.selected>=0&&unsigned(after.selected)<a.count;
}
constexpr bool evidence_catalog_fits(){
  for(const auto& l:lessons)if(l.count>evidence_step_limit)return false;
  return true;
}
static_assert(evidence_catalog_fits(),"Extend evidence bitsets before larger lessons");
class SessionEvidence {
  struct Record { std::uint32_t observed=0,correct=0; };
  std::array<Record,lesson_count> records{};
 public:
  // Called only after an accepted Activity -> Feedback transition. A
  // repeated answer, review, guided prompt or teacher observation cannot
  // change this boot's first independent response.
  bool observe(const Preview& before,const Preview& after){
    if(!independent_response(before,after))return false;
    const auto& a=after.activity();
    auto& record=records[after.lesson];
    const auto bit=std::uint32_t{1}<<after.step;
    if(record.observed&bit)return false;
    record.observed|=bit;
    if(after.selected==a.answer)record.correct|=bit;
    return true;
  }
  SkillEvidence skill(std::size_t index)const{
    SkillEvidence result{};
    if(index>=lesson_count)return result;
    const auto& l=lessons[index];const auto& record=records[index];
    for(unsigned i=0;i<l.count;++i){
      if(!independent_choice(l.activities[i]))continue;
      ++result.possible;
      const auto bit=std::uint32_t{1}<<i;
      if(record.observed&bit)++result.observed;
      if(record.correct&bit)++result.correct;
    }
    return result;
  }
};
enum class PracticeHint { NoEvidence, IncompleteEvidence, PracticeSuggested, AnswersCorrect };
inline PracticeHint practice_hint(const SkillEvidence& e){
  if(!e.observed||e.correct>e.observed||e.observed>e.possible)return PracticeHint::NoEvidence;
  if(e.correct<e.observed)return PracticeHint::PracticeSuggested;
  if(e.observed<e.possible)return PracticeHint::IncompleteEvidence;
  return PracticeHint::AnswersCorrect;
}
}
