#pragma once
#include <cstdint>

namespace learner_storage {
// All values are bytes; decimal thresholds match the SD capacity labels.
// Unknown is not zero free space. No allocation, filesystem calls or writes.
enum class State { NotChecked, Unavailable, Normal, Low, Critical };
inline constexpr std::uint64_t low_bytes=1000000000ULL;
inline constexpr std::uint64_t critical_bytes=256000000ULL;
struct Status {
  State state=State::NotChecked;
  std::uint64_t total=0,used=0,free=0;
  unsigned free_percent=0;
  bool known()const{
    return state==State::Normal||state==State::Low||state==State::Critical;
  }
};
// Calculate floor(total*p/100) without overflowing 64-bit integers.
inline constexpr std::uint64_t percent_floor(std::uint64_t total,unsigned p){
  return (total/100)*p+((total%100)*p)/100;
}
inline unsigned free_percent(std::uint64_t free,std::uint64_t total){
  if(!total||free>total)return 0;
  // Compare against ceil(total*p/100), so tiny cards and large values
  // cannot report more free space than is actually available.
  for(unsigned p=100;p>0;--p){
    const auto floor=percent_floor(total,p);
    const auto ceil=floor+(((total%100)*p)%100!=0);
    if(free>=ceil)return p;
  }
  return 0;
}
inline Status unavailable(){return {State::Unavailable,0,0,0,0};}
inline Status measured(std::uint64_t total,std::uint64_t used){
  if(!total||used>total)return unavailable();
  const auto free=total-used;
  const auto state=(free<=critical_bytes||free<=percent_floor(total,5))?State::Critical:
    (free<=low_bytes||free<=percent_floor(total,10))?State::Low:State::Normal;
  return {state,total,used,free,free_percent(free,total)};
}
inline const char* state_label(State state){
  switch(state){
    case State::NotChecked:return "Storage not checked";
    case State::Unavailable:return "Storage unavailable";
    case State::Normal:return "Storage OK";
    case State::Low:return "Storage low - tell your teacher";
    case State::Critical:return "Storage critical - tell your teacher";
  }
  return "Storage unavailable";
}
// Candidate admission policy for OPTIONAL new content only, not learner
// evidence. Caller must recheck capacity and still handle real write errors.
// Reject unknown sizes/overflow by subtraction; reserve bytes are separate.
inline bool optional_content_fits(const Status& s,std::uint64_t incoming,
                                  std::uint64_t reserve=critical_bytes){
  return s.known()&&s.state!=State::Critical&&incoming>0&&
    s.free>=reserve&&incoming<=s.free-reserve;
}
}
