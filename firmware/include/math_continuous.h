#pragma once
#include <array>
#include <cstdint>
#include <limits>

namespace math_continuous {
// Immutable draft bank v1: all ordered addition pairs with sum <=20,
// and corresponding subtraction pairs. First within 10, then 11..20.
// A question's meaning is bank revision + ordinal modulo bank_size.
// Never reinterpret an old revision after a curriculum update.
inline constexpr std::uint32_t bank_revision=1;
inline constexpr std::uint32_t bank_size=462;
enum class Operation : std::uint8_t { Add, Subtract };
struct Question {
  Operation operation=Operation::Add;
  unsigned a=0,b=0;
  std::array<unsigned,4> choices{};
  unsigned correct=0;
};
inline Question question(std::uint32_t ordinal){
  // Each sum n contributes n+1 ordered pairs for each operation.
  unsigned offset=ordinal%bank_size;
  unsigned sum=0;
  while(offset>=2*(sum+1)){offset-=2*(sum+1);++sum;}
  const unsigned first=offset%(sum+1),second=sum-first;
  const bool subtract=offset>=sum+1;
  Question q{};
  q.operation=subtract?Operation::Subtract:Operation::Add;
  q.a=subtract?sum:first;q.b=second;
  const unsigned answer=subtract?first:sum;
  // Rotate correct index by ordinal, with three distinct nonnegative
  // distractors in the 0..20 range. No external random state.
  q.correct=ordinal%4;
  q.choices[q.correct]=answer;
  unsigned delta=1;
  for(unsigned i=0;i<4;++i)
    if(i!=q.correct)q.choices[i]=(answer+delta++)%21;
  return q;
}
struct State {
  std::uint32_t attempts=0,solved=0,independent=0;
  bool retried=false;
};
inline bool valid(const State& s){
  return s.independent<=s.solved&&s.solved<=s.attempts&&
    s.attempts-s.solved>=s.solved-s.independent&&
    s.attempts-s.solved-(s.solved-s.independent)>=unsigned(s.retried);
}
enum class Result : std::uint8_t { Rejected, TryAgain, Correct };
inline Result answer(State& s,unsigned choice){
  if(!valid(s)||choice>=4||s.attempts==std::numeric_limits<std::uint32_t>::max())
    return Result::Rejected;
  const auto q=question(s.solved);
  ++s.attempts;
  if(choice!=q.correct){s.retried=true;return Result::TryAgain;}
  if(!s.retried)++s.independent;
  ++s.solved;s.retried=false;return Result::Correct;
}
}
