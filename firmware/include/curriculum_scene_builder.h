#pragma once
#include "curriculum_visual_types.h"
#include <array>
namespace curriculum_preview {
// Bounded immutable scene builder; all scene construction happens at compile
// time. Runtime drawing continues to use the existing rectangle renderer.
struct MathScene {
  std::array<VisualRect,192> shapes{};
  std::size_t count=0;
  bool valid=true;
  constexpr void rect(int x,int y,int w,int h,int r,std::uint16_t color){
    if(count>=shapes.size()||x<0||y<0||w<=0||h<=0||
       x+w>visual_w||y+h>visual_h||r<0||2*r>w||2*r>h){
      valid=false;return;
    }
    shapes[count++]={x,y,w,h,r,color};
  }
  constexpr Visual view()const{return {shapes.data(),count};}
};
inline constexpr std::uint16_t paper=0xEF7D,ink=0x2104,blue_rod=0x045F,
  green_rod=0x05E0,red_mark=0xC000,muted=0xB596;
constexpr MathScene math_panel(){
  MathScene s{};s.rect(0,0,500,148,0,paper);return s;
}
}
