#include "learner_navigation.h"
#include <cassert>
#include <array>

int main() {
  LearnerNavigation state;
  const std::array<LearnerAction, 5> actions = {
      LearnerAction::Math, LearnerAction::Reading, LearnerAction::Spelling,
      LearnerAction::Flashcards, LearnerAction::Journey};
  const std::array<LearnerScreen, 5> screens = {
      LearnerScreen::Math, LearnerScreen::Reading, LearnerScreen::Spelling,
      LearnerScreen::Flashcards, LearnerScreen::Journey};
  for (unsigned i = 0; i < actions.size(); ++i) {
    assert(state.screen == LearnerScreen::Home);
    assert(learner_navigate(state, actions[i]));
    assert(state.screen == screens[i]);
    assert(learner_navigate(state, LearnerAction::Help));
    assert(state.help_open);
    assert(!learner_navigate(state, LearnerAction::Math));
    assert(learner_navigate(state, LearnerAction::Back));
    assert(!state.help_open && state.screen == screens[i]);
    assert(learner_navigate(state, LearnerAction::Back));
    assert(state.screen == LearnerScreen::Home);
  }
  assert(!learner_navigate(state, LearnerAction::Back));
  assert(learner_navigate(state, LearnerAction::Help));
  assert(!learner_navigate(state, LearnerAction::Reading));
  assert(learner_navigate(state, LearnerAction::Home));
  assert(!state.help_open);
}
