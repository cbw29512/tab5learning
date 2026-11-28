#pragma once
#include "curriculum_preview.h"
#include "curriculum_scene_builder.h"
#include <cstring>
namespace curriculum_preview {
// 24 fixed directions, clockwise from twelve, in thousandths of radius.
// Odd positions place the hour hand exactly halfway between hour marks.
struct ClockDirection {int x,y;};
inline constexpr ClockDirection clock_dirs[]={
 {0,-1000},{259,-966},{500,-866},{707,-707},{866,-500},{966,-259},
 {1000,0},{966,259},{866,500},{707,707},{500,866},{259,966},
 {0,1000},{-259,966},{-500,866},{-707,707},{-866,500},{-966,259},
 {-1000,0},{-966,-259},{-866,-500},{-707,-707},{-500,-866},{-259,-966}
};
constexpr unsigned hour_direction(unsigned hour,bool half){return (2*(hour%12)+(half?1:0))%24;}
constexpr unsigned minute_direction(bool half){return half?12:0;}
constexpr ClockDirection clock_point(unsigned direction,int radius){
  return {clock_dirs[direction].x*radius/1000,clock_dirs[direction].y*radius/1000};
}
// A tiny original 3x5 numeral font keeps the art independent of text state.
inline constexpr unsigned digit_rows[10][5]={
 {7,5,5,5,7},{2,6,2,2,7},{7,1,7,4,7},{7,1,7,1,7},{5,5,7,1,1},
 {7,4,7,1,7},{7,4,7,5,7},{7,1,1,1,1},{7,5,7,5,7},{7,5,7,1,7}
};
constexpr void clock_digit(MathScene& s,unsigned digit,int x,int y){
  for(int row=0;row<5;++row){
    // Adjacent pixels are drawn as one rectangle; bounded shape count.
    for(int col=0;col<3;){
      if(!(digit_rows[digit][row]&(4u>>col))){++col;continue;}
      const int start=col;
      while(col<3&&(digit_rows[digit][row]&(4u>>col)))++col;
      s.rect(x+2*start,y+2*row,2*(col-start),2,0,ink);
    }
  }
}
constexpr void clock_number(MathScene& s,unsigned n,int x,int y){
  if(n>=10){clock_digit(s,1,x-7,y-5);clock_digit(s,n%10,x+1,y-5);}
  else clock_digit(s,n,x-3,y-5);
}
constexpr void clock_hand(MathScene& s,unsigned direction,int length,int width,std::uint16_t color){
  // Overlapping square samples are a continuous thick hand, even diagonally.
  const auto end=clock_point(direction,length);
  const int count=(length+3)/4;
  for(int i=0;i<=count;++i)
    s.rect(250+end.x*i/count-width/2,74+end.y*i/count-width/2,width,width,0,color);
}
constexpr MathScene clock_scene(unsigned hour,bool half){
  auto s=math_panel();
  if(hour<1||hour>12){s.valid=false;return s;}
  s.rect(178,2,144,144,72,ink);s.rect(182,6,136,136,68,0xFFFF);
  for(unsigned n=1;n<=12;++n){
    const auto tick=clock_point((n%12)*2,63);
    s.rect(248+tick.x,72+tick.y,4,4,0,ink);
    const auto label=clock_point((n%12)*2,52);
    clock_number(s,n,250+label.x,74+label.y);
  }
  // Longer minute hand, shorter hour hand. Color is secondary to length.
  clock_hand(s,minute_direction(half),44,5,ink);
  clock_hand(s,hour_direction(hour,half),29,8,blue_rod);
  s.rect(245,69,10,10,5,ink);return s;
}
inline constexpr auto hour_0=clock_scene(3,false);
static_assert(hour_0.valid);
inline constexpr auto hour_0_packed=pack_scene<hour_0>();
inline constexpr auto hour_1=clock_scene(3,false);
static_assert(hour_1.valid);
inline constexpr auto hour_1_packed=pack_scene<hour_1>();
inline constexpr auto hour_2=clock_scene(7,false);
static_assert(hour_2.valid);
inline constexpr auto hour_2_packed=pack_scene<hour_2>();
inline constexpr auto hour_3=clock_scene(9,false);
static_assert(hour_3.valid);
inline constexpr auto hour_3_packed=pack_scene<hour_3>();
inline constexpr auto hour_4=clock_scene(4,false);
static_assert(hour_4.valid);
inline constexpr auto hour_4_packed=pack_scene<hour_4>();
inline constexpr auto hour_5=clock_scene(6,false);
static_assert(hour_5.valid);
inline constexpr auto hour_5_packed=pack_scene<hour_5>();
inline constexpr Visual hour_views[]={packed_view(hour_0_packed),packed_view(hour_1_packed),packed_view(hour_2_packed),packed_view(hour_3_packed),packed_view(hour_4_packed),packed_view(hour_5_packed)};
inline constexpr auto half_hour_0=clock_scene(2,true);
static_assert(half_hour_0.valid);
inline constexpr auto half_hour_0_packed=pack_scene<half_hour_0>();
inline constexpr auto half_hour_1=clock_scene(5,true);
static_assert(half_hour_1.valid);
inline constexpr auto half_hour_1_packed=pack_scene<half_hour_1>();
inline constexpr auto half_hour_2=clock_scene(7,true);
static_assert(half_hour_2.valid);
inline constexpr auto half_hour_2_packed=pack_scene<half_hour_2>();
inline constexpr auto half_hour_3=clock_scene(10,true);
static_assert(half_hour_3.valid);
inline constexpr auto half_hour_3_packed=pack_scene<half_hour_3>();
inline constexpr auto half_hour_4=clock_scene(4,true);
static_assert(half_hour_4.valid);
inline constexpr auto half_hour_4_packed=pack_scene<half_hour_4>();
inline constexpr auto half_hour_5=clock_scene(11,true);
static_assert(half_hour_5.valid);
inline constexpr auto half_hour_5_packed=pack_scene<half_hour_5>();
inline constexpr Visual half_hour_views[]={packed_view(half_hour_0_packed),packed_view(half_hour_1_packed),packed_view(half_hour_2_packed),packed_view(half_hour_3_packed),packed_view(half_hour_4_packed),packed_view(half_hour_5_packed)};
static_assert(hour_direction(12,false)==0&&hour_direction(11,true)==23);
static_assert(hour_direction(2,true)==5&&minute_direction(true)==12);
static_assert(clock_point(0,44).y==-44&&clock_point(12,44).y==44);
inline const Visual* clock_visual_for(const Lesson& l,std::size_t step){
  if(l.revision!=1||step>=l.count||step>=6)return nullptr;
  if(std::strcmp(l.id,"g1-clock-hours")==0)return &hour_views[step];
  if(std::strcmp(l.id,"g1-clock-half-hours")==0)return &half_hour_views[step];
  return nullptr;
}
}
