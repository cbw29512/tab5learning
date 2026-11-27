#include "curriculum_completion_ledger.h"
#include <cassert>
#include <vector>
using namespace curriculum_preview;

struct MemoryBackend:CompletionBackend {
  std::vector<CompletionBytes> records;
  bool reject=false,ambiguous=false,partial=false,fail_read=false;
  unsigned writes=0;
  bool size(std::uint64_t& bytes)override{
    bytes=records.size()*96+unsigned(partial);return true;
  }
  bool read(unsigned slot,CompletionBytes& b)override{
    if(fail_read||slot>=records.size())return false;
    b=records[slot];return true;
  }
  bool append(unsigned expected,const CompletionBytes& b)override{
    if(reject||expected!=records.size())return false;
    records.push_back(b);++writes;return !ambiguous;
  }
};
Preview completed(unsigned subject,unsigned row){
  Preview p;assert(p.choose_subject(subject)&&p.choose_lesson(row));
  while(p.screen!=Screen::Finished){
    if(p.screen==Screen::Activity&&p.activity().kind==Kind::Choice)assert(p.answer(0));
    else assert(p.next());
  }
  return p;
}
int main(){
  MemoryBackend media;
  CompletionLedger l(media);
  assert(l.begin()&&l.verified_count()==0);
  Preview unfinished;assert(l.finish(unfinished)==CompletionResult::Invalid);
  auto math=completed(0,0),reading=completed(1,0);
  assert(l.lookup(math.current())==CompletionLookup::NotRecorded);
  assert(l.finish(math)==CompletionResult::Saved);
  assert(l.finish(math)==CompletionResult::AlreadyRecorded&&media.writes==1);
  assert(l.finish(reading)==CompletionResult::Saved);
  CompletionLedger reboot(media);
  assert(reboot.begin()&&reboot.verified_count()==2);
  assert(reboot.lookup(math.current())==CompletionLookup::Recorded);
  // Preserve a historical lesson revision that is no longer in the current catalog.
  CompletionEvent historical{};assert(completion_decode(media.records[0],historical));
  historical.key.revision=99;historical.sequence=3;
  CompletionBytes old{};assert(completion_encode(historical,old));
  media.records.push_back(old);
  assert(reboot.begin()&&reboot.verified_count()==3);
  auto science=completed(2,0);
  media.reject=true;
  assert(reboot.finish(science)==CompletionResult::Unavailable&&!reboot.available());
  media.reject=false;assert(reboot.begin()&&reboot.verified_count()==3);
  media.ambiguous=true;
  assert(reboot.finish(science)==CompletionResult::Unavailable);
  media.ambiguous=false;assert(reboot.begin()&&reboot.verified_count()==4);
  assert(reboot.finish(science)==CompletionResult::AlreadyRecorded);
  media.partial=true;assert(!reboot.begin()&&!reboot.available());
  media.partial=false;assert(reboot.begin());
  auto copy=media.records[0];copy[0]^=1;media.records[0]=copy;
  assert(!reboot.begin()&&reboot.lookup(math.current())==CompletionLookup::Unavailable);
  assert(media.records[0]==copy); // never repair corruption by erasing
  // Duplicate key with correct CRC and sequence is invalid.
  MemoryBackend duplicate;
  CompletionEvent e{};assert(completion_decode(media.records[1],e));e.sequence=1;
  assert(completion_encode(e,copy));duplicate.records.push_back(copy);
  e.sequence=2;assert(completion_encode(e,copy));duplicate.records.push_back(copy);
  CompletionLedger invalid(duplicate);assert(!invalid.begin());
  // A valid CRC does not forgive a missing sequence.
  duplicate.records.resize(1);e.sequence=3;e.key.revision=2;
  assert(completion_encode(e,copy));duplicate.records.push_back(copy);
  assert(!invalid.begin());
}
