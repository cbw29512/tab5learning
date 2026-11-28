#include "curriculum_preview.h"
#include "learner_layout.h"
#include "curriculum_visuals.h"
#include <cassert>
using namespace curriculum_preview;
int main(){
  static_assert(lesson_count==127);
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
    assert(p.page==0&&p.subject_count()==(s==0?27u:s==1?61u:s==2?17u:22u));
    assert(!p.change_page(-1));
    assert(!p.change_page(0)&&p.visible_lesson(3)==nullptr);
    for(unsigned page=0;page<lesson_pages(p.subject_count());++page){
    if(page>0)assert(p.change_page(1)&&p.page==page);
    const unsigned left=p.subject_count()-page*lessons_per_page;
    const unsigned rows=left<lessons_per_page?left:lessons_per_page;
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
    assert(!p.change_page(1));
    while(p.page>0){const auto previous=p.page-1;assert(p.change_page(-1)&&p.page==previous);}
    assert(!p.change_page(-1));
    assert(p.back());assert(!p.back());
  }
  assert(visited==127&&activities==762);
  // Every lesson remains reachable exactly once through the real grouped path.
  bool reached[lesson_count]={};unsigned total=0;
  for(unsigned s=0;s<4;++s){
    Preview g;assert(g.choose_subject_grouped(s)&&g.screen==Screen::Units);
    for(unsigned up=0;up<lesson_pages(g.units_in_subject());++up){
      if(up)assert(g.change_page(1));
      for(unsigned ur=0;ur<lessons_per_page;++ur){
        const auto* u=g.visible_unit(ur);if(!u){assert(!g.choose_unit(ur));continue;}
        assert(g.choose_unit(ur)&&g.screen==Screen::Lessons&&g.page==0);
        assert(g.filtered_count()>0);
        for(unsigned lp=0;lp<lesson_pages(g.filtered_count());++lp){
          if(lp)assert(g.change_page(1));
          for(unsigned lr=0;lr<lessons_per_page;++lr){
            if(!g.visible_lesson(lr)){assert(!g.choose_lesson(lr));continue;}
            assert(g.choose_lesson(lr)&&g.current().subject==s);
            assert(lesson_units[g.lesson]==static_cast<unsigned>(u-units));
            assert(!reached[g.lesson]);reached[g.lesson]=true;++total;
            assert(g.back()&&g.screen==Screen::Lessons&&g.page==lp);
          }
        }
        assert(!g.change_page(1));
        assert(g.back()&&g.screen==Screen::Units&&g.unit_page==up&&g.unit==-1);
      }
    }
    assert(!g.change_page(1));assert(g.back()&&g.screen==Screen::Subjects);
  }
  assert(total==lesson_count);
  unsigned illustrated=0;
  for(const auto& l:lessons){
    const auto* v=visual_for(l,0);if(!v)continue;
    ++illustrated;assert(v->count>0&&l.count==6);
    for(unsigned i=0;i<l.count;++i)assert(visual_for(l,i)==v);
    assert(!visual_for(l,l.count));
    auto changed=l;++changed.revision;assert(!visual_for(changed,0));
    assert((v==&park&&std::strcmp(l.id,"g1-picture-or-words")==0)||
           (v==&garden&&std::strcmp(l.id,"g1-pictures-explain-details")==0));
  }
  assert(illustrated==2);
  // The physical path to pictures starts at the first reading topic page.
  Preview featured;assert(featured.choose_subject_grouped(1));
  assert(featured.visible_unit(0)&&std::strcmp(featured.visible_unit(0)->key,"visual-reading")==0);
  assert(featured.choose_unit(0)&&featured.filtered_count()==2);
  assert(featured.choose_lesson(0)&&visual_for(featured.current(),0)==&park);
  assert(featured.back()&&featured.choose_lesson(1)&&visual_for(featured.current(),0)==&garden);
}
