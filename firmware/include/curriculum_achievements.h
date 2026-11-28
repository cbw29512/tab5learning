#pragma once
#include <array>
#include "curriculum_progress.h"
namespace curriculum_preview {
// Derived milestones, not separately persisted awards. Revisions and
// unavailable evidence can change their current status. No clock needed.
enum class AchievementMetric { Lessons, Topics };
enum class AchievementState { Unknown, Locked, Earned };
struct AchievementDefinition {
  const char* name;
  AchievementMetric metric;
  unsigned target;
};
inline constexpr std::array<AchievementDefinition,6> achievements{{
  {"First lesson",AchievementMetric::Lessons,1},
  {"Five lessons",AchievementMetric::Lessons,5},
  {"First topic",AchievementMetric::Topics,1},
  {"Ten lessons",AchievementMetric::Lessons,10},
  {"Twenty-five lessons",AchievementMetric::Lessons,25},
  {"Fifty lessons",AchievementMetric::Lessons,50}
}};
struct AchievementProgress {
  AchievementState state=AchievementState::Unknown;
  unsigned completed=0,target=0;
};
inline SubjectProgress grade_progress(const CompletionLedger* ledger){
  SubjectProgress result{};
  result.known=ledger&&ledger->available();
  for(const auto& l:lessons){
    ++result.total;
    if(!result.known)continue;
    const auto status=lesson_progress(ledger,l);
    if(status==CompletionLookup::Unavailable)result.known=false;
    else if(status==CompletionLookup::Recorded)++result.completed;
  }
  if(!result.known)result.completed=0;
  return result;
}
inline AchievementProgress achievement_progress(const AchievementDefinition& def,
    const SubjectProgress& lessons_done,const SubjectProgress& topics_done){
  const auto& p=def.metric==AchievementMetric::Lessons?lessons_done:topics_done;
  if(!p.known||!def.target||p.completed>p.total)return {AchievementState::Unknown,0,def.target};
  return {p.completed>=def.target?AchievementState::Earned:AchievementState::Locked,
    p.completed<def.target?p.completed:def.target,def.target};
}
}
