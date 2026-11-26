#pragma once
#include <nvs.h>
#include <cstdio>
#include <cstring>

// Explicit target; never auto-select or fall back. The dedicated target is
// an internal staging candidate, not a learner boot/cutover policy.
enum class MathJournalTarget { Factory, DedicatedStaging };
inline constexpr unsigned math_journal_max_records=4096; // PLMB V1 bound
inline constexpr const char* math_learning_partition="pllearning";

inline esp_err_t math_journal_open(MathJournalTarget target,nvs_handle_t& handle) {
  if(target==MathJournalTarget::Factory)
    return nvs_open("pljournal",NVS_READWRITE,&handle);
  // Caller must have initialized the verified partition separately.
  return nvs_open_from_partition(math_learning_partition,"pljournal",NVS_READWRITE,&handle);
}
inline bool math_journal_limit(MathJournalTarget target,unsigned& limit) {
  nvs_stats_t stats{};
  const char* part=target==MathJournalTarget::Factory?nullptr:math_learning_partition;
  if(nvs_get_stats(part,&stats)!=ESP_OK||!stats.total_entries||stats.total_entries>1000000)
    return false;
  limit=stats.total_entries>math_journal_max_records?math_journal_max_records:
    static_cast<unsigned>(stats.total_entries);
  return true;
}
inline void math_journal_key(unsigned slot,char (&key)[12]) {
  std::snprintf(key,sizeof(key),"a%03u",slot);
}
// Inspect actual keys in the staging namespace. Do not silently omit an
// out-of-range record, noncanonical key, unsupported schema or wrong type.
inline bool math_journal_staging_keys(unsigned limit) {
  nvs_iterator_t it=nullptr;
  auto result=nvs_entry_find(math_learning_partition,"pljournal",NVS_TYPE_ANY,&it);
  bool valid=true;
  while(result==ESP_OK) {
    nvs_entry_info_t info{};
    if(nvs_entry_info(it,&info)!=ESP_OK||info.type!=NVS_TYPE_BLOB) { valid=false; break; }
    if(std::strcmp(info.key,"basev1")!=0) {
      unsigned slot=0;
      const char* p=info.key;
      if(*p++!='a'||!*p) { valid=false; break; }
      for(;*p;++p) {
        if(*p<'0'||*p>'9'||slot>limit) { valid=false; break; }
        slot=slot*10+unsigned(*p-'0');
      }
      char canonical[12]; math_journal_key(slot,canonical);
      if(!valid||!slot||slot>limit||std::strcmp(canonical,info.key)!=0) { valid=false; break; }
    }
    result=nvs_entry_next(&it);
  }
  nvs_release_iterator(it);
  return valid&&result==ESP_ERR_NVS_NOT_FOUND;
}
