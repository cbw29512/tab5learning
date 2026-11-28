#include "learner_layout.h"
#include <cassert>
int main() {
  LearnerNavigation nav;
  LearnerAction action;
  for (const auto& t:learner_tiles) {
    assert(t.x>=0 && t.y>=0 && t.x+t.w<=1280 && t.y+t.h<=720);
    assert(learner_hit(nav,t.x+t.w/2,t.y+t.h/2,action));
    assert(action==t.action);
    assert(!t.hit(t.x+t.w,t.y));
  }
  const auto& sd=learner_storage_region;
  assert(sd.x+sd.w<=learner_battery_region.x);
  assert(learner_heading_x>=learner_home.x+learner_home.w);
  // Longest existing heading is 19 characters; default font 6px at size 3.
  assert(learner_heading_x+20*6*learner_heading_size<sd.x);
  assert(sd.y+sd.h<=learner_notice_region.y);
  assert(learner_hit(nav,sd.x+5,sd.y+5,action)&&action==LearnerAction::Help);
  assert(!learner_hit(nav,sd.x+sd.w,sd.y+5,action));
  assert(!learner_sd_check_hit(nav,49,281));
  assert(learner_navigate(nav,action)&&nav.help_open);
  assert(learner_sd_check_hit(nav,49,281));
  assert(!learner_sd_check_hit(nav,49,332));
  assert(!learner_sd_check_hit(nav,sd.x+5,sd.y+5));
  assert(learner_navigate(nav,LearnerAction::Back)&&!nav.help_open);
  assert(!learner_hit(nav,640,250,action));
  learner_navigate(nav,LearnerAction::Math);
  assert(!learner_hit(nav,700,250,action));
  assert(learner_hit(nav,100,600,action)&&action==LearnerAction::Back);
  learner_navigate(nav,LearnerAction::Help);
  assert(!learner_hit(nav,100,200,action));
  assert(learner_hit(nav,50,50,action)&&action==LearnerAction::Home);
}
