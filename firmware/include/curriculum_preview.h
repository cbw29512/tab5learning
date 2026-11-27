#pragma once
#include "curriculum_preview.generated.h"
namespace curriculum_preview {
inline constexpr const char* subjects[]={"Mathematics","Reading and writing","Science","Social studies"};
inline constexpr std::size_t lesson_count=sizeof(lessons)/sizeof(lessons[0]);
enum class Screen { Subjects, Lessons, Activity, Feedback, Finished };
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
  unsigned subject=0,page=0;
  std::size_t lesson=0,step=0;
  int selected=-1;
  void reset(){*this={};}
  bool choose_subject(unsigned value){
    if(screen!=Screen::Subjects||value>=4)return false;
    subject=value;page=0;screen=Screen::Lessons;return true;
  }
  const Lesson* nth(unsigned value)const{
    for(const auto& l:lessons)if(l.subject==subject){
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
    if(screen!=Screen::Lessons)return false;
    if(direction==-1&&page>0){--page;return true;}
    if(direction==1&&lesson_page_valid(page+1,subject_count())){++page;return true;}
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
      case Screen::Lessons:screen=Screen::Subjects;return true;
      default:screen=Screen::Lessons;selected=-1;return true;
    }
  }
};
}
