#pragma once
#include <nvs.h>
#include <cstdio>
#include <cstring>
#include "math_attempt_codec.h"

enum class JournalLoad { Empty, Restored, Error };
enum class JournalFailure { None, Full, Storage, Conflict };
// Synthetic preview: retain V1 records and use physical slot numbers, not
// logical attempt IDs, after a legacy checkpoint migration.
class MathJournal {
  nvs_handle_t handle=0;
  bool opened=false, ready=false;
  unsigned last=0, limit=0;
  MathPractice current{};
  JournalFailure error=JournalFailure::None;
  static void key_for(unsigned slot,char (&key)[12]) {
    snprintf(key,sizeof(key),"a%03u",slot);
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
    AttemptBytes verify{}; std::size_t size=verify.size();
    if(read(handle,key,verify.data(),size)!=ESP_OK||size!=n||
       std::memcmp(src,verify.data(),n)!=0) return fail(JournalFailure::Storage);
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
  MathJournal()=default;
  MathJournal(const MathJournal&)=delete;
  MathJournal& operator=(const MathJournal&)=delete;
  ~MathJournal() { if(opened) nvs_close(handle); }
  JournalFailure failure() const { return error; }
  JournalLoad begin(MathPractice& out) {
    ready=false; last=0; error=JournalFailure::None;
    if(opened) { nvs_close(handle); opened=false; }
    auto bad=[this](JournalFailure reason) { fail(reason); return JournalLoad::Error; };
    const auto opened_result=nvs_open("pljournal",NVS_READWRITE,&handle);
    if(opened_result!=ESP_OK) return bad(opened_result==ESP_ERR_NVS_NOT_ENOUGH_SPACE?
      JournalFailure::Full:JournalFailure::Storage);
    opened=true;
    nvs_stats_t stats{};
    if(nvs_get_stats(nullptr,&stats)!=ESP_OK||!stats.total_entries||stats.total_entries>1000000)
      return bad(JournalFailure::Storage);
    // Every attempt needs at least one entry; this bounds gap scanning by
    // the actual partition, not an arbitrary attempt count.
    limit=static_cast<unsigned>(stats.total_entries);
    MathPractice state, old;
    const auto base=checkpoint(handle,"basev1",state);
    const auto prior=legacy(old);
    if((base!=ESP_OK&&base!=ESP_ERR_NVS_NOT_FOUND)||
       (prior!=ESP_OK&&prior!=ESP_ERR_NVS_NOT_FOUND)) return bad(JournalFailure::Storage);
    if(base==ESP_OK&&(prior!=ESP_OK||!same(state,old))) return bad(JournalFailure::Conflict);
    bool old_matches=prior!=ESP_OK||!old.attempts||same(state,old), gap=false;
    for(unsigned slot=1;slot<=limit;++slot) {
      char key[12]; key_for(slot,key);
      AttemptBytes bytes{}; auto n=bytes.size();
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
        state=old;
      } else if(!old_matches) return bad(JournalFailure::Conflict);
    }
    current=state; out=state; ready=true;
    return state.attempts?JournalLoad::Restored:JournalLoad::Empty;
  }
  bool append(const MathAttempt& a,const MathPractice& before,MathPractice& out) {
    if(!ready||!same(before,current)||before.attempts==UINT32_MAX||a.sequence!=before.attempts+1)
      return false;
    if(last>=limit) return fail(JournalFailure::Full);
    AttemptBytes bytes{}; std::size_t n=0; auto next=before;
    if(!attempt_encode(a,bytes,n)||!attempt_replay(bytes,n,next)) return false;
    char key[12]; key_for(last+1,key);
    if(!write(key,bytes.data(),n)) return false;
    ++last; current=next; out=next;
    return true;
  }
};
