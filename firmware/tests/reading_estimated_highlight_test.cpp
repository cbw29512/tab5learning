#include "reading_estimated_highlight.h"
#include <cassert>

int main() {
  static const ReadingCue cues[]={{"A",0,100},{"fox",200,400}};
  ReadingEstimatedHighlight highlight;
  assert(highlight.update(0,true)==-1);
  assert(highlight.start(cues,2,500,1000));
  assert(highlight.update(1000,true)==0);
  assert(highlight.update(1099,true)==0);
  assert(highlight.update(1100,true)==-1);
  assert(highlight.update(1200,true)==1);
  assert(highlight.update(1400,true)==-1);
  assert(highlight.update(1500,true)==-1);
  assert(highlight.start(cues,2,500,2000));
  assert(highlight.update(2200,false)==-1);
  assert(highlight.update(2201,true)==-1);
  assert(highlight.start(cues,2,500,UINT32_MAX-49));
  assert(highlight.update(0,true)==0);
  assert(highlight.update(150,true)==1);
  highlight.stop();
  assert(highlight.update(151,true)==-1);
  assert(!highlight.start(cues,2,20001,0));
  assert(!highlight.start(nullptr,2,500,0));
  assert(highlight.start(cues,2,500,1000));
  assert(highlight.update(1200,true)==1);
  assert(highlight.update(1199,true)==-1);
  // Resuming at a word boundary must not replay the first highlight.
  assert(highlight.start(cues,2,500,3000,200));
  assert(highlight.update(3000,true)==1);
  assert(highlight.update(3199,true)==1);
  assert(highlight.update(3200,true)==-1);
  assert(highlight.update(3300,true)==-1);
  assert(!highlight.start(cues,2,500,0,500));
  assert(!highlight.start(cues,2,500,0,501));
  assert(highlight.start(cues,2,500,UINT32_MAX-49,200));
  assert(highlight.update(0,true)==1);
  assert(highlight.update(150,true)==-1);
  return 0;
}
