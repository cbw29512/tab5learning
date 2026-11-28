#pragma once

// Portable learner navigation: no diagnostic, USB or administrator routes.
enum class LearnerScreen { Home, Math, Reading, Spelling, Flashcards, Journey, Curriculum, Achievements };
enum class LearnerAction { Home, Back, Math, Reading, Spelling, Flashcards, Journey, Help, Curriculum, SubjectMath, SubjectEla, SubjectScience, SubjectSocial, Achievements };

// Subject tiles enter the shared curriculum, not the old standalone demos.
inline constexpr int learner_subject(LearnerAction action) {
  switch(action) {
    case LearnerAction::SubjectMath: return 0;
    case LearnerAction::SubjectEla: return 1;
    case LearnerAction::SubjectScience: return 2;
    case LearnerAction::SubjectSocial: return 3;
    default: return -1;
  }
}

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
      else if(state.screen==LearnerScreen::Achievements)state.screen=LearnerScreen::Journey;
      else state.screen = LearnerScreen::Home;
      break;
    case LearnerAction::Help:
      state.help_open = true;
      break;
    case LearnerAction::Achievements:
      if(state.help_open||state.screen!=LearnerScreen::Journey)return false;
      state.screen=LearnerScreen::Achievements;break;
    default:
      // Subject controls are available from home, not through a help overlay.
      if (state.screen != LearnerScreen::Home || state.help_open) return false;
      switch (action) {
        case LearnerAction::Math: state.screen = LearnerScreen::Math; break;
        case LearnerAction::Reading: state.screen = LearnerScreen::Reading; break;
        case LearnerAction::Spelling: state.screen = LearnerScreen::Spelling; break;
        case LearnerAction::Flashcards: state.screen = LearnerScreen::Flashcards; break;
        case LearnerAction::SubjectMath:
        case LearnerAction::SubjectEla:
        case LearnerAction::SubjectScience:
        case LearnerAction::SubjectSocial:
        case LearnerAction::Curriculum: state.screen = LearnerScreen::Curriculum; break;
        case LearnerAction::Journey: state.screen = LearnerScreen::Journey; break;
        default: return false;
      }
      break;
  }
  return previous.screen != state.screen || previous.help_open != state.help_open;
}
