#include "reading_preview.h"
#include <cassert>

int main() {
  ReadingPreview state;
  assert(state.selected == -1);
  for (std::size_t i=0;i<reading_words.size();++i) {
    const auto& w=reading_words[i];
    assert(state.select(w.x,w.y));
    assert(state.selected==static_cast<int>(i));
    assert(state.select(w.x+w.w-1,w.y+63));
    assert(state.selected==static_cast<int>(i));
    assert(!state.select(w.x+w.w,w.y));
    assert(!state.select(w.x,w.y+64));
  }
  assert(!state.select(-1,-1));
  assert(!state.select(32,580)); // Back is navigation, not a word.
  assert(!state.select(1068,24)); // Help is navigation, not a word.
  assert(state.selected==9);
  state.reset();
  assert(state.selected==-1);
}
