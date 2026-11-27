#pragma once
#include "math_continuous_record.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace math_continuous {
// A single exclusive caller-owned O_RDWR|O_APPEND file. No creation,
// truncation, repair or automatic storage selection. PMA1 records frame
// themselves with the ink count at offset 18. No RAM-sized history index.
enum class JournalState { Idle, Replaying, Ready, Failed };
enum class SaveResult { Saved, Rejected, Full, Unavailable };
class Journal {
  const int fd;
  JournalState status=JournalState::Idle;
  std::uint64_t cursor=0,end=0;
  State current{};
  Record staged{},check{}; // static owner on target, never loop stack
  // Candidate segment bound, not lifetime retention. Rotation is a later gate.
  static constexpr std::uint64_t max_bytes=64ULL*1024*1024;
  bool fail(const char* why){
    status=JournalState::Failed;
    std::printf("ERR,MATH_CONTINUOUS_JOURNAL,%s\n",why);return false;
  }
  bool size(std::uint64_t& n){
    struct stat st{};
    if(fstat(fd,&st)!=0||!S_ISREG(st.st_mode)||st.st_size<0)return false;
    n=static_cast<std::uint64_t>(st.st_size);return true;
  }
  bool exact(std::uint8_t* p,std::size_t n){
    while(n){
      const auto got=::read(fd,p,n);
      if(got<=0)return false;
      p+=got;n-=static_cast<std::size_t>(got);
    }
    return true;
  }
  bool read_record(std::uint64_t at,std::uint64_t length,Record& b,std::size_t& n){
    if(at>length||length-at<36||
       lseek(fd,static_cast<off_t>(at),SEEK_SET)!=static_cast<off_t>(at))return false;
    b.fill(0);
    if(!exact(b.data(),32)||b[0]!='P'||b[1]!='M'||b[2]!='A'||b[3]!=1)return false;
    const unsigned points=b[18]+256u*b[19];
    n=36+5*points;
    return points<=kMaxInkPoints&&n<=length-at&&exact(b.data()+32,n-32);
  }
  bool unchanged(){std::uint64_t n=0;return size(n)&&n==end;}
  static bool same(const State& a,const State& b){
    return a.attempts==b.attempts&&a.solved==b.solved&&
      a.independent==b.independent&&a.retried==b.retried;
  }
 public:
  explicit Journal(int descriptor):fd(descriptor){}
  Journal(const Journal&)=delete;
  Journal& operator=(const Journal&)=delete;
  JournalState state()const{return status;}
  bool start(){
    if(status!=JournalState::Idle)return fail("already-started");
    const int flags=fcntl(fd,F_GETFL);
    if(flags<0||(flags&O_ACCMODE)!=O_RDWR||!(flags&O_APPEND)||
       !size(end)||end>max_bytes)return fail("open");
    current={};cursor=0;status=JournalState::Replaying;return true;
  }
  // One record per poll. Caller can yield to display and watchdog between
  // records. A partial or corrupt tail is preserved, never auto-repaired.
  JournalState poll(){
    if(status!=JournalState::Replaying)return status;
    if(!unchanged()){fail("changed-size");return status;}
    if(cursor==end){status=JournalState::Ready;return status;}
    std::size_t n=0;
    if(!read_record(cursor,end,staged,n)||!replay(staged,n,current)){
      fail("replay");return status;
    }
    cursor+=n;return status;
  }
  bool restored(State& out)const{
    if(status!=JournalState::Ready)return false;
    out=current;return true;
  }
  SaveResult save(State& visible,unsigned choice,MathScratchpad& ink,Result& feedback){
    if(status!=JournalState::Ready)return SaveResult::Unavailable;
    if(!same(visible,current))return SaveResult::Rejected;
    std::size_t n=0;
    auto next=current;
    if(!encode(current,choice,ink,staged,n)||!replay(staged,n,next))
      return SaveResult::Rejected;
    if(n>max_bytes-end){fail("segment-full");return SaveResult::Full;}
    if(!unchanged()||::write(fd,staged.data(),n)!=static_cast<ssize_t>(n)||
       fsync(fd)!=0){fail("append");return SaveResult::Unavailable;}
    // Require exact committed length and exact byte readback before UI
    // changes. Ambiguous writes require a fresh owner and replay.
    std::uint64_t actual=0;std::size_t read=0;
    if(!size(actual)||actual!=end+n||
       !read_record(end,actual,check,read)||read!=n||check!=staged){
      fail("readback");return SaveResult::Unavailable;
    }
    end=actual;cursor=end;current=next;visible=next;
    feedback=static_cast<Result>(staged[17]);
    if(feedback==Result::Correct)scratch_clear(ink);
    return SaveResult::Saved;
  }
};
}
