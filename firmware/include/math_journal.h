#pragma once
#include <Preferences.h>
#include <cstdio>
#include "math_attempt_codec.h"

enum class JournalLoad { Empty, Restored, Error };
// Synthetic preview only. Append one key per attempt. No eviction or overwrite.
// A bounded journal deliberately fails closed when NVS is full.
class MathJournal {
  Preferences prefs;
  bool ready=false;
  unsigned last=0;
  static constexpr unsigned limit=32;
  static void key_for(unsigned seq,char (&key)[12]) {
    snprintf(key,sizeof(key),"a%03u",seq);
  }
 public:
  JournalLoad begin(MathPractice& out) {
    ready=false;
    last=0;
    if(!prefs.begin("pljournal",false)) return JournalLoad::Error;
    MathPractice state;
    bool gap=false;
    for(unsigned seq=1;seq<=limit;++seq) {
      char key[12]; key_for(seq,key);
      if(!prefs.isKey(key)) { gap=true; continue; }
      if(gap) return JournalLoad::Error;
      AttemptBytes bytes{};
      const auto n=prefs.getBytesLength(key);
      if(n<28||n>bytes.size()||prefs.getBytes(key,bytes.data(),n)!=n||
         !attempt_replay(bytes,n,state)||state.attempts!=seq) return JournalLoad::Error;
      last=seq;
    }
    out=state;
    ready=true;
    return last?JournalLoad::Restored:JournalLoad::Empty;
  }
  bool append(const MathAttempt& a,const MathPractice& before,MathPractice& out) {
    if(!ready||last>=limit||a.sequence!=last+1||before.attempts!=last) return false;
    AttemptBytes bytes{},verify{};
    std::size_t n=0;
    auto next=before;
    if(!attempt_encode(a,bytes,n)||!attempt_replay(bytes,n,next)) return false;
    char key[12]; key_for(a.sequence,key);
    if(prefs.isKey(key)||prefs.putBytes(key,bytes.data(),n)!=n||
       prefs.getBytes(key,verify.data(),n)!=n||verify!=bytes) {
      ready=false;
      return false;
    }
    last=a.sequence;
    out=next;
    return true;
  }
};
