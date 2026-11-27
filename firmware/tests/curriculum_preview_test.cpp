#include "curriculum_preview.h"
#include <cassert>
using namespace curriculum_preview;
int main(){
  static_assert(lesson_count==28);
  static_assert(lesson_pages(0)==1&&lesson_pages(3)==1&&lesson_pages(4)==2);
  static_assert(lesson_pages(9)==3&&lesson_pages(10)==4);
  static_assert(!lesson_page_valid(3,9)&&lesson_page_valid(3,10));
  unsigned visited=0,activities=0;
  Preview p;
  for(unsigned s=0;s<4;++s){
    p.reset();assert(p.choose_subject(s));
    assert(p.page==0&&p.subject_count()==7);
    assert(!p.change_page(-1));
    assert(!p.change_page(0)&&p.visible_lesson(3)==nullptr);
    for(unsigned page=0;page<3;++page){
    if(page>0)assert(p.change_page(1)&&p.page==page);
    const unsigned rows=page==2?1:3;
    for(unsigned i=0;i<rows;++i){
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
    assert(!p.choose_lesson(rows));
    }
    assert(!p.change_page(1)&&p.change_page(-1)&&p.page==1);
    assert(p.change_page(-1)&&p.page==0);
    assert(p.back());assert(!p.back());
  }
  assert(visited==28&&activities==168);
}
