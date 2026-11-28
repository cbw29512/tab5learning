#pragma once
#include "curriculum_preview.h"
#include "curriculum_visual_types.h"
#include "curriculum_math_visuals.h"
#include "curriculum_clock_visuals.h"
#include "curriculum_fraction_visuals.h"
#include <cstdint>
#include <cstring>
namespace curriculum_preview {
// Original geometric illustrations in flash; no external decoder or storage writes.
inline constexpr VisualRect park_shapes[]={
  {0,0,500,148,0,0x9E7F}, {0,108,500,40,0,0x67E0},
  {380,5,34,34,17,0xFFE0}, {65,45,20,72,0,0x8200},
  {25,10,100,65,25,0x03E0}, {150,45,230,28,5,0x001F},
  {150,75,230,14,3,0x001F}, {165,89,14,45,0,0x4208},
  {351,89,14,45,0,0x4208}, {244,103,36,36,18,0xF800}
};
inline constexpr VisualRect garden_shapes[]={
  {0,0,500,148,0,0x9E7F}, {0,112,500,36,0,0x8200},
  {90,47,10,74,0,0x03E0}, {245,42,10,79,0,0x03E0},
  {400,49,10,72,0,0x03E0}, {65,74,35,15,7,0x03E0},
  {100,86,35,15,7,0x03E0}, {220,72,35,15,7,0x03E0},
  {255,84,35,15,7,0x03E0}, {375,78,35,15,7,0x03E0},
  {410,88,35,15,7,0x03E0}, {65,14,60,60,30,0xF800},
  {220,9,60,60,30,0xF81F}, {375,16,60,60,30,0xFBE0},
  {85,34,20,20,10,0xFFE0}, {240,29,20,20,10,0xFFE0},
  {395,36,20,20,10,0xFFE0}
};
static_assert(visual_bounds(park_shapes)&&visual_bounds(garden_shapes));
static_assert(visual_y+visual_h<=408&&visual_text_x+visual_columns*18<=1232);
inline constexpr Visual park{park_shapes,sizeof(park_shapes)/sizeof(*park_shapes)};
inline constexpr Visual garden{garden_shapes,sizeof(garden_shapes)/sizeof(*garden_shapes)};
// Stable IDs and revision; changed or unknown lesson content fails closed.
inline const Visual* visual_for(const Lesson& l,std::size_t step){
  if(l.revision!=1||step>=l.count||step>=6)return nullptr;
  if(std::strcmp(l.id,"g1-picture-or-words")==0)return &park;
  if(std::strcmp(l.id,"g1-pictures-explain-details")==0)return &garden;
  if(const auto* scene=math_visual_for(l,step))return scene;
  if(const auto* scene=clock_visual_for(l,step))return scene;
  return fraction_visual_for(l,step);
}
}
