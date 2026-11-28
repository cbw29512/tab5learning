#pragma once
#include "curriculum_resume_codec.h"
#include "curriculum_completion_ledger.h"

namespace curriculum_preview {
// Native UI stages its own valid navigation first. Commit only when
// the next screen is ready to be shown. The optional ledger is explicitly
// disabled in the normal preview until physical SD acceptance.
enum class TransitionResult { Applied, ResumeError, CompletionError };
template<class Checkpoint>
TransitionResult commit_transition(Preview& current,const Preview& candidate,
                                   Checkpoint& resume,CompletionLedger* completion=nullptr){
  // A review is transient and must not replace the active lesson checkpoint.
  if(candidate.reviewing){
    if(candidate.lesson>=lesson_count||candidate.step>=candidate.current().count||
       !completion||completion->lookup(candidate.current())!=CompletionLookup::Recorded){
      std::printf("ERR,CURRICULUM_REVIEW_UNVERIFIED\n");
      return TransitionResult::CompletionError;
    }
    current=candidate;return TransitionResult::Applied;
  }
  const bool checkpoint=candidate.screen==Screen::Activity||candidate.screen==Screen::Finished;
  if(checkpoint){
    // Verify the checkpoint shape before any completion write.
    ResumeBytes encoded{};
    if(!resume_encode(candidate,encoded)){
      std::printf("ERR,CURRICULUM_TRANSITION_INVALID\n");
      return TransitionResult::ResumeError;
    }
    if(candidate.screen==Screen::Finished&&completion){
      // Ledger first: a power cut between these writes may repeat the
      // last activity, but duplicate completion is safely idempotent.
      // Never infer completion from a previous resume record.
      const auto result=completion->finish(candidate);
      if(result!=CompletionResult::Saved&&result!=CompletionResult::AlreadyRecorded){
        std::printf("ERR,CURRICULUM_COMPLETION_NOT_SAVED\n");
        return TransitionResult::CompletionError;
      }
    }
    if(!resume.save(candidate)){
      std::printf("ERR,CURRICULUM_ADVANCE_NOT_SAVED\n");
      return TransitionResult::ResumeError;
    }
  }
  current=candidate;
  return TransitionResult::Applied;
}
}
