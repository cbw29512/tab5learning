#pragma once
#include <array>
#include <cstddef>

// Synthetic preview curriculum; not a validated mastery assessment.
// State remains in RAM until durable learning storage is integrated.
struct MathQuestion { int a,b; std::array<int,4> choices; };
inline constexpr std::array<MathQuestion,5> math_questions{{
  {2,3,{4,5,6,7}}, {4,2,{6,8,5,7}}, {3,4,{5,8,7,6}},
  {5,4,{8,10,7,9}}, {6,4,{10,9,11,8}}
}};
struct MathPractice {
  std::size_t index=0;
  unsigned attempts=0;
  unsigned independent=0;
  bool retried=false;
  bool finished=false;
};
enum class MathResult { Ignored, TryAgain, Correct, Finished };
inline MathResult math_answer(MathPractice& s, std::size_t choice) {
  if (s.finished || s.index>=math_questions.size() || choice>=4) return MathResult::Ignored;
  const auto& q=math_questions[s.index];
  ++s.attempts;
  if(q.choices[choice]!=q.a+q.b) { s.retried=true; return MathResult::TryAgain; }
  if(!s.retried) ++s.independent;
  s.retried=false;
  ++s.index;
  s.finished=s.index==math_questions.size();
  return s.finished?MathResult::Finished:MathResult::Correct;
}
inline int math_choice_at(int x,int y) {
  if(y<444 || y>=540) return -1;
  for(int i=0;i<4;++i) {
    const int left=32+i*308;
    if(x>=left && x<left+292) return i;
  }
  return -1;
}
