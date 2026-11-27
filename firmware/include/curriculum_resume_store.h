#pragma once
#include <nvs.h>
#include <cstdio>
#include "curriculum_resume_codec.h"

namespace curriculum_preview {
enum class ResumeLoad { Empty, Restored, Error };
// Bounded one-blob resume checkpoint in a separate namespace. Existing math
// journal remains authoritative. No erase, NVS init, partition changes or
// storage of grades. NVS API errors fail closed until reboot/reconciliation.
class ResumeStore {
  bool ready=false, present=false;
  ResumeBytes last{};
  bool fail(){
    ready=false;std::printf("ERR,CURRICULUM_RESUME\n");return false;
  }
 public:
  bool available()const{return ready;}
  bool has_resume()const{return ready&&present;}
  ResumeLoad begin(){
    ready=false;present=false;
    nvs_handle_t h=0;
    const auto opened=nvs_open("plcurriculum",NVS_READONLY,&h);
    if(opened==ESP_ERR_NVS_NOT_FOUND){ready=true;return ResumeLoad::Empty;}
    if(opened!=ESP_OK){fail();return ResumeLoad::Error;}
    ResumeBytes bytes{};auto size=bytes.size();
    const auto result=nvs_get_blob(h,"resume1",bytes.data(),&size);
    nvs_close(h);
    if(result==ESP_ERR_NVS_NOT_FOUND){ready=true;return ResumeLoad::Empty;}
    Preview p{};
    if(result!=ESP_OK||size!=bytes.size()||!resume_decode(bytes,p)){
      fail();return ResumeLoad::Error;
    }
    last=bytes;present=true;ready=true;return ResumeLoad::Restored;
  }
  bool restore(Preview& out)const{return has_resume()&&resume_decode(last,out);}
  bool save(const Preview& p){
    if(!ready)return false;
    ResumeBytes bytes{};
    if(!resume_encode(p,bytes))return fail();
    if(present&&bytes==last)return true;
    nvs_handle_t h=0;
    if(nvs_open("plcurriculum",NVS_READWRITE,&h)!=ESP_OK)return fail();
    // Commit and exact readback before the learner advances.
    const auto written=nvs_set_blob(h,"resume1",bytes.data(),bytes.size());
    const auto committed=written==ESP_OK?nvs_commit(h):written;
    ResumeBytes check{};auto size=check.size();
    const auto read=committed==ESP_OK?nvs_get_blob(h,"resume1",check.data(),&size):committed;
    nvs_close(h);
    if(read!=ESP_OK||size!=check.size()||check!=bytes)return fail();
    last=bytes;present=true;return true;
  }
};
}
