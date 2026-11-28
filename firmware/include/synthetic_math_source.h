#pragma once
#include "math_frame_snapshot.h"

namespace learner_auth {
// Deterministic public sample only. No NVS, SD, student identifiers or device
// credentials are accessed. Reuses exact attempt codec and snapshot verifier.
class SyntheticMathSource {
 public:
  MathSnapshotResult snapshot(MathSnapshotSink& sink){
    MathPractice state{};MathSnapshot manifest{};
    manifest.count=2;
    if(!math_encode(state,manifest.base))return MathSnapshotResult::Invalid;
    scratch_clear(scratch_);
    if(!scratch_touch(scratch_,656,184,true)||
       !make_math_attempt(state,0,scratch_,attempt_)||
       !attempt_encode(attempt_,first_,first_len_)||
       !attempt_replay(first_,first_len_,state))return MathSnapshotResult::Invalid;
    scratch_clear(scratch_);
    if(!make_math_attempt(state,1,scratch_,attempt_)||
       !attempt_encode(attempt_,second_,second_len_)||
       !attempt_replay(second_,second_len_,state)||
       !math_encode(state,manifest.final))return MathSnapshotResult::Invalid;
    MathSnapshotVerifier verifier;
    if(!verifier.begin(manifest)||!verifier.record(1,first_,first_len_)||
       !verifier.record(2,second_,second_len_)||!verifier.complete())
      return MathSnapshotResult::Invalid;
    if(!sink.begin(manifest)||!sink.record(1,first_,first_len_)||
       !sink.record(2,second_,second_len_)||!sink.commit()){
      sink.abort();return MathSnapshotResult::SinkFailure;
    }
    return MathSnapshotResult::Complete;
  }
 private:
  MathScratchpad scratch_{};MathAttempt attempt_{};
  AttemptBytes first_{},second_{};
  std::size_t first_len_=0,second_len_=0;
};
}
