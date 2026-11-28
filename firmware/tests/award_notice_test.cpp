#include "award_notice.h"
#include <cassert>

int main() {
  AwardNotice state;
  assert(award_notice_offer(state, 1, 100, true));
  assert(state.visible && state.count == 1);
  assert(!award_notice_offer(state, 1, 200, true));
  assert(award_notice_offer(state, 2, 500, true));
  assert(state.count == 2 && state.started == 100);
  award_notice_tick(state, 2599, true);
  assert(state.visible);
  award_notice_tick(state, 2600, true);
  assert(!state.visible);
  assert(!award_notice_offer(state, 3, 3000, false));
  assert(!state.visible);
  assert(!award_notice_offer(state, 3, 3100, true));
  assert(award_notice_offer(state, 4, 3200, true));
  award_notice_tick(state, 3201, false);
  assert(!state.visible);
  state.enabled = false;
  assert(!award_notice_offer(state, 5, 3300, true));
  state.enabled = true;
  assert(award_notice_offer(state, 6, 0xfffffff0u, true));
  award_notice_tick(state, 0xfffffff0u + 2499u, true);
  assert(state.visible);
  award_notice_tick(state, 0xfffffff0u + 2500u, true);
  assert(!state.visible);
}
