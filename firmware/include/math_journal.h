#pragma once
#include "math_journal_partition.h"
#include <cstdio>
#include <cstring>
#include "math_snapshot.h"

enum class JournalLoad { Empty, Restored, Error };
enum class JournalFailure { None, Full, Storage, Conflict };
// Synthetic preview: retain V1 records and use physical slot numbers, not
// logical attempt IDs, after a legacy checkpoint migration.
class MathJournal {
  friend class MathJournalRestorer;
  friend class MathJournalMigration;
  const MathJournalTarget target;
  nvs_handle_t handle=0;
  bool opened=false, ready=false, exporting=false;
  unsigned last=0, limit=0;
  MathPractice current{}, origin{};
  // Single-owner journal: bounded persistent work buffers, not loop stack.
  AttemptBytes work{}, readback{};
  JournalFailure error=JournalFailure::None;
  static void key_for(unsigned slot,char (&key)[12]) {
    math_journal_key(slot,key);
  }
  bool fail(JournalFailure reason) {
    ready=false; error=reason;
    std::printf("ERR,MATH_JOURNAL,%u\n",static_cast<unsigned>(reason));
    return false;
  }
  // A missing key is distinct from I/O, type and length errors.
  static esp_err_t read(nvs_handle_t h,const char* key,void* dst,std::size_t& n) {
    return nvs_get_blob(h,key,dst,&n);
  }
  bool write(const char* key,const void* src,std::size_t n) {
    std::size_t absent=0;
    if(nvs_get_blob(handle,key,nullptr,&absent)!=ESP_ERR_NVS_NOT_FOUND)
      return fail(JournalFailure::Storage);
    auto result=nvs_set_blob(handle,key,src,n);
    if(result==ESP_ERR_NVS_NOT_ENOUGH_SPACE) return fail(JournalFailure::Full);
    if(result!=ESP_OK||nvs_commit(handle)!=ESP_OK) return fail(JournalFailure::Storage);
    std::size_t size=readback.size();
    if(read(handle,key,readback.data(),size)!=ESP_OK||size!=n||
       std::memcmp(src,readback.data(),n)!=0) return fail(JournalFailure::Storage);
    return true;
  }
  static esp_err_t checkpoint(nvs_handle_t h,const char* key,MathPractice& state) {
    MathCheckpoint bytes{}; auto n=bytes.size();
    const auto result=read(h,key,bytes.data(),n);
    if(result!=ESP_OK) return result;
    return n==bytes.size()&&math_decode(bytes,state)?ESP_OK:ESP_FAIL;
  }
  static esp_err_t legacy(MathPractice& state) {
    nvs_handle_t old=0;
    auto result=nvs_open("pocketlearn",NVS_READONLY,&old);
    if(result!=ESP_OK) return result;
    result=checkpoint(old,"mathv1",state); nvs_close(old);
    return result;
  }
  static bool same(const MathPractice& a,const MathPractice& b) {
    MathCheckpoint x{},y{};
    return math_encode(a,x)&&math_encode(b,y)&&x==y;
  }
 public:
  explicit MathJournal(MathJournalTarget storage= MathJournalTarget::Factory):target(storage) {}
  MathJournal(const MathJournal&)=delete;
  MathJournal& operator=(const MathJournal&)=delete;
  ~MathJournal() { if(opened) nvs_close(handle); }
  JournalFailure failure() const { return error; }
  JournalLoad begin(MathPractice& out) {
    if(exporting) return JournalLoad::Error;
    ready=false; last=0; error=JournalFailure::None;
    if(opened) { nvs_close(handle); opened=false; }
    auto bad=[this](JournalFailure reason) { fail(reason); return JournalLoad::Error; };
    const auto opened_result=math_journal_open(target,handle);
    if(opened_result!=ESP_OK) return bad(opened_result==ESP_ERR_NVS_NOT_ENOUGH_SPACE?
      JournalFailure::Full:JournalFailure::Storage);
    opened=true;
    if(!math_journal_limit(target,limit)) return bad(JournalFailure::Storage);
    const bool staging=target==MathJournalTarget::DedicatedStaging;
    if(staging&&!math_journal_staging_keys(limit)) return bad(JournalFailure::Conflict);
    MathPractice state, old;
    const auto base=checkpoint(handle,"basev1",state);
    // An isolated destination must never infer progress from default NVS.
    const auto prior=staging?ESP_ERR_NVS_NOT_FOUND:legacy(old);
    if((base!=ESP_OK&&base!=ESP_ERR_NVS_NOT_FOUND)||
       (prior!=ESP_OK&&prior!=ESP_ERR_NVS_NOT_FOUND)) return bad(JournalFailure::Storage);
    if(!staging&&base==ESP_OK&&(prior!=ESP_OK||!same(state,old))) return bad(JournalFailure::Conflict);
    origin=state;
    bool old_matches=prior!=ESP_OK||!old.attempts||same(state,old), gap=false;
    for(unsigned slot=1;slot<=limit;++slot) {
      char key[12]; key_for(slot,key);
      auto& bytes=work; bytes.fill(0); auto n=bytes.size();
      const auto result=read(handle,key,bytes.data(),n);
      if(result==ESP_ERR_NVS_NOT_FOUND) { gap=true; continue; }
      if(result!=ESP_OK||gap||!attempt_replay(bytes,n,state)) return bad(JournalFailure::Storage);
      last=slot;
      old_matches=old_matches||same(state,old);
    }
    if(base!=ESP_OK&&prior==ESP_OK&&old.attempts) {
      if(!last) {
        // Copy the summary, never manufacture missing historical attempts.
        MathCheckpoint bytes{};
        if(!math_encode(old,bytes)) return bad(JournalFailure::Storage);
        if(!write("basev1",bytes.data(),bytes.size())) return JournalLoad::Error;
        state=old; origin=old;
      } else if(!old_matches) return bad(JournalFailure::Conflict);
    }
    current=state; out=state; ready=true;
    return state.attempts?JournalLoad::Restored:JournalLoad::Empty;
  }
  // Only recovered journals can be exported. A failed append must be
  // reconciled by begin() first, even if it reported insufficient space.
  MathSnapshotResult snapshot(MathSnapshotSink& sink) {
    if(!ready||exporting) return MathSnapshotResult::Unavailable;
    exporting=true;
    struct Guard { bool& flag; ~Guard() { flag=false; } } guard{exporting};
    auto reject=[&sink](MathSnapshotResult reason) { sink.abort(); return reason; };
    MathSnapshot manifest{};
    manifest.count=last;
    MathSnapshotVerifier verifier;
    if(!math_encode(origin,manifest.base)||!math_encode(current,manifest.final)||
       !verifier.begin(manifest)) return MathSnapshotResult::Invalid;
    // Recheck the migrated base on disk, not just the cached checkpoint.
    MathPractice persisted;
    const auto base=checkpoint(handle,"basev1",persisted);
    if(origin.attempts?(base!=ESP_OK||!same(origin,persisted)):base!=ESP_ERR_NVS_NOT_FOUND)
      return MathSnapshotResult::Invalid;
    if(!sink.begin(manifest)) return reject(MathSnapshotResult::SinkFailure);
    for(unsigned slot=1;slot<=last;++slot) {
      char key[12]; key_for(slot,key);
      auto& bytes=work; bytes.fill(0); auto n=bytes.size();
      if(read(handle,key,bytes.data(),n)!=ESP_OK||!verifier.record(slot,bytes,n))
        return reject(MathSnapshotResult::Invalid);
      if(!sink.record(slot,bytes,n)) return reject(MathSnapshotResult::SinkFailure);
    }
    if(!verifier.complete()) return reject(MathSnapshotResult::Invalid);
    if(!sink.commit()) return reject(MathSnapshotResult::SinkFailure);
    return MathSnapshotResult::Complete;
  }
  bool append(const MathAttempt& a,const MathPractice& before,MathPractice& out) {
    if(exporting||!ready||!same(before,current)||before.attempts==UINT32_MAX||a.sequence!=before.attempts+1)
      return false;
    if(last>=limit) return fail(JournalFailure::Full);
    auto& bytes=work; std::size_t n=0; auto next=before;
    if(!attempt_encode(a,bytes,n)||!attempt_replay(bytes,n,next)) return false;
    char key[12]; key_for(last+1,key);
    if(!write(key,bytes.data(),n)) return false;
    ++last; current=next; out=next;
    return true;
  }
};
