#include "curriculum_sd_fixture.h"
#include <cassert>
#include <cstdlib>
using namespace curriculum_preview;
int main(){
  char path[]="/tmp/tab5-sd-fixture-XXXXXX";
  int seed=mkstemp(path);assert(seed>=0);assert(close(seed)==0);
  // Existing path must not be opened by the exclusive creation branch.
  assert(open(path,O_CREAT|O_EXCL|O_RDWR|O_APPEND,0600)==-1&&errno==EEXIST);
  assert(unlink(path)==0);
  const int fd=open(path,O_CREAT|O_EXCL|O_RDWR|O_APPEND,0600);assert(fd>=0);
  {
    CompletionFile file(fd);assert(file.begin());
    CompletionLedger ledger(file);assert(probe_initialize(file,ledger));
    assert(probe_exact(file)&&ledger.verified_count()==2);
    // Refuse to initialize an existing populated log again.
    assert(!probe_initialize(file,ledger)&&probe_exact(file));
  }
  assert(close(fd)==0);
  const int reopened=open(path,O_RDWR|O_APPEND);assert(reopened>=0);
  {
    CompletionFile file(reopened);assert(file.begin()&&probe_exact(file));
    const char extra='x';assert(write(reopened,&extra,1)==1&&fsync(reopened)==0);
    assert(!probe_exact(file)); // exact fixture, no repair
    std::uint64_t n=0;assert(file.size(n)&&n==193);
  }
  assert(close(reopened)==0&&unlink(path)==0);
}
