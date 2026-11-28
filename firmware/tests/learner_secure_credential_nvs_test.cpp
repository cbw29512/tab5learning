#include "learner_secure_credential_nvs.h"
#include <cassert>
using namespace learner_auth;
int main() {
  using namespace SecureFake;
  reset();
  Bytes key{}; key[0]=1; CredentialRecord candidate{},other{},out{};
  assert(credential_encode("tab5-001",key,candidate));
  assert(credential_encode("tab5-002",key,other));
  {
    SecureCredentialNvs backend;
    assert(!backend.begin()&&init_calls==0&&key_reads==0);
    assert(backend.read(out)==CredentialState::Unavailable);
    boot=release=has_keys=has_storage=true;
    keys.encrypted=false; assert(!backend.begin()&&key_reads==0);
    keys.encrypted=true; storage.encrypted=true; assert(!backend.begin()&&key_reads==0);
    storage.encrypted=false; assert(!backend.begin()&&key_reads==1&&init_calls==0);
    key_read=true; init_ok=false; assert(!backend.begin()&&init_calls==1);
    init_ok=true; assert(backend.begin()&&init_calls==2);
    SecureCredentialNvs duplicate; assert(!duplicate.begin());
    assert(backend.read(out)==CredentialState::Empty);
    assert(install_credential(backend,candidate,false)==InstallResult::Unauthorized&&writes==0);
    assert(install_credential(backend,candidate,true)==InstallResult::Installed&&writes==1);
    assert(install_credential(backend,candidate,true)==InstallResult::AlreadyPresent&&writes==1);
    assert(install_credential(backend,other,true)==InstallResult::Conflict&&writes==1);
    assert(!backend.write_once(other)&&writes==1);
    fail_read=true; assert(backend.read(out)==CredentialState::Unavailable&&out==CredentialRecord{});
    fail_read=false; wrong_type=true; assert(backend.read(out)==CredentialState::Corrupt);
    wrong_type=false; assert(backend.read(out)==CredentialState::Valid&&out==candidate);
  }
  {
    SecureCredentialNvs recovered; assert(recovered.begin());
    assert(recovered.read(out)==CredentialState::Valid&&out==candidate);
    record[0]^=1; assert(recovered.read(out)==CredentialState::Corrupt);
    assert(install_credential(recovered,candidate,true)==InstallResult::Corrupt&&writes==1);
    record.clear(); ambiguous=true;
    assert(install_credential(recovered,candidate,true)==InstallResult::PresentAfterAmbiguousWrite);
    assert(writes==2&&recovered.read(out)==CredentialState::Valid&&out==candidate);
  }
}
