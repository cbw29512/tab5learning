#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

// Portable reference. No serial transport, key injection or authorization.
namespace learner_auth {
using Bytes = std::array<uint8_t,32>;
using Hmac = bool (*)(const Bytes&, const uint8_t*, std::size_t, Bytes&);
enum class Role { Device, Host, Ack };

inline bool valid_id(const char* id) {
  if(!id) return false;
  for(std::size_t i=0;i<=64;++i) {
    const char c=id[i];
    if(c=='\0') return i>0;
    if(i==64 || !((c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='_'||c=='-')) return false;
  }
  return false;
}

inline bool make_proof(Hmac hmac,const Bytes& key,Role role,const char* id,
                       const Bytes& host,const Bytes& device,Bytes& out) {
  out.fill(0);
  if(!hmac||!valid_id(id)||(role!=Role::Device && role!=Role::Host && role!=Role::Ack)) return false;
  constexpr char domain[]="TAB5-AUTH-V1";
  const char* label=role==Role::Device?"device":(role==Role::Host?"host":"ack");
  std::array<uint8_t,sizeof(domain)+7+65+64> transcript{};
  std::size_t used=0;
  auto append=[&](const void* p,std::size_t n) {
    std::memcpy(transcript.data()+used,p,n); used+=n;
  };
  append(domain,sizeof(domain));
  append(label,std::strlen(label)+1);
  append(id,std::strlen(id)+1);
  append(host.data(),host.size());
  append(device.data(),device.size());
  if(!hmac(key,transcript.data(),used,out)) { out.fill(0); return false; }
  return true;
}

inline bool equal_proof(const Bytes& a,const Bytes& b) {
  // Accumulate every byte, never early-return on a mismatch.
  volatile uint8_t diff=0;
  for(std::size_t i=0;i<a.size();++i) diff=static_cast<uint8_t>(diff|(a[i]^b[i]));
  return diff==0;
}

class DeviceChallenge {
 public:
  DeviceChallenge() = default;
  DeviceChallenge(const DeviceChallenge&) = delete;
  DeviceChallenge& operator=(const DeviceChallenge&) = delete;
  // Poll even without incoming traffic so an expired proof cannot survive a timer wrap.
  void expire(uint32_t now) {
    if(pending_ && uint32_t(now-started_)>10000) cancel();
  }
  bool begin(Hmac hmac,const Bytes& key,const char* id,const Bytes& host,
             const Bytes& device,uint32_t now,Bytes& device_proof) {
    cancel();
    if(!make_proof(hmac,key,Role::Device,id,host,device,device_proof)) return false;
    // Compute the expected host proof once; do not retain the secret.
    if(!make_proof(hmac,key,Role::Host,id,host,device,expected_)) {
      device_proof.fill(0); cancel(); return false;
    }
    started_=now;
    pending_=true;
    return true;
  }

  bool complete(const Bytes& host_proof,uint32_t now) {
    const bool ok=pending_ && uint32_t(now-started_)<=10000 &&
                  equal_proof(expected_,host_proof);
    cancel();  // A failed attempt also consumes the challenge.
    return ok;
  }

  void cancel() {
    pending_=false;
    expected_.fill(0);
    started_=0;
  }

  bool pending() const { return pending_; }

 private:
  Bytes expected_{};
  uint32_t started_=0;
  bool pending_=false;
};
}  // namespace learner_auth
