#include "learner_storage_format.h"
#include <cassert>
#include <cstring>
#include <limits>
using namespace learner_storage;

int main(){
  Status initial{};assert(!initial.known()&&initial.state==State::NotChecked);
  assert(!measured(0,0).known()&&!measured(100,101).known());
  assert(!optional_content_fits(initial,1));
  auto large=measured(511741755392ULL,215613440ULL);
  assert(large.known()&&large.state==State::Normal);
  assert(large.free==511526141952ULL&&large.free_percent==99);
  char line[128];format_summary(large,line,sizeof(line));
  assert(std::strcmp(line,"511.52 GB free of 511.74 GB (99%)")==0);
  auto small=measured(16000000000ULL,15000000000ULL);
  assert(small.state==State::Low&&small.free_percent==6);
  assert(measured(64000000000ULL,57600000000ULL).state==State::Low);
  assert(measured(64000000000ULL,57599999999ULL).state==State::Normal);
  assert(measured(64000000000ULL,60800000000ULL).state==State::Critical);
  assert(measured(2000000000ULL,1744000000ULL).state==State::Critical);
  assert(measured(1000000000ULL,1000000000ULL).state==State::Critical);
  assert(measured(1000000000ULL,1000000000ULL).free_percent==0);
  assert(measured(64000000000ULL,0).free_percent==100);
  assert(free_percent(1,3)==33&&free_percent(2,3)==66);
  auto huge=measured(std::numeric_limits<std::uint64_t>::max(),0);
  assert(huge.free_percent==100&&huge.state==State::Normal);
  assert(optional_content_fits(small,744000000ULL));
  assert(!optional_content_fits(small,744000001ULL));
  assert(!optional_content_fits(small,0));
  assert(!optional_content_fits(small,std::numeric_limits<std::uint64_t>::max()));
  assert(!optional_content_fits(measured(64000000000ULL,60800000000ULL),1));
  format_summary(unavailable(),line,sizeof(line));
  assert(std::strcmp(line,"Storage unavailable")==0);
}
