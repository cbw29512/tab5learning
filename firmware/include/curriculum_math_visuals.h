#pragma once
#include "curriculum_preview.h"
#include "curriculum_visual_types.h"
#include <array>
#include <cstring>
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
constexpr MathScene coin_scene(unsigned n,unsigned cents){
  auto s=math_panel();
  if(n==0||n>10||(cents!=1&&cents!=5&&cents!=10)){s.valid=false;return s;}
  // Schematic coins, not realistic US coin faces. The written prompt names
  // the denomination; diameter and rim style differentiate the tokens.
  const int diameter=cents==5?54:cents==1?48:42;
  const std::uint16_t rim=cents==1?0x9B65:0x8410;
  const std::uint16_t face=cents==1?0xE4CA:0xCE59;
  for(unsigned i=0;i<n;++i){
    const int cx=50+100*int(i%5),cy=37+74*int(i/5);
    s.rect(cx-diameter/2,cy-diameter/2,diameter,diameter,diameter/2,rim);
    const int inner=diameter-8;
    s.rect(cx-inner/2,cy-inner/2,inner,inner,inner/2,face);
  }
  return s;
}
// Each rod has ten equal segments, visibly separated by nine dark marks.
constexpr void ten_rod(MathScene& s,int x,int y,int w,std::uint16_t color){
  s.rect(x,y,w,100,0,color);
  for(int i=1;i<10;++i)s.rect(x,y+10*i,w,1,0,ink);
}
constexpr void operand(MathScene& s,unsigned value,int x,std::uint16_t color){
  if(value>99){s.valid=false;return;}
  for(unsigned i=0;i<value/10;++i)ten_rod(s,x+20*int(i),8,15,color);
  for(unsigned i=0;i<value%10;++i)s.rect(x+20*int(i),120,15,15,0,color);
}
constexpr MathScene addition_scene(unsigned a,unsigned b){
  auto s=math_panel();
  operand(s,a,14,blue_rod);operand(s,b,285,green_rod);
  // A plus sign between operands; no numeric answer displayed.
  s.rect(230,70,40,7,0,ink);s.rect(246,54,7,39,0,ink);
  return s;
}
constexpr MathScene subtraction_scene(unsigned total,unsigned removed){
  auto s=math_panel();
  if(total==0||total>9||removed>total){s.valid=false;return s;}
  for(unsigned i=0;i<total;++i){
    const int x=22+52*int(i);
    const bool crossed=i>=total-removed;
    ten_rod(s,x,24,28,crossed?muted:blue_rod);
    if(crossed){
      // A red X also marks removed rods by shape, not color alone.
      for(int j=0;j<5;++j){
        s.rect(x+5*j,61+5*j,7,7,0,red_mark);
        s.rect(x+20-5*j,61+5*j,7,7,0,red_mark);
      }
    }
  }
  return s;
}
// g1-count-like-coins, revision 1: exact activity order, verified against catalog.
inline constexpr auto coins_0=coin_scene(4,5);
static_assert(coins_0.valid);
inline constexpr auto coins_1=coin_scene(3,10);
static_assert(coins_1.valid);
inline constexpr auto coins_2=coin_scene(6,1);
static_assert(coins_2.valid);
inline constexpr auto coins_3=coin_scene(8,5);
static_assert(coins_3.valid);
inline constexpr auto coins_4=coin_scene(10,10);
static_assert(coins_4.valid);
inline constexpr auto coins_5=coin_scene(5,5);
static_assert(coins_5.valid);
inline constexpr Visual coins_views[]={coins_0.view(),coins_1.view(),coins_2.view(),coins_3.view(),coins_4.view(),coins_5.view()};

// g1-add-tens-ones, revision 1: exact activity order, verified against catalog.
inline constexpr auto addition_0=addition_scene(36,4);
static_assert(addition_0.valid);
inline constexpr auto addition_1=addition_scene(24,30);
static_assert(addition_1.valid);
inline constexpr auto addition_2=addition_scene(32,5);
static_assert(addition_2.valid);
inline constexpr auto addition_3=addition_scene(48,5);
static_assert(addition_3.valid);
inline constexpr auto addition_4=addition_scene(27,6);
static_assert(addition_4.valid);
inline constexpr auto addition_5=addition_scene(65,20);
static_assert(addition_5.valid);
inline constexpr Visual addition_views[]={addition_0.view(),addition_1.view(),addition_2.view(),addition_3.view(),addition_4.view(),addition_5.view()};

// g1-subtract-tens, revision 1: exact activity order, verified against catalog.
inline constexpr auto subtraction_0=subtraction_scene(6,2);
static_assert(subtraction_0.valid);
inline constexpr auto subtraction_1=subtraction_scene(8,3);
static_assert(subtraction_1.valid);
inline constexpr auto subtraction_2=subtraction_scene(4,1);
static_assert(subtraction_2.valid);
inline constexpr auto subtraction_3=subtraction_scene(9,7);
static_assert(subtraction_3.valid);
inline constexpr auto subtraction_4=subtraction_scene(5,4);
static_assert(subtraction_4.valid);
inline constexpr auto subtraction_5=subtraction_scene(7,7);
static_assert(subtraction_5.valid);
inline constexpr Visual subtraction_views[]={subtraction_0.view(),subtraction_1.view(),subtraction_2.view(),subtraction_3.view(),subtraction_4.view(),subtraction_5.view()};
inline const Visual* math_visual_for(const Lesson& l,std::size_t step){
  if(l.revision!=1||step>=l.count||step>=6)return nullptr;
  if(std::strcmp(l.id,"g1-count-like-coins")==0)return &coins_views[step];
  if(std::strcmp(l.id,"g1-add-tens-ones")==0)return &addition_views[step];
  if(std::strcmp(l.id,"g1-subtract-tens")==0)return &subtraction_views[step];
  return nullptr;
}
}
