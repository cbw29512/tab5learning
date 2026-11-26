#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>
using esp_err_t=int;
using nvs_handle_t=unsigned;
inline constexpr int ESP_OK=0, ESP_FAIL=-1, ESP_ERR_NVS_NOT_FOUND=1,
  ESP_ERR_NVS_TYPE_MISMATCH=2, ESP_ERR_NVS_INVALID_LENGTH=3;
inline constexpr int NVS_READONLY=0,NVS_READWRITE=1;
inline constexpr int ESP_PARTITION_TYPE_DATA=1,ESP_PARTITION_SUBTYPE_DATA_NVS=2,
  ESP_PARTITION_SUBTYPE_DATA_NVS_KEYS=4,ESP_FLASH_ENC_MODE_RELEASE=2;
struct esp_partition_t { bool encrypted; std::size_t size; };
struct nvs_sec_cfg_t { std::uint8_t bytes[64]; };
namespace SecureFake {
inline esp_partition_t keys{true,4096},storage{false,12288};
inline bool boot=false,release=false,has_keys=false,has_storage=false,key_read=false,
  init_ok=true,ready=false,namespace_exists=false,fail_open=false,fail_read=false,
  wrong_type=false,fail_write=false,ambiguous=false;
inline unsigned init_calls=0,key_reads=0,writes=0;
inline std::vector<std::uint8_t> record;
inline void reset() {
  keys={true,4096}; storage={false,12288};
  boot=release=has_keys=has_storage=key_read=ready=namespace_exists=fail_open=
    fail_read=wrong_type=fail_write=ambiguous=false;
  init_ok=true; init_calls=key_reads=writes=0; record.clear();
}
}
inline bool esp_secure_boot_enabled() { return SecureFake::boot; }
inline int esp_get_flash_encryption_mode() { return SecureFake::release?2:0; }
inline const esp_partition_t* esp_partition_find_first(int type,int subtype,const char* label) {
  using namespace SecureFake;
  if(type!=ESP_PARTITION_TYPE_DATA) return nullptr;
  if(subtype==ESP_PARTITION_SUBTYPE_DATA_NVS_KEYS&&std::strcmp(label,"plseckeys")==0)
    return has_keys?&keys:nullptr;
  if(subtype==ESP_PARTITION_SUBTYPE_DATA_NVS&&std::strcmp(label,"plsecure")==0)
    return has_storage?&storage:nullptr;
  return nullptr;
}
inline esp_err_t nvs_flash_read_security_cfg(const esp_partition_t*,nvs_sec_cfg_t* cfg) {
  ++SecureFake::key_reads; std::memset(cfg,7,sizeof(*cfg));
  return SecureFake::key_read?ESP_OK:ESP_FAIL;
}
inline esp_err_t nvs_flash_secure_init_partition(const char* p,nvs_sec_cfg_t* cfg) {
  ++SecureFake::init_calls;
  if(std::strcmp(p,"plsecure")||!cfg||!SecureFake::init_ok) return ESP_FAIL;
  SecureFake::ready=true; return ESP_OK;
}
inline esp_err_t nvs_flash_deinit_partition(const char* p) {
  if(std::strcmp(p,"plsecure")) return ESP_FAIL;
  SecureFake::ready=false; return ESP_OK;
}
inline esp_err_t nvs_open_from_partition(const char* p,const char* ns,int mode,nvs_handle_t* h) {
  using namespace SecureFake;
  if(std::strcmp(p,"plsecure")||std::strcmp(ns,"credentials")||!ready||fail_open)
    return ESP_FAIL;
  if(mode==NVS_READONLY&&!namespace_exists) return ESP_ERR_NVS_NOT_FOUND;
  if(mode==NVS_READWRITE) namespace_exists=true;
  *h=1; return ESP_OK;
}
inline void nvs_close(nvs_handle_t) {}
inline esp_err_t nvs_get_blob(nvs_handle_t,const char*,void* dst,std::size_t* n) {
  using namespace SecureFake;
  if(fail_read) return ESP_FAIL;
  if(wrong_type) return ESP_ERR_NVS_TYPE_MISMATCH;
  if(record.empty()) return ESP_ERR_NVS_NOT_FOUND;
  if(!dst) { *n=record.size(); return ESP_OK; }
  if(*n<record.size()) return ESP_ERR_NVS_INVALID_LENGTH;
  *n=record.size(); std::memcpy(dst,record.data(),*n); return ESP_OK;
}
inline esp_err_t nvs_set_blob(nvs_handle_t,const char*,const void* src,std::size_t n) {
  using namespace SecureFake;
  ++writes;
  if(fail_write) return ESP_FAIL;
  auto p=static_cast<const std::uint8_t*>(src); record.assign(p,p+n); return ESP_OK;
}
inline esp_err_t nvs_commit(nvs_handle_t) { return SecureFake::ambiguous?ESP_FAIL:ESP_OK; }
