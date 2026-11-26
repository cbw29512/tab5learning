#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <map>
#include <string>
#include <vector>
using esp_err_t=int;
using nvs_handle_t=unsigned;
inline constexpr int ESP_OK=0, ESP_FAIL=-1, ESP_ERR_NVS_NOT_FOUND=1,
  ESP_ERR_NVS_NOT_ENOUGH_SPACE=2, ESP_ERR_NVS_INVALID_LENGTH=3;
inline constexpr int NVS_READONLY=0,NVS_READWRITE=1,NVS_TYPE_ANY=255,NVS_TYPE_BLOB=66;
struct nvs_entry_info_t { char key[16]{}; int type=NVS_TYPE_BLOB; };
struct FakeIterator { std::vector<std::string> keys; std::size_t index=0; };
using nvs_iterator_t=FakeIterator*;
struct nvs_stats_t { std::size_t total_entries=0; };
namespace FakeNvs {
using Bytes=std::vector<std::uint8_t>;
inline std::map<std::string,std::map<std::string,Bytes>> records;
inline int writes_before_full=-1;
inline bool dedicated_initialized=false,fail_inventory=false;
inline std::size_t dedicated_entries=64512;
inline bool fail_open=false,fail_read=false,fail_write=false,ambiguous=false,full=false;
inline void reset() {
  records.clear(); writes_before_full=-1; dedicated_initialized=fail_inventory=false; dedicated_entries=64512; fail_open=fail_read=fail_write=ambiguous=full=false;
}
inline const char* space(nvs_handle_t h) { return h==3?"dedicated/pljournal":h==1?"pljournal":"pocketlearn"; }
}
inline esp_err_t nvs_open(const char* ns,int mode,nvs_handle_t* h) {
  if(FakeNvs::fail_open) return ESP_FAIL;
  if(mode==NVS_READONLY&&!FakeNvs::records.count(ns)) return ESP_ERR_NVS_NOT_FOUND;
  *h=std::strcmp(ns,"pljournal")==0?1:2; return ESP_OK;
}
inline void nvs_close(nvs_handle_t) {}
inline esp_err_t nvs_open_from_partition(const char* part,const char* ns,int mode,nvs_handle_t* h) {
  if(FakeNvs::fail_open||!FakeNvs::dedicated_initialized||std::strcmp(part,"pllearning")||
     std::strcmp(ns,"pljournal")||mode!=NVS_READWRITE) return ESP_FAIL;
  *h=3; return ESP_OK;
}
inline esp_err_t nvs_get_stats(const char* part,nvs_stats_t* stats) {
  if(part&&(!FakeNvs::dedicated_initialized||std::strcmp(part,"pllearning"))) return ESP_FAIL;
  stats->total_entries=part?FakeNvs::dedicated_entries:756; return ESP_OK;
}
inline esp_err_t nvs_entry_find(const char* part,const char* ns,int,nvs_iterator_t* it) {
  *it=nullptr;
  if(FakeNvs::fail_inventory||!FakeNvs::dedicated_initialized||std::strcmp(part,"pllearning")||
     std::strcmp(ns,"pljournal")) return ESP_FAIL;
  auto& records=FakeNvs::records["dedicated/pljournal"];
  if(records.empty()) return ESP_ERR_NVS_NOT_FOUND;
  *it=new FakeIterator{};
  for(const auto& record:records) (*it)->keys.push_back(record.first);
  return ESP_OK;
}
inline esp_err_t nvs_entry_info(nvs_iterator_t it,nvs_entry_info_t* info) {
  if(!it||it->index>=it->keys.size()||it->keys[it->index].size()>=sizeof(info->key)) return ESP_FAIL;
  std::strcpy(info->key,it->keys[it->index].c_str()); info->type=NVS_TYPE_BLOB; return ESP_OK;
}
inline void nvs_release_iterator(nvs_iterator_t it) { delete it; }
inline esp_err_t nvs_entry_next(nvs_iterator_t* it) {
  if(++(*it)->index<(*it)->keys.size()) return ESP_OK;
  nvs_release_iterator(*it); *it=nullptr; return ESP_ERR_NVS_NOT_FOUND;
}
inline esp_err_t nvs_get_blob(nvs_handle_t h,const char* key,void* dst,std::size_t* n) {
  if(FakeNvs::fail_read) return ESP_FAIL;
  auto& ns=FakeNvs::records[FakeNvs::space(h)]; auto it=ns.find(key);
  if(it==ns.end()) return ESP_ERR_NVS_NOT_FOUND;
  if(!dst) { *n=it->second.size(); return ESP_OK; }
  if(*n<it->second.size()) return ESP_ERR_NVS_INVALID_LENGTH;
  *n=it->second.size(); std::memcpy(dst,it->second.data(),*n); return ESP_OK;
}
inline esp_err_t nvs_set_blob(nvs_handle_t h,const char* key,const void* src,std::size_t n) {
  if(FakeNvs::full||FakeNvs::writes_before_full==0) return ESP_ERR_NVS_NOT_ENOUGH_SPACE;
  if(FakeNvs::writes_before_full>0) --FakeNvs::writes_before_full;
  if(FakeNvs::fail_write) return ESP_FAIL;
  auto p=static_cast<const std::uint8_t*>(src);
  FakeNvs::records[FakeNvs::space(h)][key]={p,p+n}; return ESP_OK;
}
inline esp_err_t nvs_commit(nvs_handle_t) { return FakeNvs::ambiguous?ESP_FAIL:ESP_OK; }
