#pragma once
#include <cstdio>
#include <cstddef>
#include "learner_storage_status.h"

namespace learner_storage {
// Pure presentation for the probe, future learner Help and teacher tooling.
// Integer arithmetic avoids floating-point printf support differences.
inline void format_capacity(std::uint64_t bytes,char* out,std::size_t n){
  if(bytes>=1000000000ULL){
    std::snprintf(out,n,"%llu.%02llu GB",
      static_cast<unsigned long long>(bytes/1000000000ULL),
      static_cast<unsigned long long>((bytes%1000000000ULL)/10000000ULL));
  }else if(bytes>=1000000ULL){
    std::snprintf(out,n,"%llu.%02llu MB",
      static_cast<unsigned long long>(bytes/1000000ULL),
      static_cast<unsigned long long>((bytes%1000000ULL)/10000ULL));
  }else{
    std::snprintf(out,n,"%llu bytes",static_cast<unsigned long long>(bytes));
  }
}
inline void format_summary(const Status& s,char* out,std::size_t n){
  if(!s.known()){std::snprintf(out,n,"%s",state_label(s.state));return;}
  char free[48],total[48];
  format_capacity(s.free,free,sizeof(free));format_capacity(s.total,total,sizeof(total));
  std::snprintf(out,n,"%s free of %s (%u%%)",free,total,s.free_percent);
}
}
