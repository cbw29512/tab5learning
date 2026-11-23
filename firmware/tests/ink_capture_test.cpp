#include "ink_capture.h"
#include <cassert>
#include <iostream>

int main() {
  InkCapture ink{};
  assert(ink.count == 0);
  assert(ink_stroke_count(ink) == 0);

  assert(ink_append(ink, 10, 20, true));
  assert(ink_append(ink, 12, 22, false));
  assert(ink_append(ink, 12, 22, false));  // Deduplicated.
  assert(ink.count == 2);
  assert(ink_stroke_count(ink) == 1);

  assert(ink_append(ink, 30, 40, true));
  assert(ink_stroke_count(ink) == 2);
  assert(ink.points[2].stroke_start);

  ink_clear(ink);
  assert(ink.count == 0 && !ink.overflow);
  for (std::size_t i = 0; i < kMaxInkPoints; ++i) {
    assert(ink_append(ink, static_cast<std::uint16_t>(i),
                      5, i == 0));
  }
  assert(!ink_append(ink, 600, 5, false));
  assert(ink.overflow);
  assert(ink.count == kMaxInkPoints);
  ink_clear(ink);
  assert(ink.count == 0 && !ink.overflow);

  std::cout << "ink capture tests passed\n";
}
