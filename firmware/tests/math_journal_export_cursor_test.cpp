#include "math_journal_export_cursor.h"
#include <cassert>
#include <vector>

class Sink final:public MathSnapshotSink {
 public:
  MathSnapshot manifest{};
  std::vector<FakeNvs::Bytes> staged,published;
  bool aborted=false,fail=false;
  bool begin(const MathSnapshot& m)noexcept override{manifest=m;return true;}
  bool record(uint32_t slot,const AttemptBytes& b,std::size_t n)noexcept override{
    if(fail||slot!=staged.size()+1)return false;
    try{staged.emplace_back(b.begin(),b.begin()+n);return true;}catch(...){return false;}
  }
  bool commit()noexcept override{
    try{published=staged;return true;}catch(...){return false;}
  }
  void abort()noexcept override{staged.clear();aborted=true;}
};
static bool submit(MathJournal& j,MathPractice& state){
  static MathScratchpad scratch;static MathAttempt a;
  return make_math_attempt(state,0,scratch,a)&&j.append(a,state,state);
}
int main(){
  using S=MathExportState;
  FakeNvs::reset();MathJournal j;MathPractice state;
  assert(j.begin(state)==JournalLoad::Empty);
  for(unsigned i=0;i<100;++i)assert(submit(j,state));
  const auto before=FakeNvs::records;
  Sink sink;MathJournalExportCursor c(j,sink);
  assert(c.start(0)&&c.active());
  assert(c.poll(1,false)==S::Manifest&&sink.staged.empty());
  assert(c.poll(2,true)==S::Records&&sink.manifest.count==100);
  assert(!submit(j,state)&&j.begin(state)==JournalLoad::Error);
  Sink other;MathJournalExportCursor blocked(j,other);
  assert(!blocked.start(3)&&!blocked.active());
  for(unsigned i=0;i<100;++i){
    c.poll(3+i,true);
    assert(sink.staged.size()==i+1&&sink.published.empty());
  }
  assert(c.state()==S::Terminal);
  assert(c.poll(104,false)==S::Terminal&&sink.published.empty());
  assert(c.poll(105,true)==S::Complete&&!c.active());
  assert(FakeNvs::records==before&&sink.published.size()==100);
  unsigned i=1;
  for(const auto& raw:sink.published){
    char key[12];math_journal_key(i++,key);
    assert(raw==before.at("pljournal").at(key));
  }
  assert(submit(j,state));
  Sink paused;MathJournalExportCursor timeout(j,paused);
  assert(timeout.start(500)&&timeout.poll(501,true)==S::Records);
  assert(timeout.poll(110500,false)==S::Failed&&paused.aborted);
  assert(paused.published.empty()&&!timeout.active()&&submit(j,state));
  Sink broken;MathJournalExportCursor failure(j,broken);
  assert(failure.start(0));failure.poll(1,true);broken.fail=true;
  assert(failure.poll(2,true)==S::Failed&&broken.aborted&&!failure.active());
  {Sink cancelled;MathJournalExportCursor scope(j,cancelled);assert(scope.start(0));}
  assert(submit(j,state));
  Sink invalid;MathJournalExportCursor corrupted(j,invalid);
  assert(corrupted.start(0));corrupted.poll(1,true);
  FakeNvs::records["pljournal"]["a001"][0]^=1;
  assert(corrupted.poll(2,true)==S::Failed&&invalid.aborted&&invalid.published.empty());
  FakeNvs::reset();assert(j.begin(state)==JournalLoad::Empty);
  Sink empty;MathJournalExportCursor zero(j,empty);
  assert(zero.start(0)&&zero.poll(1,true)==S::Terminal);
  assert(zero.poll(2,true)==S::Complete&&empty.manifest.count==0);
}
