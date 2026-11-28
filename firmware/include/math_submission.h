#pragma once
#include "math_journal.h"

enum class MathSubmitStatus { Saved, Rejected, StorageError };
// Owned by the learner for its lifetime. No large temporary attempt on the
// touch callback's stack. A failed write leaves visible practice unchanged.
class MathSubmission {
  MathAttempt staged{};
 public:
  MathSubmitStatus submit(MathJournal& storage,MathPractice& state,
                          MathScratchpad& scratch,unsigned choice,MathResult& feedback) {
    if(!make_math_attempt(state,choice,scratch,staged)) return MathSubmitStatus::Rejected;
    auto next=state;
    if(!storage.append(staged,state,next)) return MathSubmitStatus::StorageError;
    state=next; feedback=staged.result;
    if(feedback!=MathResult::TryAgain) scratch_clear(scratch);
    return MathSubmitStatus::Saved;
  }
};
