#pragma once
#include <cerrno>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include "curriculum_completion_ledger.h"

namespace curriculum_preview {
// POSIX reference backend. Caller owns one already-open regular file and
// excludes every other writer for its lifetime. No path access, creation,
// formatting, truncation or repair. Requires working fstat, lseek, write,
// read, fsync and O_APPEND; validate these separately on the ESP VFS/FAT port.
class CompletionFile final:public CompletionBackend {
  const int fd;
  bool valid=false;
  bool fail(const char* operation){
    valid=false;std::printf("ERR,CURRICULUM_FILE,%s\n",operation);return false;
  }
 public:
  explicit CompletionFile(int descriptor):fd(descriptor){}
  CompletionFile(const CompletionFile&)=delete;
  CompletionFile& operator=(const CompletionFile&)=delete;
  bool begin(){
    valid=false;
    const int flags=fcntl(fd,F_GETFL);
    struct stat info{};
    if(flags<0||(flags&O_ACCMODE)!=O_RDWR||!(flags&O_APPEND)||
       fstat(fd,&info)!=0||!S_ISREG(info.st_mode)||info.st_size<0)return fail("open");
    valid=true;return true;
  }
  bool size(std::uint64_t& bytes)override{
    struct stat info{};
    if(!valid||fstat(fd,&info)!=0||!S_ISREG(info.st_mode)||info.st_size<0)return fail("size");
    bytes=static_cast<std::uint64_t>(info.st_size);return true;
  }
  bool read(unsigned slot,CompletionBytes& bytes)override{
    const auto offset=std::uint64_t(slot)*bytes.size();
    std::uint64_t length=0;
    if(slot>=256||!size(length)||offset+bytes.size()>length||
       lseek(fd,static_cast<off_t>(offset),SEEK_SET)!=static_cast<off_t>(offset))return fail("seek");
    CompletionBytes candidate{};
    std::size_t done=0;
    while(done<candidate.size()){
      const auto n=::read(fd,candidate.data()+done,candidate.size()-done);
      if(n<=0)return fail("read"); // bounded, including interrupted I/O
      done+=static_cast<std::size_t>(n);
    }
    bytes=candidate;return true;
  }
  bool append(unsigned expected,const CompletionBytes& bytes)override{
    std::uint64_t length=0;
    CompletionEvent e{};
    if(expected>=256||!completion_decode(bytes,e)||e.sequence!=expected+1||
       !size(length)||length!=std::uint64_t(expected)*bytes.size())return fail("append-precondition");
    // One append request. Any short write is ambiguous; never retry the
    // remainder or erase a partial tail. Replay after reopen is mandatory.
    if(::write(fd,bytes.data(),bytes.size())!=static_cast<ssize_t>(bytes.size()))
      return fail("write");
    if(fsync(fd)!=0)return fail("sync");
    if(!size(length)||length!=std::uint64_t(expected+1)*bytes.size())return fail("length");
    return true; // ledger separately reads back exact bytes
  }
};
}
