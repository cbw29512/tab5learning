#pragma once
#include "math_snapshot.h"

// Internal staging receipt: version, physical count, exact checkpoints,
// rolling record CRC and receipt CRC. Corruption detection, NOT authentication.
using MathCopyReceipt=std::array<std::uint8_t,48>;
inline std::uint32_t math_copy_crc_update(std::uint32_t crc,const std::uint8_t* p,std::size_t n) {
  for(std::size_t i=0;i<n;++i) {
    crc^=p[i];
    for(unsigned bit=0;bit<8;++bit) crc=(crc>>1)^((crc&1u)?0xedb88320u:0u);
  }
  return crc;
}
class MathCopyDigest {
  std::uint32_t crc=0xffffffffu;
 public:
  void add(const AttemptBytes& bytes,std::size_t n) {
    const std::uint8_t length[]={std::uint8_t(n),std::uint8_t(n>>8)};
    crc=math_copy_crc_update(crc,length,2);
    // Exclude each record's own CRC to avoid the fixed CRC residue.
    crc=math_copy_crc_update(crc,bytes.data(),n-4);
  }
  std::uint32_t value() const { return ~crc; }
};
inline MathCopyReceipt math_copy_receipt(const MathSnapshot& m,std::uint32_t digest) {
  MathCopyReceipt bytes{};
  bytes[0]='P'; bytes[1]='L'; bytes[2]='C'; bytes[3]=1;
  for(unsigned i=0;i<4;++i) {
    bytes[4+i]=std::uint8_t(m.count>>(8*i));
    bytes[40+i]=std::uint8_t(digest>>(8*i));
  }
  for(unsigned i=0;i<16;++i) { bytes[8+i]=m.base[i]; bytes[24+i]=m.final[i]; }
  const auto crc=~math_copy_crc_update(0xffffffffu,bytes.data(),44);
  for(unsigned i=0;i<4;++i) bytes[44+i]=std::uint8_t(crc>>(8*i));
  return bytes;
}
