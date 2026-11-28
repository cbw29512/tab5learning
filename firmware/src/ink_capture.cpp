#include "ink_capture.h"

void ink_clear(InkCapture& capture) {
  capture.count = 0;
  capture.overflow = false;
}

bool ink_append(InkCapture& capture, std::uint16_t x,
                std::uint16_t y, bool stroke_start) {
  // Repeated coordinates add no information; a new stroke always gets a point.
  if (capture.count && !stroke_start) {
    const InkPoint& last = capture.points[capture.count - 1];
    if (last.x == x && last.y == y) return true;
  }

  // Never write beyond the fixed buffer. Signal failure for UI logging.
  if (capture.count >= kMaxInkPoints) {
    capture.overflow = true;
    return false;
  }
  capture.points[capture.count++] = {x, y, stroke_start};
  return true;
}

std::size_t ink_stroke_count(const InkCapture& capture) {
  std::size_t strokes = 0;
  for (std::size_t i = 0; i < capture.count; ++i) {
    if (capture.points[i].stroke_start) ++strokes;
  }
  return strokes;
}
