#pragma once
#include <cstddef>
#include <cstring>
namespace curriculum_preview {
// Portable, fixed-memory word wrapping. Counting and drawing use exactly
// the same algorithm. Long tokens split without losing any characters.
template<class Emit>
unsigned wrap_lines(const char* text,unsigned columns,Emit emit){
  if(!text||!columns||columns>60)return 0;
  char line[61]{};
  unsigned used=0,lines=0;
  auto flush=[&](){
    if(!used)return;
    line[used]='\0';emit(line,lines++);used=0;
  };
  while(*text){
    while(*text==' ')++text;
    if(!*text)break;
    const char* end=text;
    while(*end&&*end!=' ')++end;
    auto remaining=static_cast<unsigned>(end-text);
    if(used&&used+1+remaining>columns)flush();
    if(used)line[used++]=' ';
    while(remaining){
      const auto available=columns-used;
      const auto take=remaining<available?remaining:available;
      std::memcpy(line+used,text,take);
      used+=take;text+=take;remaining-=take;
      if(remaining)flush();
    }
  }
  flush();
  return lines;
}
struct TextLayout {
  unsigned font,columns,lines,pitch;
};
inline TextLayout activity_text_layout(const char* text,bool illustrated,bool feedback){
  // Four 32 px lines with 38 px pitch fit both existing prompt and
  // feedback regions. Preserve larger 64 px choices and full artwork.
  const unsigned wide=illustrated?24u:48u;
  const auto n=wrap_lines(text,wide,[](const char*,unsigned){});
  if(n<=4)return {4,wide,4,38};
  return {3,illustrated?32u:60u,illustrated&&feedback?4u:5u,30};
}
}
