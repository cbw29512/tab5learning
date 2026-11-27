#pragma once
#include <cstdio>
#include "curriculum_completion_codec.h"

namespace curriculum_preview {
// Durable storage contract: inspect returns exact byte length (not rounded).
// append must commit to independent storage and never truncate/overwrite.
// read returns exactly one fixed record; single exclusive owner required.
// No default NVS fallback. The SD adapter must independently prove these
// guarantees before this ledger can enter learner firmware.
class CompletionBackend {
 public:
  virtual ~CompletionBackend()=default;
  virtual bool size(std::uint64_t& bytes)=0;
  virtual bool read(unsigned slot,CompletionBytes& bytes)=0;
  virtual bool append(unsigned expected_count,const CompletionBytes& bytes)=0;
};
enum class CompletionResult { Saved, AlreadyRecorded, Unavailable, Invalid, Full };
enum class CompletionLookup { NotRecorded, Recorded, Unavailable };

class CompletionLedger {
  // Preview-only bounded index. Allocate as static/owned object, not on loop stack.
  // Later SD-backed indexed generations must remove this engineering bound.
  static constexpr unsigned capacity=256;
  CompletionBackend& backend;
  std::array<CompletionKey,capacity> keys{};
  unsigned count=0;
  bool ready=false;
  bool fail(){
    ready=false;std::printf("ERR,CURRICULUM_COMPLETION_LEDGER\n");return false;
  }
  bool contains(const CompletionKey& k)const{
    for(unsigned i=0;i<count;++i)if(keys[i]==k)return true;
    return false;
  }
  bool unchanged_size(){
    std::uint64_t bytes=0;
    return backend.size(bytes)&&bytes==std::uint64_t(count)*CompletionBytes{}.size();
  }
 public:
  explicit CompletionLedger(CompletionBackend& storage):backend(storage){}
  CompletionLedger(const CompletionLedger&)=delete;
  CompletionLedger& operator=(const CompletionLedger&)=delete;
  bool begin(){
    ready=false;count=0;
    std::uint64_t size=0;
    if(!backend.size(size)||size%CompletionBytes{}.size()||
       size/CompletionBytes{}.size()>capacity)return fail();
    const auto records=static_cast<unsigned>(size/CompletionBytes{}.size());
    for(unsigned slot=0;slot<records;++slot){
      CompletionBytes bytes{};CompletionEvent e{};
      if(!backend.read(slot,bytes)||!completion_decode(bytes,e)||
         e.sequence!=slot+1||contains(e.key))return fail();
      keys[count++]=e.key;
    }
    if(!unchanged_size())return fail();
    ready=true;return true;
  }
  bool available()const{return ready;}
  unsigned verified_count()const{return ready?count:0;}
  CompletionLookup lookup(const Lesson& l)const{
    CompletionKey k{};
    if(!ready||!completion_key(l,k))return CompletionLookup::Unavailable;
    return contains(k)?CompletionLookup::Recorded:CompletionLookup::NotRecorded;
  }
  CompletionResult finish(const Preview& p){
    if(!ready)return CompletionResult::Unavailable;
    if(p.screen!=Screen::Finished||p.lesson>=lesson_count||
       p.step+1!=p.current().count)return CompletionResult::Invalid;
    CompletionKey k{};
    if(!completion_key(p.current(),k))return CompletionResult::Invalid;
    if(!unchanged_size()){fail();return CompletionResult::Unavailable;}
    if(contains(k))return CompletionResult::AlreadyRecorded;
    if(count>=capacity)return CompletionResult::Full;
    CompletionBytes record{},readback{};
    if(!completion_encode({count+1,k},record))return CompletionResult::Invalid;
    if(!backend.append(count,record)||!backend.read(count,readback)||record!=readback){
      fail();return CompletionResult::Unavailable;
    }
    // Check exact new length before acknowledgement. Ambiguous commits are
    // re-read on begin(); never append again based on stale in-memory state.
    std::uint64_t size=0;
    if(!backend.size(size)||size!=std::uint64_t(count+1)*record.size()){
      fail();return CompletionResult::Unavailable;
    }
    keys[count++]=k;return CompletionResult::Saved;
  }
};
}
