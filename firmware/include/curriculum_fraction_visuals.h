#pragma once
#include "curriculum_preview.h"
#include "curriculum_scene_builder.h"
#include <cstring>
namespace curriculum_preview {
// Rectangles are the same 180 x 100 whole; internal partitions are equal.
constexpr void share_rectangle(MathScene& s,int x,unsigned pieces,bool shade){
  if(pieces!=2&&pieces!=4){s.valid=false;return;}
  s.rect(x,24,180,100,0,ink);
  s.rect(x+2,26,176,96,0,0xFFFF);
  if(shade)s.rect(x+2,26,180/int(pieces)-2,96,0,blue_rod);
  for(unsigned i=1;i<pieces;++i)s.rect(x+180*int(i)/int(pieces)-1,24,2,100,0,ink);
}
constexpr MathScene fraction_scene(unsigned activity){
  auto s=math_panel();
  switch(activity){
    case 0: share_rectangle(s,45,2,false);share_rectangle(s,275,4,false);break;
    case 1:
      s.rect(186,10,128,128,64,ink);s.rect(188,12,124,124,62,0xFFFF);
      s.rect(249,12,2,124,0,ink);break;
    case 2: share_rectangle(s,160,4,false);break;
    case 3:
      // Offset chord, not a diameter: two unequal areas of the same circle.
      s.rect(186,10,128,128,64,ink);s.rect(188,12,124,124,62,0xFFFF);
      s.rect(274,17,2,114,0,ink);break;
    case 4: share_rectangle(s,45,2,false);share_rectangle(s,275,4,false);break;
    case 5: share_rectangle(s,45,2,true);share_rectangle(s,275,4,true);break;
    default:s.valid=false;
  }
  return s;
}
inline constexpr auto shares_0=fraction_scene(0);
static_assert(shares_0.valid);
inline constexpr auto shares_1=fraction_scene(1);
static_assert(shares_1.valid);
inline constexpr auto shares_2=fraction_scene(2);
static_assert(shares_2.valid);
inline constexpr auto shares_3=fraction_scene(3);
static_assert(shares_3.valid);
inline constexpr auto shares_4=fraction_scene(4);
static_assert(shares_4.valid);
inline constexpr auto shares_5=fraction_scene(5);
static_assert(shares_5.valid);
inline constexpr Visual shares_views[]={shares_0.view(),shares_1.view(),shares_2.view(),shares_3.view(),shares_4.view(),shares_5.view()};
inline const Visual* fraction_visual_for(const Lesson& l,std::size_t step){
  if(l.revision!=1||step>=l.count||step>=6)return nullptr;
  return std::strcmp(l.id,"g1-equal-shares")==0?&shares_views[step]:nullptr;
}
}
