#pragma once
#include <cstdint>
#include <cstring>
#include <map>
#include <string>
#include <vector>

// Host-only simulation of the Arduino Preferences calls used by MathJournal.
class Preferences {
 public:
  inline static std::map<std::string,std::vector<std::uint8_t>> records;
  inline static bool fail_open=false, fail_write=false, fail_read=false, ambiguous=false;
  static void reset() {
    records.clear(); fail_open=fail_write=fail_read=ambiguous=false;
  }
  bool begin(const char*,bool) { return !fail_open; }
  bool isKey(const char* key) { return records.count(key)!=0; }
  std::size_t getBytesLength(const char* key) {
    auto it=records.find(key);
    return it==records.end()?0:it->second.size();
  }
  std::size_t getBytes(const char* key,void* dst,std::size_t n) {
    auto it=records.find(key);
    if(fail_read||it==records.end()||it->second.size()!=n) return 0;
    std::memcpy(dst,it->second.data(),n);
    return n;
  }
  std::size_t putBytes(const char* key,const void* src,std::size_t n) {
    if(fail_write) return 0;
    auto p=static_cast<const std::uint8_t*>(src);
    records[key]={p,p+n};
    return ambiguous?0:n;
  }
};
