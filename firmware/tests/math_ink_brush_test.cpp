#include "math_ink_brush.h"
#include <cassert>
#include <set>
#include <utility>

int main() {
  std::set<std::pair<int,int>> pixels;
  auto span=[&](int left,int right,int y) {
    assert(left<=right);
    for(int x=left;x<=right;++x) {
      assert(scratch_inside(x,y));
      pixels.emplace(x,y);
    }
  };
  InkPoint a{700,220,true},b{720,220,false};
  math_ink_brush(a,nullptr,span);
  for(int dy=-3;dy<=3;++dy) assert(pixels.count({700,220+dy}));
  assert(!pixels.count({700,224}));
  pixels.clear();
  math_ink_brush(b,&a,span);
  for(int x=700;x<=720;++x)
    for(int dy=-3;dy<=3;++dy) assert(pixels.count({x,220+dy}));
  pixels.clear();
  InkPoint c{730,240,true};
  math_ink_brush(c,&b,span);
  assert(pixels.count({730,240})&&!pixels.count({720,220}));
  pixels.clear();
  InkPoint edge{656,184,true},end{1247,427,false};
  math_ink_brush(edge,nullptr,span);
  assert(pixels.count({656,184})&&pixels.count({659,184}));
  math_ink_brush(end,&edge,span);
  assert(pixels.count({1247,427}));
  pixels.clear();
  math_ink_brush(InkPoint{656,184,false},&end,span);
  assert(pixels.count({656,184})&&pixels.count({1247,427}));
  pixels.clear();
  math_ink_brush(InkPoint{655,184,true},nullptr,span);
  assert(pixels.empty());
}
