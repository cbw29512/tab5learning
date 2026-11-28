#include "curriculum_historical_recommendation.h"
#include <cassert>
using namespace curriculum_preview;
struct History {
  std::array<SkillHistorySample,lesson_count> samples{};
  bool ready=true;
  History(){for(auto& sample:samples)sample.known=true;}
  bool available()const{return ready;}
  SkillHistorySample skill(std::size_t i)const{return samples[i];}
};
int main(){
  assert(historical_hint({})==HistoricalHint::HistoryUnavailable);
  assert(historical_hint({true,0,0,0,0,0})==HistoricalHint::NoEvidence);
  assert(historical_hint({true,1,1,1,0,0})==HistoricalHint::MoreEvidenceNeeded);
  assert(historical_hint({true,2,0,1,1,1})==HistoricalHint::PracticeSuggested);
  assert(historical_hint({true,3,1,2,2,2})==HistoricalHint::PracticeSuggested);
  assert(historical_hint({true,4,2,2,2,2})==HistoricalHint::RecurringDifficulty);
  assert(historical_hint({true,5,3,3,2,0})==HistoricalHint::NoCurrentDifficulty);
  assert(historical_hint({true,2,3,1,0,0})==HistoricalHint::HistoryUnavailable);
  assert(historical_hint({true,2,1,1,0,0})==HistoricalHint::HistoryUnavailable);
  assert(historical_hint({true,2,1,1,1,2})==HistoricalHint::HistoryUnavailable);
  History h;
  std::size_t math=lesson_count,math2=lesson_count,ela=lesson_count;
  for(std::size_t i=0;i<lesson_count;++i){
    if(lessons[i].subject==0&&practice_suffix(lessons[i])){
      if(math==lesson_count)math=i;else if(math2==lesson_count)math2=i;
    }
    if(lessons[i].subject==1&&ela==lesson_count&&practice_suffix(lessons[i]))ela=i;
  }
  assert(math2<lesson_count&&ela<lesson_count);
  assert(historical_practice(0,h).lesson==lesson_count);
  h.samples[math]={true,2,1,1,1,1};
  h.samples[ela]={true,6,0,3,3,3};
  assert(historical_practice(0,h).lesson==math);
  h.samples[math2]={true,4,2,2,2,2};
  assert(historical_practice(0,h).lesson==math2);
  h.samples[math]={true,4,2,2,2,2};
  assert(historical_practice(0,h).lesson==math); // stable catalog tie
  h.samples[math]={true,5,3,3,2,0};
  assert(historical_practice(0,h).lesson==math2); // later correct session
  h.samples[math2]={true,5,3,3,2,0};
  assert(historical_practice(0,h).lesson==lesson_count);
  assert(historical_practice(1,h).lesson==ela);
  h.ready=false;assert(historical_practice(1,h).hint==HistoricalHint::HistoryUnavailable);
  h.ready=true;h.samples[math]={};
  assert(historical_practice(0,h).hint==HistoricalHint::HistoryUnavailable);
}
