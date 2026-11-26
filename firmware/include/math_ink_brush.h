#pragma once
#include "math_scratchpad.h"

// Presentation only: 7-pixel round brush. Persisted touch points and card
// dimensions remain identical. Emit clipped horizontal spans, no framebuffer
// allocation, floating point or large temporary stack buffers.
inline constexpr int math_ink_radius=3;
template<class Span>
void math_ink_stamp(int x,int y,Span& span) {
  const auto& c=math_ink_card;
  constexpr int halfwidth[]={0,2,2,3,2,2,0};
  for(int dy=-math_ink_radius;dy<=math_ink_radius;++dy) {
    const int row=y+dy;
    if(row<c.y||row>=c.y+c.h) continue;
    int left=x-halfwidth[dy+math_ink_radius];
    int right=x+halfwidth[dy+math_ink_radius];
    if(left<c.x) left=c.x;
    if(right>=c.x+c.w) right=c.x+c.w-1;
    if(left<=right) span(left,right,row);
  }
}
template<class Span>
void math_ink_brush(const InkPoint& point,const InkPoint* prev,Span&& span) {
  if(!scratch_inside(point.x,point.y)) return;
  if(point.stroke_start||!prev||!scratch_inside(prev->x,prev->y)) {
    math_ink_stamp(point.x,point.y,span); return;
  }
  // Integer Bresenham interpolation connects sparse samples without gaps.
  int x=prev->x,y=prev->y;
  const int tx=point.x,ty=point.y;
  const int dx=tx>=x?tx-x:x-tx,dy=ty>=y?y-ty:ty-y;
  const int sx=x<tx?1:-1,sy=y<ty?1:-1;
  int error=dx-dy;
  for(;;) {
    math_ink_stamp(x,y,span);
    if(x==tx&&y==ty) break;
    const int e2=2*error;
    if(e2>-dy) { error-=dy; x+=sx; }
    if(e2<dx) { error+=dx; y+=sy; }
  }
}
