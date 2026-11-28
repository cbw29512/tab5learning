#include "learner_credential_store.h"
#include <cassert>
#include <string>
using namespace learner_auth;

struct FakeStore : CredentialBackend {
  CredentialState state=CredentialState::Empty;
  CredentialRecord record{};
  unsigned reads=0,writes=0;
  bool fail=false,ambiguous=false,lose_readback=false;
  CredentialState read(CredentialRecord& out) override {
    ++reads;out=record;
    return lose_readback&&writes?CredentialState::Unavailable:state;
  }
  bool write_once(const CredentialRecord& in) override {
    ++writes;
    if(fail||state!=CredentialState::Empty) return false;
    record=in;state=CredentialState::Valid;
    return !ambiguous;
  }
};
int main() {
  Bytes key{},decoded{};
  for(unsigned i=0;i<32;++i) key[i]=i;
  CredentialRecord record{},other{};
  char id[65]{};
  assert(credential_encode("tab5-001",key,record));
  assert(credential_crc(record)==0xd50ea1c9u);
  assert(credential_decode(record,id,decoded)&&std::string(id)=="tab5-001"&&decoded==key);
  assert(credential_encode("tab5-002",key,other));
  for(unsigned i=0;i<record.size();++i) {
    auto corrupt=record;corrupt[i]^=1;
    assert(!credential_decode(corrupt,id,decoded)&&id[0]==0&&decoded==Bytes{});
  }
  FakeStore store;
  assert(install_credential(store,record,false)==InstallResult::Unauthorized);
  assert(store.reads==0&&store.writes==0);
  assert(install_credential(store,CredentialRecord{},true)==InstallResult::InvalidCandidate);
  assert(install_credential(store,record,true)==InstallResult::Installed);
  assert(store.writes==1);
  assert(install_credential(store,record,true)==InstallResult::AlreadyPresent);
  assert(install_credential(store,other,true)==InstallResult::Conflict);
  assert(store.writes==1);
  store.record[4]^=1;
  assert(install_credential(store,record,true)==InstallResult::Corrupt);
  assert(store.writes==1);
  FakeStore unavailable;unavailable.state=CredentialState::Unavailable;
  assert(install_credential(unavailable,record,true)==InstallResult::Unavailable);
  assert(unavailable.writes==0);
  FakeStore uncertain;uncertain.ambiguous=true;
  assert(install_credential(uncertain,record,true)==InstallResult::PresentAfterAmbiguousWrite);
  assert(install_credential(uncertain,record,true)==InstallResult::AlreadyPresent);
  assert(uncertain.writes==1);
  FakeStore interrupted;interrupted.lose_readback=true;
  assert(install_credential(interrupted,record,true)==InstallResult::RecoveryRequired);
  assert(interrupted.writes==1);
  interrupted.lose_readback=false;
  assert(install_credential(interrupted,record,true)==InstallResult::AlreadyPresent);
  assert(interrupted.writes==1);
  FakeStore failed;failed.fail=true;
  assert(install_credential(failed,record,true)==InstallResult::RecoveryRequired);
  assert(failed.writes==1);
  assert(!credential_encode("BAD ID",key,other));
  assert(!credential_encode("tab5-001",Bytes{},other));
}
