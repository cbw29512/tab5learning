#pragma once
#include <cstddef>
#include <cstdint>
namespace curriculum_preview {
struct VisualRect { int x,y,w,h,radius; std::uint16_t color; };
struct Visual { const VisualRect* shapes; std::size_t count; };
inline constexpr int visual_x=48,visual_y=250,visual_w=500,visual_h=148;
inline constexpr int visual_text_x=620,visual_columns=32;
template<std::size_t N> constexpr bool visual_bounds(const VisualRect (&s)[N]){
  for(const auto& r:s)
    if(r.x<0||r.y<0||r.w<=0||r.h<=0||r.x+r.w>visual_w||
       r.y+r.h>visual_h||r.radius<0||2*r.radius>r.w||2*r.radius>r.h)return false;
  return true;
}
}
