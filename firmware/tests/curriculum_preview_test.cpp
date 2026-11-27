#include "curriculum_preview.h"
#include "learner_layout.h"
#include <cassert>
using namespace curriculum_preview;
int main(){
  static_assert(lesson_count==44);
  static_assert(lesson_pages(0)==1&&lesson_pages(3)==1&&lesson_pages(4)==2);
  static_assert(lesson_pages(9)==3&&lesson_pages(10)==4);
  static_assert(!lesson_page_valid(3,9)&&lesson_page_valid(3,10));
  // Home tiles must open each subject directly; legacy Math is not a tile.
  static_assert(learner_tiles.size()==5);
  for(unsigned s=0;s<4;++s){
    LearnerNavigation nav;LearnerAction action;
    const auto& tile=learner_tiles[s];
    assert(learner_hit(nav,tile.x+1,tile.y+1,action));
    assert(learner_subject(action)==static_cast<int>(s));
    assert(learner_navigate(nav,action)&&nav.screen==LearnerScreen::Curriculum);
    Preview route;assert(route.choose_subject(s)&&route.screen==Screen::Lessons);
    assert(route.choose_lesson(0)&&route.back()&&route.screen==Screen::Lessons);
    assert(learner_navigate(nav,LearnerAction::Back)&&nav.screen==LearnerScreen::Home);
  }
  assert(learner_subject(learner_tiles[4].action)==-1);
  unsigned visited=0,activities=0;
  Preview p;
  for(unsigned s=0;s<4;++s){
    p.reset();assert(p.choose_subject(s));
    assert(p.page==0&&p.subject_count()==11);
    assert(!p.change_page(-1));
    assert(!p.change_page(0)&&p.visible_lesson(3)==nullptr);
    for(unsigned page=0;page<4;++page){
    if(page>0)assert(p.change_page(1)&&p.page==page);
    const unsigned rows=page==3?2:3;
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
    assert(!p.change_page(1)&&p.change_page(-1)&&p.page==2);
    assert(p.change_page(-1)&&p.page==1);
    assert(p.change_page(-1)&&p.page==0);
    assert(p.back());assert(!p.back());
  }
  assert(visited==44&&activities==264);
}
