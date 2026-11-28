#pragma once
#include <nvs.h>
#include <nvs_flash.h>
#include <esp_partition.h>
#include <esp_flash_encrypt.h>
#include <esp_secure_boot.h>
#include <cstdio>
#include "learner_credential_store.h"

namespace learner_auth {
// Candidate for the flash-encryption-based scheme ONLY. The current factory
// layout has neither partition. Never initialize default NVS or generate keys.
// Single owner, single maintenance task; no concurrent direct NVS access.
class SecureCredentialNvs final : public CredentialBackend {
  static constexpr const char* partition="plsecure";
  static constexpr const char* key_partition="plseckeys";
  static constexpr const char* name_space="credentials";
  static constexpr const char* record_key="recordv1";
  inline static bool claimed=false;
  bool ready=false;
  static void wipe(void* ptr,std::size_t n) {
    auto p=static_cast<volatile uint8_t*>(ptr);
    while(n--) *p++=0;
  }
  static const esp_partition_t* keys() {
    return esp_partition_find_first(ESP_PARTITION_TYPE_DATA,ESP_PARTITION_SUBTYPE_DATA_NVS_KEYS,key_partition);
  }
  static const esp_partition_t* storage() {
    return esp_partition_find_first(ESP_PARTITION_TYPE_DATA,ESP_PARTITION_SUBTYPE_DATA_NVS,partition);
  }
  static bool rejected() {
    std::printf("ERR,SECURE_CREDENTIAL_STORAGE\n"); return false;
  }
 public:
  SecureCredentialNvs()=default;
  SecureCredentialNvs(const SecureCredentialNvs&)=delete;
  SecureCredentialNvs& operator=(const SecureCredentialNvs&)=delete;
  ~SecureCredentialNvs() override {
    if(ready&&nvs_flash_deinit_partition(partition)==ESP_OK) claimed=false;
  }

  // Read-only hardware/partition check. No fuse or flash writes.
  static bool prerequisites() {
    if(!esp_secure_boot_enabled()||esp_get_flash_encryption_mode()!=ESP_FLASH_ENC_MODE_RELEASE)
      return false;
    const auto* k=keys(); const auto* s=storage();
    return k&&s&&k->encrypted&&k->size>=4096&&!s->encrypted&&s->size>=12288;
  }

  // Explicit trusted startup only. Caller must separately validate security
  // provisioning, debug/download policy and recovery. No automatic repair.
  bool begin() {
    if(ready) return true;
    if(claimed||!prerequisites()) return rejected();
    nvs_sec_cfg_t cfg{};
    const auto loaded=nvs_flash_read_security_cfg(keys(),&cfg);
    if(loaded!=ESP_OK) { wipe(&cfg,sizeof(cfg)); return rejected(); }
    const auto initialized=nvs_flash_secure_init_partition(partition,&cfg);
    wipe(&cfg,sizeof(cfg));
    if(initialized!=ESP_OK) return rejected();
    ready=true; claimed=true; return true;
  }

  CredentialState read(CredentialRecord& out) override {
    out.fill(0);
    if(!ready) return CredentialState::Unavailable;
    nvs_handle_t handle=0;
    const auto opened=nvs_open_from_partition(partition,name_space,NVS_READONLY,&handle);
    if(opened==ESP_ERR_NVS_NOT_FOUND) return CredentialState::Empty;
    if(opened!=ESP_OK) return CredentialState::Unavailable;
    auto size=out.size();
    const auto result=nvs_get_blob(handle,record_key,out.data(),&size);
    nvs_close(handle);
    if(result==ESP_OK&&size==out.size()&&valid_credential(out)) return CredentialState::Valid;
    out.fill(0);
    if(result==ESP_ERR_NVS_NOT_FOUND) return CredentialState::Empty;
    if(result==ESP_OK||result==ESP_ERR_NVS_TYPE_MISMATCH||result==ESP_ERR_NVS_INVALID_LENGTH)
      return CredentialState::Corrupt;
    return CredentialState::Unavailable;
  }

  bool write_once(const CredentialRecord& record) override {
    if(!ready||!valid_credential(record)) return rejected();
    nvs_handle_t handle=0;
    if(nvs_open_from_partition(partition,name_space,NVS_READWRITE,&handle)!=ESP_OK)
      return rejected();
    // Check again on the same handle immediately before the write.
    std::size_t n=0;
    bool ok=nvs_get_blob(handle,record_key,nullptr,&n)==ESP_ERR_NVS_NOT_FOUND;
    if(ok) ok=nvs_set_blob(handle,record_key,record.data(),record.size())==ESP_OK&&
              nvs_commit(handle)==ESP_OK;
    nvs_close(handle);
    return ok?true:rejected();
  }
};
}
