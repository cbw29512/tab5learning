#pragma once
#include <cstdint>

// Mono signed 16-bit PCM. The sample count excludes container headers.
// All ranges are half-open and expressed in source samples.
struct PcmRange {
  uint32_t first = 0;
  uint32_t end = 0;
};

class PcmSourceClock {
 public:
  bool initialize(uint32_t rate, uint32_t count) {
    rate_ = 0; count_ = 0;
    // Constrain duration to the uint32_t millisecond timeline contract.
    if (rate < 8000 || rate > 48000 || !count ||
        uint64_t(count) * 1000 / rate > UINT32_MAX) return false;
    rate_ = rate; count_ = count;
    return true;
  }

  uint32_t duration_ms() const {
    return rate_ ? static_cast<uint32_t>(uint64_t(count_) * 1000 / rate_) : 0;
  }

  bool range(uint32_t start_ms, uint32_t end_ms, PcmRange& out) const {
    if (!rate_ || start_ms >= end_ms || end_ms > duration_ms()) return false;
    // Floor both boundaries so adjacent words never share or skip samples.
    const auto first = uint64_t(start_ms) * rate_ / 1000;
    const auto end = end_ms == duration_ms() ? count_ :
                     uint64_t(end_ms) * rate_ / 1000;
    if (first >= end || end > count_) return false;
    out = {static_cast<uint32_t>(first), static_cast<uint32_t>(end)};
    return true;
  }

  // Only pass samples confirmed consumed by the hardware, not queued samples.
  bool position(uint32_t consumed, uint32_t& ms) const {
    if (!rate_ || consumed > count_) return false;
    ms = static_cast<uint32_t>(uint64_t(consumed) * 1000 / rate_);
    return true;
  }

 private:
  uint32_t rate_ = 0, count_ = 0;
};
