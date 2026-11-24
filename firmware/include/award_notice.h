#pragma once
#include <cstdint>

// Presentation only. Call offer exclusively for a newly committed badge.
// The portfolio is durable; notices are deliberately ephemeral.
struct AwardNotice {
  std::uint32_t started = 0;
  std::uint32_t sequence = 0;
  std::uint32_t newest = 0;
  std::uint16_t count = 0;
  bool visible = false;
  bool enabled = true;
};

constexpr std::uint32_t award_notice_ms = 2500;

inline void award_notice_tick(AwardNotice& state, std::uint32_t now,
                              bool safe_region) {
  // Unsigned subtraction handles wraparound of the millisecond clock.
  if (!state.enabled || !safe_region ||
      (state.visible && now - state.started >= award_notice_ms)) {
    state.visible = false;
    state.count = 0;
  }
}

inline bool award_notice_offer(AwardNotice& state, std::uint32_t sequence,
                               std::uint32_t now, bool safe_region) {
  // Caller supplies monotonically increasing committed award sequences.
  if (sequence == 0 || sequence <= state.newest) return false;
  state.newest = sequence;
  award_notice_tick(state, now, safe_region);
  if (!state.enabled || !safe_region) return false;
  if (!state.visible) {
    state.started = now;
    state.count = 0;
  }
  state.sequence = sequence;
  if (state.count < 65535) ++state.count;
  state.visible = true;
  // Multiple awards share a notice and never extend its original lifetime.
  return true;
}
