#include "curriculum_progress.h"
#include <cassert>
#include <vector>
using namespace curriculum_preview;
struct Media:CompletionBackend{
  std::vector<CompletionBytes> records;
  bool invalid=false;
  bool size(std::uint64_t& n)override{n=records.size()*96+unsigned(invalid);return true;}
  bool read(unsigned i,CompletionBytes& b)override{
    if(i>=records.size())return false;
    b=records[i];return true;
  }
  bool append(unsigned i,const CompletionBytes& b)override{
    if(i!=records.size())return false;
    records.push_back(b);return true;
  }
};
int main(){
  for(unsigned s=0;s<4;++s){
    auto p=subject_progress(nullptr,s);
    assert(!p.known&&p.total==(s==0?27u:17u)&&p.completed==0);
  }
  assert(!subject_progress(nullptr,4).known);
  Media media;CompletionLedger ledger(media);
  assert(!subject_progress(&ledger,0).known);
  assert(ledger.begin());
  auto p=subject_progress(&ledger,0);
  assert(p.known&&p.total==27&&p.completed==0);
  Preview lesson;
  assert(lesson.choose_subject(0)&&lesson.change_page(1)&&lesson.choose_lesson(0));
  assert(lesson_progress(&ledger,lesson.current())==CompletionLookup::NotRecorded);
  while(lesson.screen!=Screen::Finished){
    if(lesson.screen==Screen::Activity&&lesson.activity().kind==Kind::Choice)assert(lesson.answer(0));
    else assert(lesson.next());
  }
  assert(ledger.finish(lesson)==CompletionResult::Saved);
  assert(ledger.finish(lesson)==CompletionResult::AlreadyRecorded);
  assert(subject_progress(&ledger,0).completed==1);
  assert(subject_progress(&ledger,1).completed==0);
  // Historical revision remains in the ledger without increasing current progress.
  CompletionEvent historical{};assert(completion_decode(media.records[0],historical));
  historical.key.revision=99;historical.sequence=2;
  CompletionBytes b{};assert(completion_encode(historical,b));media.records.push_back(b);
  assert(ledger.begin()&&ledger.verified_count()==2);
  assert(subject_progress(&ledger,0).completed==1);
  assert(lesson_progress(&ledger,lesson.current())==CompletionLookup::Recorded);
  media.invalid=true;assert(!ledger.begin());
  assert(!subject_progress(&ledger,0).known);
  assert(lesson_progress(&ledger,lesson.current())==CompletionLookup::Unavailable);
}
