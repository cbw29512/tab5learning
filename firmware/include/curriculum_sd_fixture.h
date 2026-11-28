#pragma once
#include "curriculum_completion_file.h"

namespace curriculum_preview {
// Fixed public development fixture. Replaying it must never look like an
// assessment. Exact bytes checked, not merely valid CRCs or record count.
inline bool probe_completed(unsigned subject,Preview& out){
  Preview p{};
  if(!p.choose_subject(subject)||!p.choose_lesson(0))return false;
  for(unsigned tick=0;tick<128;++tick){
    if(p.screen==Screen::Finished){out=p;return true;}
    if(p.screen==Screen::Activity&&p.activity().kind==Kind::Choice){
      if(!p.answer(0))return false;
    }else if(!p.next())return false;
  }
  return false;
}
inline bool probe_expected(unsigned slot,CompletionBytes& out){
  if(slot>=2)return false;
  Preview p{};CompletionKey k{};
  return probe_completed(slot,p)&&completion_key(p.current(),k)&&
    completion_encode({slot+1,k},out);
}
inline bool probe_exact(CompletionBackend& file){
  std::uint64_t n=0;
  if(!file.size(n)||n!=192)return false;
  for(unsigned slot=0;slot<2;++slot){
    CompletionBytes expected{},actual{};
    if(!probe_expected(slot,expected)||!file.read(slot,actual)||expected!=actual)return false;
  }
  return true;
}
// Only a file exclusively created during this invocation may be written.
// Existing files are never appended to, even if they are empty or partial.
inline bool probe_initialize(CompletionBackend& file,CompletionLedger& ledger){
  if(!ledger.begin()||ledger.verified_count()!=0)return false;
  for(unsigned subject=0;subject<2;++subject){
    Preview p{};
    if(!probe_completed(subject,p)||ledger.finish(p)!=CompletionResult::Saved||
       ledger.finish(p)!=CompletionResult::AlreadyRecorded)return false;
  }
  return ledger.verified_count()==2&&probe_exact(file);
}
}
