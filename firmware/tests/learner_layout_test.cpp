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
  assert(!learner_hit(nav,640,250,action));
  learner_navigate(nav,LearnerAction::Math);
  assert(!learner_hit(nav,700,250,action));
  assert(learner_hit(nav,100,600,action)&&action==LearnerAction::Back);
  learner_navigate(nav,LearnerAction::Help);
  assert(!learner_hit(nav,100,200,action));
  assert(learner_hit(nav,50,50,action)&&action==LearnerAction::Home);
}
