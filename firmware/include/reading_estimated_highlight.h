#pragma once
#include "reading_timeline.h"

// UI-only estimate from successful submission time, not acoustic position.
// Never use this for pronunciation scores or confirmed playback completion.
class ReadingEstimatedHighlight {
 public:
  bool start(const ReadingCue* cues, size_t count, uint32_t duration, uint32_t now) {
    stop();
    if (duration > 20000 || !timeline_.initialize(cues,count,duration)) return false;
    started_=now; duration_=duration;
    active_=timeline_.play();
    return active_;
  }
  int update(uint32_t now, bool queued) {
    if (!active_ || !queued) { stop(); return -1; }
    // Unsigned subtraction handles millis() rollover for this bounded clip.
    const uint32_t elapsed=now-started_;
    if (elapsed>=duration_ || !timeline_.observe(elapsed)) {
      stop(); return -1;
    }
    return timeline_.active;
  }
  void stop() { active_=false; }
 private:
  ReadingTimeline timeline_;
  uint32_t started_=0, duration_=0;
  bool active_=false;
};
