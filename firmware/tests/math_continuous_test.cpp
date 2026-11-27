#include "math_continuous_record.h"
#include <cassert>
#include <set>
#include <tuple>
using namespace math_continuous;
int main(){
  std::set<std::tuple<unsigned,unsigned,unsigned>> distinct;
  for(unsigned i=0;i<bank_size;++i){
    const auto q=question(i);
    const unsigned correct=q.operation==Operation::Add?q.a+q.b:q.a-q.b;
    assert(q.correct<4&&q.choices[q.correct]==correct&&correct<=20);
    assert(q.operation==Operation::Add?q.a+q.b<=20:q.a>=q.b&&q.a<=20);
    std::set<unsigned> choices(q.choices.begin(),q.choices.end());
    assert(choices.size()==4&&*choices.rbegin()<=20);
    assert(distinct.emplace(unsigned(q.operation),q.a,q.b).second);
    if(i<132)assert(q.operation==Operation::Add?q.a+q.b<=10:q.a<=10);
  }
  assert(distinct.size()==bank_size);
  State s{},restored{};
  MathScratchpad scratch;
  assert(scratch_touch(scratch,656,184,true));
  Record bytes{};std::size_t n=0;
  const auto first=question(0);
  assert(encode(s,(first.correct+1)%4,scratch,bytes,n)&&n==41);
  assert(replay(bytes,n,s)&&s.attempts==1&&s.solved==0&&s.retried);
  assert(replay(bytes,n,restored)&&restored.retried);
  assert(!replay(bytes,n,s)); // duplicate sequence
  auto wrong=bytes;wrong[17]=std::uint8_t(Result::Correct);
  put(wrong,n-4,crc(wrong,n-4));State empty{};
  assert(!replay(wrong,n,empty)&&empty.attempts==0);
  auto version=bytes;put(version,4,2);put(version,n-4,crc(version,n-4));
  assert(!replay(version,n,empty));
  assert(encode(s,first.correct,scratch,bytes,n));
  assert(replay(bytes,n,s)&&replay(bytes,n,restored));
  assert(s.solved==1&&s.independent==0&&!s.retried);
  scratch_clear(scratch);
  for(unsigned i=1;i<1000;++i){
    assert(encode(s,question(s.solved).correct,scratch,bytes,n)&&n==36);
    assert(replay(bytes,n,s)&&replay(bytes,n,restored));
  }
  assert(s.solved==1000&&s.attempts==1001&&s.independent==999);
  assert(restored.solved==s.solved&&restored.attempts==s.attempts);
  State exhausted{UINT32_MAX,0,0,false};
  assert(!encode(exhausted,0,scratch,bytes,n)&&n==0);
}
