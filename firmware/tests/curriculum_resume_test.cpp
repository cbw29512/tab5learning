#include "curriculum_resume_store.h"
#include <cassert>
using namespace curriculum_preview;

int main(){
  FakeNvs::reset();
  ResumeStore s;
  Preview p{};
  assert(s.begin()==ResumeLoad::Empty&&!s.has_resume());
  assert(p.choose_subject(0)&&p.choose_lesson(0));
  assert(s.save(p)&&s.has_resume());
  assert(p.next()&&p.step==1);
  assert(s.save(p));
  Preview recovered{};
  ResumeStore reboot;
  assert(reboot.begin()==ResumeLoad::Restored&&reboot.restore(recovered));
  assert(recovered.lesson==p.lesson&&recovered.step==1);
  assert(recovered.screen==Screen::Activity&&recovered.selected==-1);
  // Duplicate save must not write; a full store still accepts exact retry.
  FakeNvs::full=true;
  assert(reboot.save(recovered));
  assert(recovered.answer(0)&&recovered.next()&&recovered.step==2);
  assert(!reboot.save(recovered)&&!reboot.available());
  FakeNvs::full=false;
  ResumeStore after_failure;
  assert(after_failure.begin()==ResumeLoad::Restored);
  assert(after_failure.restore(recovered)&&recovered.step==1);
  // Feedback never persists a selected answer.
  assert(recovered.answer(0));
  ResumeBytes b{};
  assert(resume_encode(recovered,b));
  Preview transient{};
  assert(resume_decode(b,transient)&&transient.screen==Screen::Activity&&transient.selected==-1);
  b[84]^=1;resume_put32(b,92,resume_crc(b));
  assert(!resume_decode(b,transient)); // incompatible revision
  auto& stored=FakeNvs::records["plcurriculum"]["resume1"];
  stored[0]^=1;
  ResumeStore corrupt;
  assert(corrupt.begin()==ResumeLoad::Error&&!corrupt.save(p));
  assert(stored[0]!='C'); // never erase or overwrite corruption
  FakeNvs::reset();
  ResumeStore finish;assert(finish.begin()==ResumeLoad::Empty);
  p.reset();assert(p.choose_subject(1)&&p.choose_lesson(0));
  while(p.screen!=Screen::Finished){
    if(p.screen==Screen::Activity&&p.activity().kind==Kind::Choice)assert(p.answer(0));
    else assert(p.next());
  }
  assert(finish.save(p));
  ResumeStore done;assert(done.begin()==ResumeLoad::Restored);
  assert(done.restore(recovered)&&recovered.screen==Screen::Finished);
  assert(recovered.current().subject==1);
  // A commit error is ambiguous; fail closed, require re-read on reboot.
  p.reset();assert(p.choose_subject(2)&&p.choose_lesson(0));
  FakeNvs::ambiguous=true;assert(!done.save(p)&&!done.available());
  FakeNvs::ambiguous=false;
  ResumeStore reconcile;assert(reconcile.begin()==ResumeLoad::Restored);
  assert(reconcile.restore(recovered)&&recovered.current().subject==2);
}
