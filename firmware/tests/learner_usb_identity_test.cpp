#include "learner_usb_identity.h"
#include <cassert>
#include <string>

int main() {
  LearnerUsbIdentity parser;
  char nonce[33]{};
  const std::string token="0123456789abcdef0123456789abcdef";
  auto send=[&](const std::string& line) {
    int count=0;
    for(unsigned char c:line) if(parser.feed(c,nonce)) ++count;
    return count;
  };
  assert(send("HELLO,1,"+token+"\n")==1);
  assert(std::string(nonce)==token);
  assert(send("HELLO,1,"+token+"\r\n")==1);
  assert(send("HELLO,2,"+token+"\n")==0);
  assert(send("HELLO,1,"+token+"x\n")==0);
  assert(send("HELLO,1,"+token.substr(1)+"\n")==0);
  assert(send("HELLO,1,"+token+"\rX\n")==0);
  assert(send("HELLO,1,"+std::string(32,'G')+"\n")==0);
  assert(send("RESET\nEXPORT\n")==0);
  assert(send(std::string(400,'x')+"HELLO,1,"+token+"\n")==0);
  assert(send("HELLO,1,"+token)==0);
  assert(send("\n")==1);
  assert(send("HELLO,1,"+token+"\n")==1);
}
