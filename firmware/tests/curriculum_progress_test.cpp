#include "curriculum_browse.h"
#include "curriculum_achievements.h"
#include <cassert>
#include <vector>
using namespace curriculum_preview;
struct Media:CompletionBackend{
  std::vector<CompletionBytes> records;
  bool invalid=false;
  bool size(std::uint64_t& n)override{n=records.size()*96+unsigned(invalid);return true;}
  bool read(unsigned i,CompletionBytes& b)override{
    if(i>=records.size())return false;
    b=records[i];return true;
  }
  bool append(unsigned i,const CompletionBytes& b)override{
    if(i!=records.size())return false;
    records.push_back(b);return true;
  }
};
int main(){
  for(unsigned s=0;s<4;++s){
    auto p=subject_progress(nullptr,s);
    assert(!p.known&&p.total==(s==0?27u:s==1?61u:s==2?17u:22u)&&p.completed==0);
  }
  assert(!subject_progress(nullptr,4).known);
  unsigned topic_total=0,nonempty=0;
  for(unsigned u=0;u<unit_count;++u){
    const auto p=topic_progress(nullptr,u);
    assert(!p.known&&!topic_finished(p));
    topic_total+=p.total;if(p.total)++nonempty;
  }
  assert(topic_total==lesson_count);
  assert(!topic_progress(nullptr,unit_count).known);
  assert(!topic_finished({0,0,true}));
  assert(topic_milestones(nullptr).total==nonempty&&!topic_milestones(nullptr).known);
  assert(grade_progress(nullptr).total==lesson_count&&!grade_progress(nullptr).known);
  for(const auto& def:achievements)assert(achievement_progress(def,grade_progress(nullptr),topic_milestones(nullptr)).state==AchievementState::Unknown);
  Media media;CompletionLedger ledger(media);
  assert(!subject_progress(&ledger,0).known);
  assert(ledger.begin());
  for(const auto& def:achievements)assert(achievement_progress(def,grade_progress(&ledger),topic_milestones(&ledger)).state==AchievementState::Locked);
  auto p=subject_progress(&ledger,0);
  assert(p.known&&p.total==27&&p.completed==0);
  Preview lesson;
  assert(lesson.choose_subject(0)&&lesson.change_page(1)&&lesson.choose_lesson(0));
  assert(lesson_progress(&ledger,lesson.current())==CompletionLookup::NotRecorded);
  while(lesson.screen!=Screen::Finished){
    if(lesson.screen==Screen::Activity&&lesson.activity().kind==Kind::Choice)assert(lesson.answer(0));
    else assert(lesson.next());
  }
  assert(ledger.finish(lesson)==CompletionResult::Saved);
  assert(ledger.finish(lesson)==CompletionResult::AlreadyRecorded);
  assert(grade_progress(&ledger).completed==1);
  assert(achievement_progress(achievements[0],grade_progress(&ledger),topic_milestones(&ledger)).state==AchievementState::Earned);
  assert(achievement_progress(achievements[1],grade_progress(&ledger),topic_milestones(&ledger)).state==AchievementState::Locked);
  assert(subject_progress(&ledger,0).completed==1);
  assert(subject_progress(&ledger,1).completed==0);
  const auto unit=lesson_units[lesson.lesson];
  assert(topic_progress(&ledger,unit).completed==1);
  assert(topic_milestones(&ledger).known);
  // Historical revision remains in the ledger without increasing current progress.
  CompletionEvent historical{};assert(completion_decode(media.records[0],historical));
  historical.key.revision=99;historical.sequence=2;
  CompletionBytes b{};assert(completion_encode(historical,b));media.records.push_back(b);
  assert(ledger.begin()&&ledger.verified_count()==2);
  assert(subject_progress(&ledger,0).completed==1);
  assert(lesson_progress(&ledger,lesson.current())==CompletionLookup::Recorded);
  Preview browser;assert(browser.choose_subject(0));
  assert(browse_count(browser,&ledger)==26);
  assert(browse_filter(browser,true,&ledger)&&browse_count(browser,&ledger)==1);
  assert(browse_visible(browser,0,&ledger)==&lesson.current());
  assert(browse_choose(browser,0,&ledger)&&browser.reviewing&&browser.step==0);
  assert(browser.back()&&!browser.reviewing&&browser.finished_only);
  assert(browse_filter(browser,false,&ledger)&&browse_count(browser,&ledger)==26);
  browser.page=999;browse_clamp(browser,&ledger);assert(browser.page==8);
  assert(!browse_page(browser,1,&ledger));
  assert(browse_count(browser,nullptr)==27&&!browse_filter(browser,true,nullptr));
  // Finish the remaining lessons in one topic and replay the ledger.
  for(std::size_t i=0;i<lesson_count;++i){
    if(lesson_units[i]!=unit||i==lesson.lesson)continue;
    Preview done{};
    done.lesson=i;done.step=done.current().count-1;done.screen=Screen::Finished;
    assert(ledger.finish(done)==CompletionResult::Saved);
    assert(ledger.finish(done)==CompletionResult::AlreadyRecorded);
  }
  assert(topic_finished(topic_progress(&ledger,unit)));
  assert(topic_milestones(&ledger).completed==1);
  CompletionLedger reopened(media);assert(reopened.begin());
  assert(topic_milestones(&reopened).completed==1);
  assert(achievement_progress(achievements[2],grade_progress(&reopened),topic_milestones(&reopened)).state==AchievementState::Earned);
  // Every current lesson completed; historic revision and replays never inflate totals.
  for(std::size_t i=0;i<lesson_count;++i){
    Preview done{};done.lesson=i;done.step=done.current().count-1;done.screen=Screen::Finished;
    const auto r=reopened.finish(done);assert(r==CompletionResult::Saved||r==CompletionResult::AlreadyRecorded);
  }
  assert(grade_progress(&reopened).completed==lesson_count);
  for(const auto& def:achievements){const auto a=achievement_progress(def,grade_progress(&reopened),topic_milestones(&reopened));assert(a.state==AchievementState::Earned&&a.completed==a.target);}
  assert(ledger.begin());
  media.invalid=true;assert(!ledger.begin());
  assert(!topic_milestones(&ledger).known&&!topic_finished(topic_progress(&ledger,unit)));
  assert(achievement_progress(achievements[0],grade_progress(&ledger),topic_milestones(&ledger)).state==AchievementState::Unknown);
  browser.finished_only=true;browse_clamp(browser,&ledger);
  assert(!browser.finished_only&&browse_count(browser,&ledger)==27);
  assert(!subject_progress(&ledger,0).known);
  assert(lesson_progress(&ledger,lesson.current())==CompletionLookup::Unavailable);
}
