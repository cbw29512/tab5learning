#pragma once
#include "curriculum_preview.generated.h"
#include <cstring>
namespace curriculum_preview {
inline constexpr const char* subjects[]={"Mathematics","Reading and writing","Science","Social studies"};
inline constexpr std::size_t lesson_count=sizeof(lessons)/sizeof(lessons[0]);
inline constexpr std::size_t unit_count=sizeof(units)/sizeof(units[0]);
static_assert(sizeof(lesson_units)/sizeof(lesson_units[0])==lesson_count);
enum class Screen { Subjects, Lessons, Activity, Feedback, Finished, Units };
// Three large lesson tiles per page, independent of the number of lessons.
inline constexpr unsigned lessons_per_page=3;
inline constexpr unsigned lesson_pages(unsigned count){
  return count==0?1:1+(count-1)/lessons_per_page;
}
inline constexpr bool lesson_page_valid(unsigned page,unsigned count){
  return page<lesson_pages(count);
}
struct Preview {
  Screen screen=Screen::Subjects;
  unsigned subject=0,page=0,unit_page=0;
  int unit=-1; // -1 is the legacy flat browser; grouped UI chooses a unit first.
  bool grouped=false;
  bool practice_only=false; // Optional extra practice; never checkpoint or award.
  bool finished_only=false,reviewing=false; // Transient browser state; never persisted as a grade.
  std::size_t lesson=0,step=0;
  int selected=-1;
  void reset(){*this={};}
  bool choose_subject(unsigned value){
    if(screen!=Screen::Subjects||value>=4)return false;
    subject=value;page=0;unit_page=0;unit=-1;grouped=false;screen=Screen::Lessons;return true;
  }
  bool choose_subject_grouped(unsigned value){
    if(!choose_subject(value))return false;
    grouped=true;screen=Screen::Units;return true;
  }
  unsigned units_in_subject()const{
    unsigned count=0;for(const auto& u:units)if(u.subject==subject)++count;
    return count;
  }
  const Unit* visible_unit(unsigned row)const{
    if(row>=lessons_per_page)return nullptr;
    auto n=unit_page*lessons_per_page+row;
    // Surface the illustrated reading topic first without changing stable unit IDs.
    const Unit* featured=nullptr;
    if(subject==1)for(const auto& u:units)
      if(u.subject==subject&&std::strcmp(u.key,"visual-reading")==0){featured=&u;break;}
    if(featured){if(n==0)return featured;--n;}
    for(const auto& u:units)if(u.subject==subject&& &u!=featured){if(n==0)return &u;--n;}
    return nullptr;
  }
  bool choose_unit(unsigned row){
    if(screen!=Screen::Units)return false;
    const auto* u=visible_unit(row);if(!u)return false;
    unit=static_cast<int>(u-units);page=0;finished_only=false;reviewing=false;practice_only=false;screen=Screen::Lessons;return true;
  }
  bool matches(const Lesson& l)const{
    return l.subject==subject&&(unit<0||lesson_units[&l-lessons]==static_cast<unsigned>(unit));
  }
  unsigned filtered_count()const{
    unsigned count=0;for(const auto& l:lessons)if(matches(l))++count;return count;
  }
  const Lesson* nth(unsigned value)const{
    for(const auto& l:lessons)if(matches(l)){
      if(value==0)return &l;
      --value;
    }
    return nullptr;
  }
  unsigned subject_count()const{
    unsigned count=0;for(const auto& l:lessons)if(l.subject==subject)++count;
    return count;
  }
  bool change_page(int direction){
    if(screen!=Screen::Lessons&&screen!=Screen::Units)return false;
    auto& active=screen==Screen::Units?unit_page:page;
    const auto count=screen==Screen::Units?units_in_subject():filtered_count();
    if(direction==-1&&active>0){--active;return true;}
    if(direction==1&&lesson_page_valid(active+1,count)){++active;return true;}
    return false;
  }
  const Lesson* visible_lesson(unsigned row)const{
    return row<lessons_per_page?nth(page*lessons_per_page+row):nullptr;
  }
  bool choose_lesson(unsigned value){
    if(screen!=Screen::Lessons)return false;
    const auto* l=visible_lesson(value);if(!l)return false;
    lesson=static_cast<std::size_t>(l-lessons);step=0;selected=-1;
    screen=Screen::Activity;return true;
  }
  const Lesson& current()const{return lessons[lesson];}
  const Activity& activity()const{return current().activities[step];}
  bool answer(unsigned value){
    if(screen!=Screen::Activity||activity().kind!=Kind::Choice||value>=activity().count)return false;
    selected=static_cast<int>(value);screen=Screen::Feedback;return true;
  }
  bool next(){
    if(screen==Screen::Activity){
      if(activity().kind==Kind::Choice)return false;
      if(activity().kind==Kind::AdultReview){screen=Screen::Feedback;return true;}
    }else if(screen!=Screen::Feedback)return false;
    selected=-1;
    if(step+1>=current().count)screen=Screen::Finished;
    else{++step;screen=Screen::Activity;}
    return true;
  }
  bool back(){
    switch(screen){
      case Screen::Subjects:return false;
      case Screen::Units:screen=Screen::Subjects;return true;
      case Screen::Lessons:screen=grouped?Screen::Units:Screen::Subjects;unit=-1;page=0;finished_only=false;reviewing=false;practice_only=false;return true;
      default:screen=Screen::Lessons;selected=-1;reviewing=false;practice_only=false;return true;
    }
  }
};
}
