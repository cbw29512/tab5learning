#pragma once
#include "math_journal.h"

// Internal synthetic candidate, never a USB command. The caller must own
// a validated, immutable staged buffer for the entire synchronous operation.
// Each record is prefixed by its little-endian uint16 byte length (PLMB V1).
struct MathRestoreImage {
  MathSnapshot manifest{};
  const std::uint8_t* data=nullptr;
  std::size_t size=0;
};
enum class MathRestoreResult { Complete, Unavailable, Invalid, Conflict, Stale, Full, Interrupted };

class MathJournalRestorer {
  static constexpr unsigned max_records=math_journal_max_records;
  static bool next(const MathRestoreImage& image,std::size_t& pos,
                   AttemptBytes& bytes,std::size_t& n) {
    if(pos>image.size||image.size-pos<2) return false;
    n=image.data[pos]+256u*image.data[pos+1]; pos+=2;
    if(n<28||n>bytes.size()||n>image.size-pos) return false;
    bytes.fill(0); std::memcpy(bytes.data(),image.data+pos,n); pos+=n;
    return true;
  }
 public:
  static MathRestoreResult apply(MathJournal& journal,const MathRestoreImage& image,
                                 MathPractice& out) {
    using R=MathRestoreResult;
    if(!journal.ready||journal.exporting) return R::Unavailable;
    if(image.manifest.count>max_records||image.size>max_records*(2+AttemptBytes{}.size())||
       (image.size&&!image.data)) return R::Invalid;
    journal.exporting=true;
    struct Guard { bool& flag; ~Guard() { flag=false; } } guard{journal.exporting};
    MathSnapshotVerifier verifier;
    if(!verifier.begin(image.manifest)) return R::Invalid;
    MathCheckpoint origin{}, saved{};
    if(!math_encode(journal.origin,origin)||!math_encode(journal.current,saved)) return R::Invalid;
    // Never replace a legacy base or invent lost evidence on an empty device.
    if(origin!=image.manifest.base) return R::Conflict;
    MathPractice persisted;
    const auto base=journal.checkpoint(journal.handle,"basev1",persisted);
    if(journal.origin.attempts?(base!=ESP_OK||!journal.same(journal.origin,persisted)):
       base!=ESP_ERR_NVS_NOT_FOUND) return R::Conflict;

    // Preflight the complete staged history and the exact stored prefix.
    std::size_t pos=0, suffix=0;
    for(unsigned slot=1;slot<=image.manifest.count;++slot) {
      auto& bytes=journal.work; std::size_t n=0;
      if(!next(image,pos,bytes,n)||!verifier.record(slot,bytes,n)) return R::Invalid;
      if(slot<=journal.last) {
        char key[12]; journal.key_for(slot,key);
        auto& existing=journal.readback; existing.fill(0); auto length=existing.size();
        if(journal.read(journal.handle,key,existing.data(),length)!=ESP_OK||
           length!=n||existing!=bytes) return R::Conflict;
        suffix=pos;
        if(slot==journal.last&&std::memcmp(bytes.data()+8,saved.data(),saved.size())!=0)
          return R::Conflict;
      }
    }
    if(pos!=image.size||!verifier.complete()) return R::Invalid;
    if(image.manifest.count<journal.last) return R::Stale;
    if(image.manifest.count>journal.limit) return R::Full;
    if(!journal.last&&origin!=saved) return R::Conflict;

    // Restore only the missing suffix. Each NVS write is independently
    // committed/read back. A partial restore is a valid, resumable prefix;
    // an ambiguous write locks the journal until begin() reconciles it.
    pos=suffix;
    for(unsigned slot=journal.last+1;slot<=image.manifest.count;++slot) {
      auto& bytes=journal.work; std::size_t n=0;
      auto state=journal.current;
      if(!next(image,pos,bytes,n)||!attempt_replay(bytes,n,state)) {
        journal.fail(JournalFailure::Conflict); return R::Invalid;
      }
      char key[12]; journal.key_for(slot,key);
      if(!journal.write(key,bytes.data(),n))
        return journal.failure()==JournalFailure::Full?R::Full:R::Interrupted;
      journal.last=slot; journal.current=state; out=state;
    }
    MathCheckpoint terminal{};
    if(pos!=image.size||!math_encode(journal.current,terminal)||terminal!=image.manifest.final) {
      journal.fail(JournalFailure::Conflict); return R::Invalid;
    }
    out=journal.current; return R::Complete;
  }
};
