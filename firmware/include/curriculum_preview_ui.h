#pragma once
#include <M5Unified.h>
#include <cstdio>
#include <cstring>
#include "curriculum_preview.h"
namespace curriculum_preview {
// Font 0 at size 3: six pixels per character before scaling. Conservative
// 60-column wrap fits within 1080 pixels; fixed buffer, no String allocations.
inline void wrapped(const char* text,int x,int y,unsigned max_lines){
  char line[61];unsigned used=0,lines=0;
  while(*text&&lines<max_lines){
    while(*text==' ')++text;
    if(!*text)break;
    const char* end=text;while(*end&&*end!=' ')++end;
    auto n=static_cast<unsigned>(end-text);
    if(n>60)n=60;
    if(used&&used+1+n>60){
      line[used]='\0';M5.Display.drawString(line,x,y+30*lines++);
      used=0;if(lines>=max_lines)return;
    }
    if(used)line[used++]=' ';
    std::memcpy(line+used,text,n);used+=n;text+=n;
  }
  if(used&&lines<max_lines){line[used]='\0';M5.Display.drawString(line,x,y+30*lines);}
}
inline bool hit(int x,int y,int top,int height){
  return x>=48&&x<1232&&y>=top&&y<top+height;
}
inline void button(const char* label,int y,int h){
  M5.Display.fillRoundRect(48,y,1184,h,12,TFT_BLUE);
  M5.Display.setTextColor(TFT_WHITE,TFT_BLUE);M5.Display.setTextSize(3);
  M5.Display.drawString(label,72,y+(h-24)/2);
}
inline void page_button(const char* label,int x){
  M5.Display.fillRoundRect(x,488,568,64,12,TFT_BLUE);
  M5.Display.setTextColor(TFT_WHITE,TFT_BLUE);M5.Display.setTextSize(3);
  M5.Display.drawString(label,x+24,508);
}
inline void render(const Preview& p){
  M5.Display.setTextSize(2);M5.Display.setTextColor(TFT_WHITE,TFT_NAVY);
  M5.Display.drawString("Grade 1 draft preview - practice only; no progress saved",48,144);
  if(p.screen==Screen::Subjects){
    for(unsigned i=0;i<4;++i)button(subjects[i],184+90*i,72);
    return;
  }
  if(p.screen==Screen::Lessons){
    for(unsigned i=0;i<lessons_per_page;++i){
      const auto* l=p.visible_lesson(i);if(l)button(l->title,184+90*i,72);
    }
    char label[48];
    std::snprintf(label,sizeof(label),"Page %u of %u",p.page+1,lesson_pages(p.subject_count()));
    M5.Display.setTextColor(TFT_WHITE,TFT_NAVY);M5.Display.setTextSize(2);
    M5.Display.drawString(label,48,454);
    if(p.page>0)page_button("Previous page",48);
    if(lesson_page_valid(p.page+1,p.subject_count()))page_button("Next page",664);
    return;
  }
  M5.Display.setTextSize(3);M5.Display.setTextColor(TFT_WHITE,TFT_NAVY);
  M5.Display.drawString(p.current().title,48,178);
  if(p.screen==Screen::Finished){
    wrapped("Preview finished. No grade, mastery or achievement was recorded.",48,230,5);
    button("Choose another lesson",488,64);return;
  }
  char label[96];
  std::snprintf(label,sizeof(label),"Activity %u of %u - %s",
    static_cast<unsigned>(p.step+1),static_cast<unsigned>(p.current().count),p.activity().phase);
  M5.Display.setTextSize(2);M5.Display.drawString(label,48,218);
  M5.Display.setTextSize(3);
  if(p.screen==Screen::Feedback){
    if(p.activity().kind==Kind::Choice){
      M5.Display.drawString(p.selected==p.activity().answer?"Correct":"Practice this again",48,250);
    }else M5.Display.drawString("Adult guidance - not graded",48,250);
    wrapped(p.activity().feedback,48,290,5);
    button("Continue",488,64);return;
  }
  wrapped(p.activity().prompt,48,250,5);
  if(p.activity().kind==Kind::Choice){
    for(unsigned i=0;i<p.activity().count;++i)button(p.activity().choices[i],408+48*i,44);
  }else button(p.activity().kind==Kind::AdultReview?"View adult guidance":"Continue",488,64);
}
inline bool touch(Preview& p,int x,int y){
  if(p.screen==Screen::Subjects){
    for(unsigned i=0;i<4;++i)if(hit(x,y,184+90*i,72))return p.choose_subject(i);
    return false;
  }
  if(p.screen==Screen::Lessons){
    for(unsigned i=0;i<lessons_per_page;++i)
      if(hit(x,y,184+90*i,72))return p.choose_lesson(i);
    if(y>=488&&y<552){
      if(x>=48&&x<616)return p.change_page(-1);
      if(x>=664&&x<1232)return p.change_page(1);
    }
    return false;
  }
  if(p.screen==Screen::Activity&&p.activity().kind==Kind::Choice){
    for(unsigned i=0;i<p.activity().count;++i)if(hit(x,y,408+48*i,44))return p.answer(i);
    return false;
  }
  if(!hit(x,y,488,64))return false;
  if(p.screen==Screen::Finished)return p.back();
  return p.next();
}
}
