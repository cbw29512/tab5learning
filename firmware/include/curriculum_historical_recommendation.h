#pragma once
#include "curriculum_skill_history.h"
#include "curriculum_practice.h"
namespace curriculum_preview {
// Advisory development defaults, not educationally validated cutoffs.
// No grades, diagnosis, difficulty adjustment or automatic promotion.
enum class HistoricalHint {
  HistoryUnavailable, NoEvidence, MoreEvidenceNeeded, PracticeSuggested,
  RecurringDifficulty, NoCurrentDifficulty
};
inline HistoricalHint historical_hint(const SkillHistorySample& p){
  if(!p.known||p.correct>p.observations||p.sessions>p.observations||
     p.error_sessions>p.sessions||p.error_sessions>p.observations-p.correct||
     p.recent_error_sessions>p.error_sessions||
     (p.observations&& !p.sessions)||
     ((p.observations>p.correct)!=(p.error_sessions>0)))
    return HistoricalHint::HistoryUnavailable;
  if(!p.observations)return HistoricalHint::NoEvidence;
  if(p.recent_error_sessions>=2&&p.observations>=4)
    return HistoricalHint::RecurringDifficulty;
  if(p.recent_error_sessions)return HistoricalHint::PracticeSuggested;
  if(p.observations<4||p.sessions<2)return HistoricalHint::MoreEvidenceNeeded;
  return HistoricalHint::NoCurrentDifficulty;
}
struct HistoricalRecommendation {
  std::size_t lesson=lesson_count;
  HistoricalHint hint=HistoricalHint::HistoryUnavailable;
  unsigned observations=0,correct=0,error_sessions=0;
};
// History can be provided by a verified reader or writer. Do not construct
// a storage owner here. Prefer recurring difficulty, then an isolated
// recent error; tie-break by consecutive error sessions and catalog order.
// Never substitute a different subject, or select unobserved skills.
template<class History>
HistoricalRecommendation historical_practice(unsigned subject,const History& history){
  HistoricalRecommendation result{};
  if(subject>=4||!history.available())return result;
  result.hint=HistoricalHint::NoEvidence;
  unsigned best_run=0;
  for(std::size_t i=0;i<lesson_count;++i){
    const auto& l=lessons[i];
    if(l.subject!=subject||!practice_suffix(l))continue;
    const auto p=history.skill(i);
    const auto hint=historical_hint(p);
    // Any unavailable current skill makes the whole projection unknown.
    if(hint==HistoricalHint::HistoryUnavailable)return {};
    if(hint!=HistoricalHint::PracticeSuggested&&hint!=HistoricalHint::RecurringDifficulty)continue;
    const bool stronger=hint==HistoricalHint::RecurringDifficulty&&result.hint!=hint;
    if(result.lesson!=lesson_count&&!stronger&&
       (hint!=result.hint||p.recent_error_sessions<=best_run))continue;
    best_run=p.recent_error_sessions;
    result={i,hint,p.observations,p.correct,p.error_sessions};
  }
  return result;
}
}
