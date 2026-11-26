#pragma once
#include "math_attempt_codec.h"

// Internal preview snapshot schema. Transport authorization and persistent
// backup format are separate gates. A nonzero base is summary-only history.
struct MathSnapshot {
  MathCheckpoint base{}, final{};
  std::uint32_t count=0;
};
enum class MathSnapshotResult { Complete, Unavailable, Invalid, SinkFailure };

// A sink stages all bytes privately. It must publish only on successful commit;
// abort discards staging, including after failed begin/commit. No reentrant I/O.
class MathSnapshotSink {
 public:
  virtual ~MathSnapshotSink()=default;
  virtual bool begin(const MathSnapshot& manifest) noexcept=0;
  virtual bool record(std::uint32_t slot,const AttemptBytes& bytes,std::size_t n) noexcept=0;
  virtual bool commit() noexcept=0;
  virtual void abort() noexcept=0;
};

// Reuse the V1 replay rules. A checkpoint cannot stand in for missing ink.
class MathSnapshotVerifier {
  MathPractice state{};
  MathCheckpoint expected{};
  std::uint32_t count=0, seen=0;
  bool valid=false;
 public:
  bool begin(const MathSnapshot& manifest) {
    valid=false; seen=0;
    MathPractice first,last;
    if(!math_decode(manifest.base,first)||!math_decode(manifest.final,last)||
       last.attempts<first.attempts||last.attempts-first.attempts!=manifest.count) return false;
    state=first; expected=manifest.final; count=manifest.count; valid=true;
    return true;
  }
  bool record(std::uint32_t slot,const AttemptBytes& bytes,std::size_t n) {
    if(!valid||seen>=count||slot!=seen+1||!attempt_replay(bytes,n,state)) {
      valid=false; return false;
    }
    ++seen; return true;
  }
  bool complete() const {
    MathCheckpoint actual{};
    return valid&&seen==count&&math_encode(state,actual)&&actual==expected;
  }
};
