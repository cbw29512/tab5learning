#pragma once

// Portable learner navigation: no diagnostic, USB or administrator routes.
enum class LearnerScreen { Home, Math, Reading, Spelling, Flashcards, Journey, Curriculum };
enum class LearnerAction { Home, Back, Math, Reading, Spelling, Flashcards, Journey, Help, Curriculum };

struct LearnerNavigation {
  LearnerScreen screen = LearnerScreen::Home;
  bool help_open = false;
};

// Returns true only when the visible state changes.
inline bool learner_navigate(LearnerNavigation& state, LearnerAction action) {
  const auto previous = state;
  switch (action) {
    case LearnerAction::Home:
      state = {};
      break;
    case LearnerAction::Back:
      if (state.help_open) state.help_open = false;
      else state.screen = LearnerScreen::Home;
      break;
    case LearnerAction::Help:
      state.help_open = true;
      break;
    default:
      // Subject controls are available from home, not through a help overlay.
      if (state.screen != LearnerScreen::Home || state.help_open) return false;
      switch (action) {
        case LearnerAction::Math: state.screen = LearnerScreen::Math; break;
        case LearnerAction::Reading: state.screen = LearnerScreen::Reading; break;
        case LearnerAction::Spelling: state.screen = LearnerScreen::Spelling; break;
        case LearnerAction::Flashcards: state.screen = LearnerScreen::Flashcards; break;
        case LearnerAction::Curriculum: state.screen = LearnerScreen::Curriculum; break;
        case LearnerAction::Journey: state.screen = LearnerScreen::Journey; break;
        default: return false;
      }
      break;
  }
  return previous.screen != state.screen || previous.help_open != state.help_open;
}
