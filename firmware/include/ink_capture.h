#pragma once
#include <cstddef>
#include <cstdint>

// Schema first: an ink sample is a physical touch coordinate and stroke boundary.
// This is volatile diagnostic data, NOT yet a persisted student answer.
struct InkPoint {
  std::uint16_t x = 0;
  std::uint16_t y = 0;
  bool stroke_start = false;
};

constexpr std::size_t kMaxInkPoints = 512;
struct InkCapture {
  InkPoint points[kMaxInkPoints]{};
  std::size_t count = 0;
  bool overflow = false;
};

void ink_clear(InkCapture& capture);
bool ink_append(InkCapture& capture, std::uint16_t x,
                std::uint16_t y, bool stroke_start);
std::size_t ink_stroke_count(const InkCapture& capture);
