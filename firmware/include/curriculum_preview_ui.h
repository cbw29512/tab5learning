#pragma once
#include <M5Unified.h>
#include <cstdio>
#include <cstring>
#include "curriculum_browse.h"
#include "curriculum_visual_ui.h"
namespace curriculum_preview {
// Font 0 at size 3: six pixels per character before scaling. Conservative
// 60-column wrap fits within 1080 pixels; fixed buffer, no String allocations.
inline void wrapped(const char* text,int x,int y,unsigned max_lines,unsigned columns=60){
  if(columns==0||columns>60)return;
  char line[61];unsigned used=0,lines=0;
  while(*text&&lines<max_lines){
    while(*text==' ')++text;
    if(!*text)break;
    const char* end=text;while(*end&&*end!=' ')++end;
    auto n=static_cast<unsigned>(end-text);
    if(n>columns)n=columns;
    if(used&&used+1+n>columns){
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
inline void render(const Preview& p,bool resume_ok,const CompletionLedger* ledger=nullptr){
  M5.Display.setTextSize(2);M5.Display.setTextColor(TFT_WHITE,TFT_NAVY);
#ifdef POCKET_LEARNER_SD_EVAL
  const char* banner=!resume_ok?"Resume storage unavailable - ask your teacher":
    (!ledger||!ledger->available())?"SD history unavailable - cannot finish lesson":
    "Grade 1 SD evaluation - completion only; no grades";
  M5.Display.drawString(banner,48,144);
#else
  M5.Display.drawString(resume_ok?"Grade 1 draft - resume location saved; no grades":"Resume storage unavailable - ask your teacher",48,144);
#endif
  if(p.screen==Screen::Subjects){
    for(unsigned i=0;i<4;++i)button(subjects[i],184+90*i,72);
    return;
  }
  if(p.screen==Screen::Units){
    for(unsigned i=0;i<lessons_per_page;++i){
      const auto* u=p.visible_unit(i);if(!u)continue;
      const int y=184+90*i;
      button(u->title,y,72);
      if(std::strcmp(u->key,"visual-reading")==0)draw_visual_at(park,1044,y+11,3);
    }
    char label[48];
    std::snprintf(label,sizeof(label),"Topics %u of %u",p.unit_page+1,lesson_pages(p.units_in_subject()));
    M5.Display.setTextColor(TFT_WHITE,TFT_NAVY);M5.Display.setTextSize(2);
    M5.Display.drawString(label,48,454);
    if(p.unit_page>0)page_button("Previous page",48);
    if(lesson_page_valid(p.unit_page+1,p.units_in_subject()))page_button("Next page",664);
    return;
  }
  if(p.screen==Screen::Lessons){
    const bool known=browse_known(p,ledger);
    const auto count=browse_count(p,ledger);
    for(unsigned i=0;i<lessons_per_page;++i){
      const auto* l=browse_visible(p,i,ledger);
      if(l){
        const int y=184+90*i;
        M5.Display.fillRoundRect(48,y,1184,72,12,TFT_BLUE);
        M5.Display.setTextColor(TFT_WHITE,TFT_BLUE);M5.Display.setTextSize(3);
        M5.Display.drawString(l->title,72,y+10);
        const auto status=lesson_progress(ledger,*l);
        M5.Display.setTextSize(2);
        M5.Display.setTextColor(status==CompletionLookup::Recorded?TFT_GREEN:TFT_WHITE,TFT_BLUE);
        M5.Display.drawString(completion_label(status),72,y+42);
        if(const auto* art=visual_for(*l,0))draw_visual_at(*art,1044,y+11,3);
      }
    }
    char label[48];
    std::snprintf(label,sizeof(label),"Page %u of %u",p.page+1,lesson_pages(count));
    M5.Display.setTextColor(TFT_WHITE,TFT_NAVY);M5.Display.setTextSize(2);
    M5.Display.drawString(label,48,454);
    if(known){
      M5.Display.fillRoundRect(410,442,388,38,6,p.finished_only?TFT_DARKGREY:TFT_BLUE);
      M5.Display.fillRoundRect(812,442,420,38,6,p.finished_only?TFT_BLUE:TFT_DARKGREY);
      M5.Display.setTextColor(TFT_WHITE,p.finished_only?TFT_DARKGREY:TFT_BLUE);
      M5.Display.drawString("Up next",430,453);
      M5.Display.setTextColor(TFT_WHITE,p.finished_only?TFT_BLUE:TFT_DARKGREY);
      M5.Display.drawString("Finished lessons",832,453);
      if(count==0){
        M5.Display.setTextColor(TFT_WHITE,TFT_NAVY);
        M5.Display.drawString(p.finished_only?"No finished lessons yet":"All lessons in this topic finished",48,210);
      }
    }else M5.Display.drawString("All lessons - history unavailable",410,454);
    if(p.page>0)page_button("Previous page",48);
    if(lesson_page_valid(p.page+1,count))page_button("Next page",664);
    return;
  }
  M5.Display.setTextSize(3);M5.Display.setTextColor(TFT_WHITE,TFT_NAVY);
  M5.Display.drawString(p.current().title,48,178);
  if(p.screen==Screen::Finished){
    const auto saved=lesson_progress(ledger,p.current())==CompletionLookup::Recorded;
    wrapped(p.reviewing?"Review finished. No new completion or grade was recorded.":saved?"Lesson completion saved. No grade, mastery or achievement was recorded.":
                  "Preview finished. No verified completion history or grade was recorded.",48,230,5);
    button(p.reviewing?"Return to finished lessons":saved?"See up next":"Choose another lesson",488,64);return;
  }
  char label[96];
  std::snprintf(label,sizeof(label),"Activity %u of %u - %s",
    static_cast<unsigned>(p.step+1),static_cast<unsigned>(p.current().count),p.activity().phase);
  M5.Display.setTextSize(2);M5.Display.drawString(label,48,218);
  M5.Display.setTextSize(3);
  const auto* visual=visual_for(p.current(),p.step);
  if(visual){
    std::printf("CURRICULUM,VISUAL,%s,%u,%u\n",p.current().id,
      static_cast<unsigned>(p.step+1),static_cast<unsigned>(visual->count));
    // A visible marker distinguishes an illustrated activity from text-only ones.
    M5.Display.setTextSize(2);
    M5.Display.drawString("Picture activity",1020,218);
    M5.Display.setTextSize(3);
  }
  if(p.screen==Screen::Feedback){
    const int text_x=visual?visual_text_x:48;
    if(visual)draw_visual(*visual);
    if(p.activity().kind==Kind::Choice){
      M5.Display.drawString(p.selected==p.activity().answer?"Correct":"Practice this again",text_x,250);
    }else M5.Display.drawString("Adult guidance - not graded",text_x,250);
    wrapped(p.activity().feedback,text_x,290,visual?4:5,visual?visual_columns:60);
    button("Continue",488,64);return;
  }
  if(visual)draw_visual(*visual);
  wrapped(p.activity().prompt,visual?visual_text_x:48,250,5,visual?visual_columns:60);
  if(p.activity().kind==Kind::Choice){
    for(unsigned i=0;i<p.activity().count;++i)button(p.activity().choices[i],408+48*i,44);
  }else button(p.activity().kind==Kind::AdultReview?"View adult guidance":"Continue",488,64);
}
inline bool touch(Preview& p,int x,int y,const CompletionLedger* ledger=nullptr){
  if(p.screen==Screen::Subjects){
    for(unsigned i=0;i<4;++i)if(hit(x,y,184+90*i,72))return p.choose_subject_grouped(i);
    return false;
  }
  if(p.screen==Screen::Lessons||p.screen==Screen::Units){
    if(p.screen==Screen::Lessons&&y>=442&&y<480){
      if(x>=410&&x<798)return browse_filter(p,false,ledger);
      if(x>=812&&x<1232)return browse_filter(p,true,ledger);
    }
    for(unsigned i=0;i<lessons_per_page;++i)
      if(hit(x,y,184+90*i,72))return p.screen==Screen::Units?p.choose_unit(i):browse_choose(p,i,ledger);
    if(y>=488&&y<552){
      if(x>=48&&x<616)return p.screen==Screen::Units?p.change_page(-1):browse_page(p,-1,ledger);
      if(x>=664&&x<1232)return p.screen==Screen::Units?p.change_page(1):browse_page(p,1,ledger);
    }
    return false;
  }
  if(p.screen==Screen::Activity&&p.activity().kind==Kind::Choice){
    for(unsigned i=0;i<p.activity().count;++i)if(hit(x,y,408+48*i,44))return p.answer(i);
    return false;
  }
  if(!hit(x,y,488,64))return false;
  if(p.screen==Screen::Finished)return browse_after_finish(p,ledger);
  return p.next();
}
}
