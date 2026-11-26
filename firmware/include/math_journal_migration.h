#pragma once
#include "math_journal.h"

// Internal offline maintenance only. Both journals must be exclusively owned
// by the caller and recovered by begin() before this synchronous operation.
// Complete means a verified staging copy, NOT an active storage cutover.
enum class MathMigrationResult { Complete, Unavailable, InvalidSource, Conflict, Full, Interrupted };

class MathJournalMigration {
  using R=MathMigrationResult;
  static bool base_matches(MathJournal& journal) {
    MathPractice persisted;
    const auto result=journal.checkpoint(journal.handle,"basev1",persisted);
    return journal.origin.attempts?
      result==ESP_OK&&journal.same(journal.origin,persisted):
      result==ESP_ERR_NVS_NOT_FOUND;
  }
  static bool read_record(MathJournal& journal,unsigned slot,std::size_t& n) {
    char key[12]; journal.key_for(slot,key);
    journal.work.fill(0); n=journal.work.size();
    return journal.read(journal.handle,key,journal.work.data(),n)==ESP_OK;
  }
  static R write_failed(MathJournal& dest) {
    return dest.failure()==JournalFailure::Full?R::Full:R::Interrupted;
  }
 public:
  static R copy(MathJournal& source,MathJournal& dest,MathPractice& out) {
    if(&source==&dest||source.target!=MathJournalTarget::Factory||
       dest.target!=MathJournalTarget::DedicatedStaging||
       !source.ready||!dest.ready||source.exporting||dest.exporting) return R::Unavailable;
    source.exporting=dest.exporting=true;
    struct Guard {
      bool& source; bool& dest;
      ~Guard() { source=dest=false; }
    } guard{source.exporting,dest.exporting};
    if(!base_matches(source)) return R::InvalidSource;
    if(!base_matches(dest)||!math_journal_staging_keys(dest.limit)) return R::Conflict;
    if(dest.sealed&&(source.last!=dest.last||!source.same(source.origin,dest.origin))) return R::Conflict;
    if(source.last>dest.limit) return R::Full;
    if(dest.last>source.last) return R::Conflict;
    // Only a completely empty destination may adopt a nonzero legacy base.
    const bool adopt=!dest.last&&!dest.origin.attempts&&source.origin.attempts;
    if(!adopt&&!source.same(source.origin,dest.origin)) return R::Conflict;
    if(!dest.last&&!dest.same(dest.origin,dest.current)) return R::Conflict;
    MathSnapshot manifest{};
    manifest.count=source.last;
    MathSnapshotVerifier verify;
    if(!math_encode(source.origin,manifest.base)||!math_encode(source.current,manifest.final)||
       !verify.begin(manifest)) return R::InvalidSource;
    // Preflight ALL source records and the existing destination prefix
    // before writing a single byte. Exact record bytes include ink evidence.
    auto replay=source.origin;
    for(unsigned slot=1;slot<=source.last;++slot) {
      std::size_t n=0;
      if(!read_record(source,slot,n)||!verify.record(slot,source.work,n)||
         !attempt_replay(source.work,n,replay)) return R::InvalidSource;
      if(slot<=dest.last) {
        std::size_t existing=0;
        if(!read_record(dest,slot,existing)||existing!=n||source.work!=dest.work)
          return R::Conflict;
        if(slot==dest.last&&!dest.same(replay,dest.current)) return R::Conflict;
      }
    }
    if(!verify.complete()) return R::InvalidSource;
    // Persist the exact base; never fabricate the missing legacy evidence.
    if(adopt) {
      if(!dest.write("basev1",manifest.base.data(),manifest.base.size())) return write_failed(dest);
      dest.origin=dest.current=source.origin;
    }
    // Every new record is committed and read back by the shared journal
    // primitive. A power loss leaves a replayable prefix, not a cutover.
    for(unsigned slot=dest.last+1;slot<=source.last;++slot) {
      std::size_t n=0; auto state=dest.current;
      if(!read_record(source,slot,n)||!attempt_replay(source.work,n,state)) {
        dest.fail(JournalFailure::Conflict); return R::InvalidSource;
      }
      char key[12]; dest.key_for(slot,key);
      if(!dest.write(key,source.work.data(),n)) return write_failed(dest);
      dest.last=slot; dest.current=state;
    }
    // Revalidate all destination bytes and terminal checkpoint before
    // reporting success. Do not update the caller's output on failure.
    if(!base_matches(dest)||!dest.same(source.current,dest.current)||
       !verify.begin(manifest)) {
      dest.fail(JournalFailure::Conflict); return R::Conflict;
    }
    for(unsigned slot=1;slot<=source.last;++slot) {
      std::size_t n=0,existing=0;
      if(!read_record(source,slot,n)||!read_record(dest,slot,existing)||
         n!=existing||source.work!=dest.work||!verify.record(slot,dest.work,existing)) {
        dest.fail(JournalFailure::Conflict); return R::Conflict;
      }
    }
    if(!verify.complete()) { dest.fail(JournalFailure::Conflict); return R::Conflict; }
    out=dest.current;
    return R::Complete;
  }
  // Seal only a verified, exact copy. A failed receipt commit requires
  // begin() to reconcile; an existing receipt is never overwritten.
  static R copy_and_seal(MathJournal& source,MathJournal& dest,MathPractice& out) {
    MathPractice candidate;
    const auto result=copy(source,dest,candidate);
    if(result!=R::Complete) return result;
    MathSnapshot m{}; m.count=dest.last;
    if(!math_encode(dest.origin,m.base)||!math_encode(dest.current,m.final)) return R::Conflict;
    MathCopyDigest digest; MathSnapshotVerifier verify;
    if(!verify.begin(m)) return R::Conflict;
    for(unsigned slot=1;slot<=dest.last;++slot) {
      std::size_t n=0;
      if(!read_record(dest,slot,n)||!verify.record(slot,dest.work,n)) {
        dest.fail(JournalFailure::Conflict); return R::Conflict;
      }
      digest.add(dest.work,n);
    }
    if(!verify.complete()) return R::Conflict;
    const auto expected=math_copy_receipt(m,digest.value());
    if(dest.sealed) {
      MathCopyReceipt stored{}; auto n=stored.size();
      if(dest.read(dest.handle,"copyv1",stored.data(),n)!=ESP_OK||
         n!=stored.size()||stored!=expected) {
        dest.fail(JournalFailure::Conflict); return R::Conflict;
      }
    } else {
      if(!dest.write("copyv1",expected.data(),expected.size())) return write_failed(dest);
      dest.sealed=true;
    }
    out=candidate; return R::Complete;
  }
};
