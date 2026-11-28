#pragma once
#include <cstddef>
namespace curriculum_preview {
enum class Kind { Instruction, Choice, AdultReview };
struct Activity {
  Kind kind;
  const char* phase;
  const char* prompt;
  const char* feedback;
  const char* choices[3];
  unsigned count;
  int answer;
};
struct Unit {
  unsigned subject;
  const char* key;
  const char* title;
};
struct Lesson {
  unsigned subject;
  const char* id;
  unsigned revision;
  const char* title;
  const Activity* activities;
  std::size_t count;
};
}
