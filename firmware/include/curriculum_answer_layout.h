#pragma once
#include "curriculum_preview.h"
namespace curriculum_preview {
// Full-width, finger-sized answers. Keep the picture and prompt intact.
// Same rectangle is used for drawing and hit testing; gaps never select.
struct AnswerRect {
  int x,y,w,h;
  constexpr bool hit(int px,int py)const{
    return px>=x&&px<x+w&&py>=y&&py<y+h;
  }
};
inline constexpr unsigned answer_max=3;
inline constexpr AnswerRect answer_rect(unsigned i){
  return {48,408+72*static_cast<int>(i),1184,64};
}
inline constexpr int answer_index(unsigned count,int x,int y){
  if(count<2||count>answer_max)return -1;
  for(unsigned i=0;i<count;++i)if(answer_rect(i).hit(x,y))return static_cast<int>(i);
  return -1;
}
inline bool large_answers(const Preview& p){
  return p.screen==Screen::Activity&&p.activity().kind==Kind::Choice;
}
// Default font: six pixels per character at scale 1; 24 px at scale 4.
// Preserve a 24 px inset at both ends. Fall back for longer future labels.
inline constexpr unsigned answer_font(std::size_t length){
  return length<=47?4:3;
}
}
