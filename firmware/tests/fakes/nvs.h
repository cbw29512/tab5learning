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
inline constexpr int NVS_READONLY=0,NVS_READWRITE=1;
struct nvs_stats_t { std::size_t total_entries=0; };
namespace FakeNvs {
using Bytes=std::vector<std::uint8_t>;
inline std::map<std::string,std::map<std::string,Bytes>> records;
inline int writes_before_full=-1;
inline bool fail_open=false,fail_read=false,fail_write=false,ambiguous=false,full=false;
inline void reset() {
  records.clear(); writes_before_full=-1; fail_open=fail_read=fail_write=ambiguous=full=false;
}
inline const char* space(nvs_handle_t h) { return h==1?"pljournal":"pocketlearn"; }
}
inline esp_err_t nvs_open(const char* ns,int mode,nvs_handle_t* h) {
  if(FakeNvs::fail_open) return ESP_FAIL;
  if(mode==NVS_READONLY&&!FakeNvs::records.count(ns)) return ESP_ERR_NVS_NOT_FOUND;
  *h=std::strcmp(ns,"pljournal")==0?1:2; return ESP_OK;
}
inline void nvs_close(nvs_handle_t) {}
inline esp_err_t nvs_get_stats(const char*,nvs_stats_t* stats) { stats->total_entries=756; return ESP_OK; }
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
