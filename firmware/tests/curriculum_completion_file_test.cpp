#include "curriculum_completion_file.h"
#include <cassert>
#include <cstdlib>
#include <sys/stat.h>
using namespace curriculum_preview;
Preview complete(){
  Preview p;assert(p.choose_subject(0)&&p.choose_lesson(0));
  while(p.screen!=Screen::Finished){
    if(p.screen==Screen::Activity&&p.activity().kind==Kind::Choice)assert(p.answer(0));
    else assert(p.next());
  }
  return p;
}
int main(){
  char path[]="/tmp/tab5-completion-XXXXXX";
  int fd=mkstemp(path);assert(fd>=0);assert(close(fd)==0);
  fd=open(path,O_RDWR);assert(fd>=0);
  CompletionFile bad(fd);assert(!bad.begin());assert(close(fd)==0);
  fd=open(path,O_RDWR|O_APPEND);assert(fd>=0);
  auto p=complete();
  {
    CompletionFile file(fd);assert(file.begin());
    CompletionLedger l(file);assert(l.begin());
    assert(l.finish(p)==CompletionResult::Saved);
    assert(l.finish(p)==CompletionResult::AlreadyRecorded);
    std::uint64_t n=0;assert(file.size(n)&&n==96);
  }
  assert(close(fd)==0);
  fd=open(path,O_RDWR|O_APPEND);assert(fd>=0);
  {
    CompletionFile file(fd);assert(file.begin());
    CompletionLedger l(file);assert(l.begin()&&l.verified_count()==1);
    assert(l.lookup(p.current())==CompletionLookup::Recorded);
    assert(l.finish(p)==CompletionResult::AlreadyRecorded);
    // Simulate a torn final write using the test fixture only.
    const char tail='X';assert(write(fd,&tail,1)==1&&fsync(fd)==0);
    assert(!l.begin());
    std::uint64_t n=0;assert(file.size(n)&&n==97);
    assert(l.finish(p)==CompletionResult::Unavailable);
    assert(file.size(n)&&n==97);
  }
  assert(close(fd)==0);
  fd=open(path,O_RDONLY);assert(fd>=0);
  CompletionFile readonly(fd);assert(!readonly.begin());assert(close(fd)==0);
  assert(unlink(path)==0); // only delete test-created file
}
