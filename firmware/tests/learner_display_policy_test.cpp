#include "learner_display_policy.h"
#include "display_startup_refresh.h"
#include <cassert>
#include <limits>
#include <initializer_list>
int main(){
  LearnerNavigation nav;
  for(auto screen:{LearnerScreen::Home,LearnerScreen::Curriculum,LearnerScreen::Journey,
                   LearnerScreen::Spelling,LearnerScreen::Flashcards}){
    nav.screen=screen;
    assert(display_refresh_policy(nav,false)==DisplayRefresh::None);
    assert(display_refresh_policy(nav,true)==DisplayRefresh::Full);
  }
  for(auto screen:{LearnerScreen::Math,LearnerScreen::Reading}){
    nav.screen=screen;
    assert(display_refresh_policy(nav,false)==DisplayRefresh::None);
    assert(display_refresh_policy(nav,true)==DisplayRefresh::Regions);
  }
  nav.help_open=true;
  assert(display_refresh_policy(nav,false)==DisplayRefresh::Full);
  assert(display_refresh_policy(nav,true)==DisplayRefresh::Full);
  DisplayDiagnostics d;
  assert(d.due(5000)&&!d.due(34999)&&d.due(35000));
  DisplayDiagnostics wrap;
  const auto start=std::numeric_limits<std::uint32_t>::max()-100;
  assert(wrap.due(start)&&!wrap.due(start+29999u)&&wrap.due(start+30000u));
  DisplayStartupRefresh r;
  r.begin(100);
  assert(!r.due(2099)&&r.due(2100)&&!r.due(2100));
  assert(r.due(6100)&&r.due(12100)&&r.due(24100)&&!r.active);
  assert(!r.due(100000));r.begin(0);r.cancel();assert(!r.due(30000));
}
