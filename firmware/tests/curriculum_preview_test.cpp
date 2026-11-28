#include "curriculum_preview.h"
#include "learner_layout.h"
#include "curriculum_visuals.h"
#include "curriculum_answer_layout.h"
#include "curriculum_feedback.h"
#include "curriculum_text_layout.h"
#include "curriculum_session_evidence.h"
#include <string>
#include <cassert>
using namespace curriculum_preview;
int main(){
  static_assert(lesson_count==127);
  std::string reconstructed;
  assert(wrap_lines("abcdefghijk",4,[&](const char* line,unsigned){reconstructed+=line;})==3);
  assert(reconstructed=="abcdefghijk");
  assert(wrap_lines("   ",4,[](const char*,unsigned){})==0);
  assert(wrap_lines("one two three",0,[](const char*,unsigned){})==0);
  static_assert(answer_rect(0).y>=400&&answer_rect(0).h==64);
  static_assert(answer_rect(2).y+answer_rect(2).h<learner_choice_back.y);
  static_assert(learner_choice_back.y+learner_choice_back.h<=720);
  static_assert(answer_index(0,48,408)==-1&&answer_index(4,48,408)==-1);
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
  SessionEvidence evidence;
  unsigned independent_count=0;
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
        const bool visual=visual_for(p.current(),p.step)!=nullptr;
        for(bool feedback:{false,true}){
          // Instruction activities advance directly and have no feedback screen.
          // Check that contract instead of requiring text that is never rendered.
          if(feedback&&a.kind==Kind::Instruction){
            assert(a.feedback&&a.feedback[0]=='\0');
            continue;
          }
          const auto* text=feedback?a.feedback:a.prompt;
          const auto layout=activity_text_layout(text,visual,feedback);
          const auto count=wrap_lines(text,layout.columns,[&](const char* line,unsigned){
            assert(std::strlen(line)<=layout.columns);
            assert(std::strlen(line)*6*layout.font<=unsigned(visual?612:1184));
          });
          assert(count>0&&count<=layout.lines);
          assert((feedback?290u:250u)+(count-1)*layout.pitch+8*layout.font<=(feedback?448u:408u));
        }
        const auto progress=activity_progress(p);
        assert(progress.valid&&progress.visited==p.step+1&&progress.total==p.current().count);
        assert(activity_bar_width(progress)>0&&activity_bar_width(progress)<=progress_bar_width);
        assert(feedback_tone(p)==FeedbackTone::Invalid);
        if(a.kind==Kind::Choice){
          assert(large_answers(p)&&a.count>=2&&a.count<=answer_max);
          for(unsigned k=0;k<a.count;++k){
            const auto r=answer_rect(k);
            assert(answer_index(a.count,r.x,r.y)==static_cast<int>(k));
            assert(answer_index(a.count,r.x+r.w-1,r.y+r.h-1)==static_cast<int>(k));
            assert(answer_index(a.count,r.x,r.y+r.h)==-1);
            assert(answer_index(a.count,r.x+r.w,r.y)==-1);
            assert(std::strlen(a.choices[k])*6*answer_font(std::strlen(a.choices[k]))<=unsigned(r.w-48));
          }
          assert(answer_index(a.count,48,answer_rect(a.count).y)==-1);
          assert(!p.next());assert(!p.answer(a.count));
          assert(a.answer>=0&&unsigned(a.answer)<a.count);
          const auto before=p;
          assert(p.answer(unsigned(a.answer)));assert(p.screen==Screen::Feedback);
          const bool scored=independent_choice(a);
          assert(evidence.observe(before,p)==scored);
          if(scored)++independent_count;
          assert(!evidence.observe(before,p));
          assert(feedback_tone(p)==FeedbackTone::Correct);
          Preview incorrect=p;
          incorrect.selected=(a.answer+1)%a.count;
          assert(feedback_tone(incorrect)==FeedbackTone::Learn);
          incorrect.selected=-1;assert(feedback_tone(incorrect)==FeedbackTone::Invalid);
          assert(!p.answer(0));assert(p.next());
        }else if(a.kind==Kind::AdultReview){
          assert(!p.answer(0));assert(p.next()&&p.screen==Screen::Feedback);
          assert(p.selected==-1&&feedback_tone(p)==FeedbackTone::Guidance);assert(p.next());
        }else assert(p.next());
      }
      assert(activity_bar_width(activity_progress(p))==progress_bar_width);
      assert(!p.next());assert(p.back());
      assert(!activity_progress(p).valid);
    }
    assert(!p.choose_lesson(rows));
    }
    assert(!p.change_page(1));
    while(p.page>0){const auto previous=p.page-1;assert(p.change_page(-1)&&p.page==previous);}
    assert(!p.change_page(-1));
    assert(p.back());assert(!p.back());
  }
  assert(visited==127&&activities==762&&independent_count==166);
  unsigned observed=0;
  for(std::size_t i=0;i<lesson_count;++i){
    const auto e=evidence.skill(i);
    assert(e.correct==e.observed&&e.observed==e.possible);
    assert(practice_hint(e)==(e.observed?PracticeHint::AnswersCorrect:PracticeHint::NoEvidence));
    observed+=e.observed;
  }
  assert(observed==166);
  SessionEvidence fresh;
  Preview sample;
  assert(sample.choose_subject(0)&&sample.choose_lesson(0));
  while(!independent_choice(sample.activity())){
    if(sample.activity().kind==Kind::Choice)assert(sample.answer(0)&&sample.next());
    else assert(sample.next());
  }
  const auto before=sample;
  assert(sample.answer((unsigned(sample.activity().answer)+1)%sample.activity().count));
  auto review=sample;review.reviewing=true;
  assert(!fresh.observe(before,review));
  assert(fresh.observe(before,sample));
  assert(practice_hint(fresh.skill(sample.lesson))==PracticeHint::PracticeSuggested);
  sample.selected=sample.activity().answer;
  assert(!fresh.observe(before,sample)&&fresh.skill(sample.lesson).correct==0);
  assert(practice_hint({1,1,2})==PracticeHint::IncompleteEvidence);
  assert(SessionEvidence{}.skill(sample.lesson).observed==0);
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
    for(unsigned i=0;i<l.count;++i){
      const auto* scene=visual_for(l,i);assert(scene&&scene->count>0);
      if(l.subject==1)assert(scene==v);
      else if(i)assert(scene!=visual_for(l,i-1));
    }
    assert(!visual_for(l,l.count));
    auto changed=l;++changed.revision;assert(!visual_for(changed,0));
    assert((v==&park&&std::strcmp(l.id,"g1-picture-or-words")==0)||
           (v==&garden&&std::strcmp(l.id,"g1-pictures-explain-details")==0)||
           (v==&coins_views[0]&&std::strcmp(l.id,"g1-count-like-coins")==0)||
           (v==&addition_views[0]&&std::strcmp(l.id,"g1-add-tens-ones")==0)||
           (v==&subtraction_views[0]&&std::strcmp(l.id,"g1-subtract-tens")==0)||
           (v==&hour_views[0]&&std::strcmp(l.id,"g1-clock-hours")==0)||
           (v==&half_hour_views[0]&&std::strcmp(l.id,"g1-clock-half-hours")==0)||
           (v==&shares_views[0]&&std::strcmp(l.id,"g1-equal-shares")==0));
  }
  assert(illustrated==8);
  // Packed storage contains only the active primitives. Compare the actual
  // backing pointers/counts and representative geometry to the source.
  static_assert(coins_0_packed.size()==coins_0.count&&coins_0_packed.size()<192);
  static_assert(subtraction_3_packed.size()==subtraction_3.count);
  static_assert(hour_1_packed.size()==hour_1.count);
  static_assert(shares_5_packed.size()==shares_5.count);
  static_assert(coins_views[0].shapes==coins_0_packed.data());
  static_assert(hour_views[1].shapes==hour_1_packed.data());
  static_assert(shares_views[5].shapes==shares_5_packed.data());
  static_assert(coins_0_packed[1].x==coins_0.shapes[1].x&&
                coins_0_packed[1].color==coins_0.shapes[1].color);
  static_assert(!clock_scene(0,false).valid&&!clock_scene(13,true).valid);
  static_assert(!fraction_scene(6).valid);
  static_assert(hour_1.count==half_hour_5.count);
  static_assert(shares_5.count==shares_0.count+2);
  static_assert(coins_0.count==9&&coins_4.count==21); // 2 circles per coin + background
  static_assert(addition_1.count==1+50+4+2); // 24 + 30; 5 rods, 4 ones, plus sign
  static_assert(subtraction_3.count==1+90+70); // 90 - 70; crossed rods
  static_assert(!coin_scene(11,5).valid&&!subtraction_scene(3,4).valid);
  static_assert(!addition_scene(100,1).valid);
  // The physical path to pictures starts at the first reading topic page.
  Preview featured;assert(featured.choose_subject_grouped(1));
  assert(featured.visible_unit(0)&&std::strcmp(featured.visible_unit(0)->key,"visual-reading")==0);
  assert(featured.choose_unit(0)&&featured.filtered_count()==2);
  assert(featured.choose_lesson(0)&&visual_for(featured.current(),0)==&park);
  assert(featured.back()&&featured.choose_lesson(1)&&visual_for(featured.current(),0)==&garden);
}
