#include "math_continuous_journal.h"
#include <cassert>
#include <cstdlib>
using namespace math_continuous;
static void recover(Journal& journal,State& state){
  assert(journal.start());
  unsigned polls=0;
  while(journal.state()==JournalState::Replaying){
    assert(++polls<10000);
    journal.poll();
  }
  assert(journal.restored(state));
}
int main(){
  char path[]="/tmp/tab5-math-pma-XXXXXX";
  const int initial=mkstemp(path);assert(initial>=0&&close(initial)==0);
  int fd=open(path,O_RDWR|O_APPEND);assert(fd>=0);
  State state{};MathScratchpad ink;Result feedback=Result::Rejected;
  {
    Journal journal(fd);recover(journal,state);
    assert(scratch_touch(ink,656,184,true));
    assert(journal.save(state,(question(0).correct+1)%4,ink,feedback)==SaveResult::Saved);
    assert(state.attempts==1&&state.retried&&ink.ink.count==1);
    assert(journal.save(state,question(0).correct,ink,feedback)==SaveResult::Saved);
    assert(state.solved==1&&!ink.ink.count&&state.independent==0);
    for(unsigned i=1;i<4100;++i)
      assert(journal.save(state,question(state.solved).correct,ink,feedback)==SaveResult::Saved);
    assert(state.solved==4100&&state.attempts==4101&&state.independent==4099);
    State stale{};
    assert(journal.save(stale,0,ink,feedback)==SaveResult::Rejected);
  }
  assert(close(fd)==0);fd=open(path,O_RDWR|O_APPEND);assert(fd>=0);
  {
    Journal reboot(fd);State restored{};recover(reboot,restored);
    assert(restored.solved==4100&&restored.attempts==4101);
    assert(reboot.save(restored,question(restored.solved).correct,ink,feedback)==SaveResult::Saved);
    assert(restored.solved==4101);
    // An external change violates exclusive ownership. No more UI updates.
    assert(write(fd,"P",1)==1);
    assert(reboot.save(restored,question(restored.solved).correct,ink,feedback)==SaveResult::Unavailable);
    assert(restored.solved==4101&&reboot.state()==JournalState::Failed);
  }
  struct stat before{},after{};assert(fstat(fd,&before)==0);
  {
    Journal corrupt(fd);assert(corrupt.start());
    State sentinel{9,0,0,false};
    while(corrupt.state()==JournalState::Replaying)corrupt.poll();
    assert(corrupt.state()==JournalState::Failed&&!corrupt.restored(sentinel));
    assert(sentinel.attempts==9);
    assert(fstat(fd,&after)==0&&before.st_size==after.st_size);
  }
  assert(close(fd)==0);
  fd=open(path,O_RDONLY);assert(fd>=0);
  {Journal readonly(fd);assert(!readonly.start());}
  assert(close(fd)==0&&unlink(path)==0);
}
