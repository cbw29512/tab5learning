#pragma once
#include <cstddef>
#include <cstring>

// Development identification only. No learner state or credentials are exposed.
class LearnerUsbIdentity {
 public:
  // Returns a nonce only on a complete, valid newline-terminated HELLO.
  bool feed(unsigned char c, char (&nonce)[33]) {
    if(c=='\n') {
      bool valid=!discard_ && used_==40 && std::memcmp(line_,"HELLO,1,",8)==0;
      if(valid) {
        for(std::size_t i=8;i<40;++i) {
          char x=line_[i];
          if(!((x>='0' && x<='9') || (x>='a' && x<='f'))) valid=false;
        }
      }
      if(valid) { std::memcpy(nonce,line_+8,32); nonce[32]='\0'; }
      used_=0; discard_=false; cr_=false;
      return valid;
    }
    if(discard_) return false;
    if(cr_) { discard_=true; return false; }
    if(c=='\r') { cr_=true; return false; }
    if(c<32 || c>126 || used_>=sizeof(line_)) { discard_=true; return false; }
    line_[used_++]=static_cast<char>(c);
    return false;
  }
 private:
  char line_[40]{};
  std::size_t used_=0;
  bool discard_=false, cr_=false;
};
void learner_usb_identity_poll();
