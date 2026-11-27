#pragma once
#include "math_journal.h"

// Maintenance-mode export: one record per caller poll, no new storage writes.
// Journal and sink MUST outlive this cursor. Single-threaded, non-reentrant
// caller only. A sink may queue one frame; poll only when its queue is free.
enum class MathExportState { Idle, Manifest, Records, Terminal, Complete, Failed };

class MathJournalExportCursor {
 public:
  MathJournalExportCursor(MathJournal& journal,MathSnapshotSink& sink):
    journal_(journal),sink_(sink){}
  MathJournalExportCursor(const MathJournalExportCursor&)=delete;
  MathJournalExportCursor& operator=(const MathJournalExportCursor&)=delete;
  ~MathJournalExportCursor(){cancel();}
  MathExportState state()const{return state_;}
  MathSnapshotResult result()const{return result_;}
  bool active()const{return owned_;}
  bool start(uint32_t now){
    if(state_!=MathExportState::Idle||!journal_.ready||journal_.exporting)return false;
    journal_.exporting=true;owned_=true;started_=now;
    manifest_.count=journal_.last;
    MathPractice persisted;
    const auto base=journal_.checkpoint(journal_.handle,"basev1",persisted);
    if(!math_encode(journal_.origin,manifest_.base)||
       !math_encode(journal_.current,manifest_.final)||!verifier_.begin(manifest_)||
       (journal_.origin.attempts?(base!=ESP_OK||!journal_.same(journal_.origin,persisted)):
        base!=ESP_ERR_NVS_NOT_FOUND)){
      finish(MathSnapshotResult::Invalid);return false;
    }
    state_=MathExportState::Manifest;return true;
  }
  // Call even when the serial queue is full to enforce the deadline.
  // Cancellation invalidates the whole attempt; reconnect starts fresh.
  MathExportState poll(uint32_t now,bool sink_ready){
    if(!owned_)return state_;
    if(uint32_t(now-started_)>=110000){finish(MathSnapshotResult::Unavailable);return state_;}
    if(!sink_ready)return state_;
    switch(state_){
      case MathExportState::Manifest:
        if(!sink_.begin(manifest_))finish(MathSnapshotResult::SinkFailure);
        else state_=manifest_.count?MathExportState::Records:MathExportState::Terminal;
        break;
      case MathExportState::Records:{
        char key[12];journal_.key_for(slot_,key);
        auto& bytes=journal_.work;bytes.fill(0);auto size=bytes.size();
        if(journal_.read(journal_.handle,key,bytes.data(),size)!=ESP_OK||
           !verifier_.record(slot_,bytes,size)){
          finish(MathSnapshotResult::Invalid);break;
        }
        if(!sink_.record(slot_,bytes,size)){finish(MathSnapshotResult::SinkFailure);break;}
        if(slot_++==manifest_.count)state_=MathExportState::Terminal;
        break;
      }
      case MathExportState::Terminal:
        if(!verifier_.complete())finish(MathSnapshotResult::Invalid);
        else if(!sink_.commit())finish(MathSnapshotResult::SinkFailure);
        else finish(MathSnapshotResult::Complete);
        break;
      default:finish(MathSnapshotResult::Unavailable);break;
    }
    return state_;
  }
  void cancel(){if(owned_)finish(MathSnapshotResult::Unavailable);}
 private:
  void finish(MathSnapshotResult result){
    result_=result;
    if(result!=MathSnapshotResult::Complete){
      sink_.abort();
      std::printf("ERR,MATH_EXPORT,%u\n",static_cast<unsigned>(result));
    }
    journal_.work.fill(0);
    journal_.exporting=false;owned_=false;
    state_=result==MathSnapshotResult::Complete?MathExportState::Complete:MathExportState::Failed;
  }
  MathJournal& journal_;MathSnapshotSink& sink_;
  MathSnapshot manifest_{};MathSnapshotVerifier verifier_;
  MathExportState state_=MathExportState::Idle;
  MathSnapshotResult result_=MathSnapshotResult::Unavailable;
  uint32_t started_=0,slot_=1;bool owned_=false;
};
