#pragma once
#include <M5Unified.h>
#include "reading_speaker_output.h"

// Channel 7 reserved for narration. The caller must initialize M5 first,
// keep PCM immutable, and avoid other producers on this channel.
// https://docs.m5stack.com/en/arduino/m5unified/speaker_class
class M5ReadingSpeaker final : public ReadingSpeakerBackend {
 public:
  size_t queued() const override { return M5.Speaker.isPlaying(kChannel); }
  bool submit(const int16_t* samples, size_t count, uint32_t rate) override {
    if (!samples || !count || queued()) return false;
    return M5.Speaker.playRaw(samples, count, rate, false, 1, kChannel, false);
  }
  void cancel() override { M5.Speaker.stop(kChannel); }
 private:
  static constexpr uint8_t kChannel = 7;
};
// Deliberately not a ReadingAudio implementation: M5Unified queue status
// does not establish exact hardware-consumed source position.
