#pragma once
#include <optional>
#include "curriculum_completion_file.h"

namespace curriculum_preview {
// Exclusive lifetime owner for a developer evaluation ledger. Caller must
// already own the mounted filesystem; use a fixed application-only path.
// Never overwrite, truncate, repair, or use the public probe fixture.
class CompletionOwner {
  int fd=-1;
  bool attempted=false;
  std::optional<CompletionFile> file;
  std::optional<CompletionLedger> ledger;
  bool fail(const char* why){
    std::printf("ERR,CURRICULUM_OWNER,%s\n",why);
    return false;
  }
 public:
  CompletionOwner()=default;
  CompletionOwner(const CompletionOwner&)=delete;
  CompletionOwner& operator=(const CompletionOwner&)=delete;
  ~CompletionOwner(){close_owner();}
  bool open_once(const char* path){
    if(attempted||!path||!*path)return fail("invalid-open");
    attempted=true;
    // Existing evidence must replay successfully before exposing a view.
    // Only ENOENT permits exclusive creation; no fallback on other errors.
    fd=::open(path,O_RDWR|O_APPEND);
    if(fd<0&&errno==ENOENT){
      fd=::open(path,O_CREAT|O_EXCL|O_RDWR|O_APPEND,0600);
      if(fd>=0&&fsync(fd)!=0){close_owner();return fail("create-sync");}
    }
    if(fd<0)return fail("open");
    file.emplace(fd);
    if(!file->begin()){close_owner();return fail("file");}
    ledger.emplace(*file);
    if(!ledger->begin()){close_owner();return fail("replay");}
    return true;
  }
  CompletionLedger* view(){return ledger&&ledger->available()?&*ledger:nullptr;}
  bool close_owner(){
    ledger.reset();file.reset();
    if(fd<0)return true;
    const int old=fd;fd=-1;
    return ::close(old)==0||fail("close");
  }
};
}
