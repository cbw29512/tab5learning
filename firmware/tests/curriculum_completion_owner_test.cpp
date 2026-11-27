#include "curriculum_completion_owner.h"
#include <cassert>
#include <cstdlib>
#include <string>
using namespace curriculum_preview;
int main(){
  char directory[]="/tmp/tab5-owner-XXXXXX";
  assert(mkdtemp(directory));
  const std::string path=std::string(directory)+"/evaluation.bin";
  Preview p;
  assert(p.choose_subject(0)&&p.choose_lesson(0));
  while(p.screen!=Screen::Finished){
    if(p.screen==Screen::Activity&&p.activity().kind==Kind::Choice)assert(p.answer(0));
    else assert(p.next());
  }
  {
    CompletionOwner owner;
    assert(owner.open_once(path.c_str())&&owner.view());
    assert(!owner.open_once(path.c_str()));
    assert(owner.view()->finish(p)==CompletionResult::Saved);
    assert(owner.close_owner()&&!owner.view());
  }
  {
    CompletionOwner owner;
    assert(owner.open_once(path.c_str()));
    assert(owner.view()->lookup(p.current())==CompletionLookup::Recorded);
    assert(owner.view()->finish(p)==CompletionResult::AlreadyRecorded);
  }
  const int fd=open(path.c_str(),O_WRONLY|O_APPEND);
  assert(fd>=0&&write(fd,"X",1)==1&&close(fd)==0);
  CompletionOwner corrupt;
  assert(!corrupt.open_once(path.c_str())&&!corrupt.view());
  struct stat st{};
  assert(stat(path.c_str(),&st)==0&&st.st_size==97);
  CompletionOwner missing;
  assert(!missing.open_once((path+"/missing").c_str()));
  assert(unlink(path.c_str())==0&&rmdir(directory)==0);
}
