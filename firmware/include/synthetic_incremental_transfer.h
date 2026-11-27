#pragma once
#include "math_journal_export_cursor.h"
#include "math_frame_snapshot.h"
#include "learner_protected_frame.h"

namespace learner_auth {
// Internal synthetic integration only, not an exposed USB command.
// Caller owns a freshly authenticated encrypted session and a fake journal
// in tests. A production dispatcher needs separate identity/teacher grants,
// exclusive learner maintenance mode and a disconnect-aware frame writer.
class SyntheticIncrementalTransfer {
 public:
  SyntheticIncrementalTransfer(MathJournal& journal,ProtectedFrameSession& session,
                               SnapshotFrameWriter& writer):
    sink_(session,writer),cursor_(journal,sink_),writer_(writer){}
  SyntheticIncrementalTransfer(const SyntheticIncrementalTransfer&)=delete;
  SyntheticIncrementalTransfer& operator=(const SyntheticIncrementalTransfer&)=delete;
  ~SyntheticIncrementalTransfer(){cancel();}
  bool start(const uint8_t* frame,std::size_t n,bool synthetic_only){
    if(used_){cancel();return false;}
    used_=true;
    if(!sink_.begin_request(frame,n,synthetic_only)){
      failed_=true;return false;
    }
    if(!cursor_.start(writer_.now())){
      sink_.abort();failed_=true;return false;
    }
    return true;
  }
  MathExportState poll(bool ready){
    if(failed_)return MathExportState::Failed;
    const auto state=cursor_.poll(writer_.now(),ready);
    if(state==MathExportState::Complete)sink_.finish_request();
    return state;
  }
  // Complete means terminal ciphertext queued, not physical delivery or
  // durable host receipt. The writer must separately finish/discard output.
  void cancel(){
    cursor_.cancel();
    if(used_){sink_.abort();failed_=true;}
  }
 private:
  MathFrameSink<ProtectedFrameSession> sink_;
  MathJournalExportCursor cursor_;
  SnapshotFrameWriter& writer_;
  bool used_=false,failed_=false;
};
}
