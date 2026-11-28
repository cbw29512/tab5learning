#include "reading_speaker_output.h"
#include <cassert>

struct FakeSpeaker : ReadingSpeakerBackend {
  size_t occupancy = 0;
  const int16_t* submitted = nullptr;
  size_t length = 0;
  uint32_t sample_rate = 0;
  bool fail = false;
  unsigned cancellations = 0;
  size_t queued() const override { return occupancy; }
  bool submit(const int16_t* p, size_t n, uint32_t rate) override {
    if (fail) return false;
    submitted = p;
    length = n;
    sample_rate = rate;
    occupancy = 1;
    return true;
  }
  void cancel() override { ++cancellations; }
};

int main() {
  static const int16_t pcm[16000] = {};
  FakeSpeaker backend;
  ReadingSpeakerOutput output(backend);
  assert(output.state() == SpeakerOutputState::Idle);
  assert(!output.start(nullptr, 16000, 16000, 0, 100));
  assert(!output.start(pcm, 16000, 16000, 100, 100));
  assert(!output.start(pcm, 16000, 16000, 0, 1001));
  backend.occupancy = 1;
  assert(!output.start(pcm, 16000, 16000, 100, 250));
  backend.occupancy = 0;
  assert(output.start(pcm, 16000, 16000, 100, 250));
  assert(backend.submitted == pcm + 1600);
  assert(backend.length == 2400 && backend.sample_rate == 16000);
  assert(!output.start(pcm, 16000, 16000, 0, 100));
  output.poll();
  assert(output.state() == SpeakerOutputState::Queued);
  output.stop();
  assert(backend.cancellations == 1);
  output.poll();
  assert(output.state() == SpeakerOutputState::Cancelling);
  assert(!output.start(pcm, 16000, 16000, 0, 100));
  backend.occupancy = 0;
  output.poll();
  assert(output.state() == SpeakerOutputState::Idle);
  assert(output.start(pcm, 16000, 16000, 0, 1000));
  backend.occupancy = 0;
  output.poll();
  assert(output.state() == SpeakerOutputState::QueueEmpty);
  backend.fail = true;
  assert(!output.start(pcm, 16000, 16000, 0, 100));
  assert(output.state() == SpeakerOutputState::Error);
  return 0;
}
