#include "curriculum_preview.h"
#include <cassert>
using namespace curriculum_preview;
int main(){
  static_assert(lesson_count==12);
  unsigned visited=0,activities=0;
  Preview p;
  for(unsigned s=0;s<4;++s){
    p.reset();assert(p.choose_subject(s));
    for(unsigned i=0;i<3;++i){
      assert(p.choose_lesson(i));++visited;
      assert(p.current().subject==s);
      while(p.screen!=Screen::Finished){
        assert(p.screen==Screen::Activity);++activities;
        const auto& a=p.activity();
        if(a.kind==Kind::Choice){
          assert(!p.next());assert(!p.answer(a.count));
          assert(a.answer>=0&&unsigned(a.answer)<a.count);
          assert(p.answer(unsigned(a.answer)));assert(p.screen==Screen::Feedback);
          assert(!p.answer(0));assert(p.next());
        }else if(a.kind==Kind::AdultReview){
          assert(!p.answer(0));assert(p.next()&&p.screen==Screen::Feedback);
          assert(p.selected==-1);assert(p.next());
        }else assert(p.next());
      }
      assert(!p.next());assert(p.back());
    }
    assert(!p.choose_lesson(3));assert(p.back());assert(!p.back());
  }
  assert(visited==12&&activities==72);
}
