#pragma once
#include "learner_credential_record.h"

namespace learner_auth {
enum class CredentialState { Empty, Valid, Corrupt, Unavailable };
enum class InstallResult {
  Installed, AlreadyPresent, PresentAfterAmbiguousWrite, RecoveryRequired,
  Unauthorized, InvalidCandidate, Conflict, Corrupt, Unavailable
};

// Trusted maintenance abstraction only. The real backend must distinguish
// an absent record from an I/O error and reject wrong-sized records.
class CredentialBackend {
 public:
  virtual ~CredentialBackend()=default;
  virtual CredentialState read(CredentialRecord& out)=0;
  virtual bool write_once(const CredentialRecord& record)=0;
};

inline CredentialState inspect(CredentialBackend& storage,CredentialRecord& out) {
  out.fill(0);
  auto state=storage.read(out);
  if(state==CredentialState::Valid && !valid_credential(out))
    state=CredentialState::Corrupt;
  if(state!=CredentialState::Valid) out.fill(0);
  return state;
}

inline InstallResult install_credential(CredentialBackend& storage,
                                        const CredentialRecord& candidate,
                                        bool maintenance_authorized) {
  // This flag is a caller precondition, not an authentication mechanism.
  // No untrusted USB or web route may invoke this API.
  if(!maintenance_authorized) return InstallResult::Unauthorized;
  if(!valid_credential(candidate)) return InstallResult::InvalidCandidate;
  CredentialRecord existing{};
  switch(inspect(storage,existing)) {
    case CredentialState::Valid:
      return existing==candidate?InstallResult::AlreadyPresent:InstallResult::Conflict;
    case CredentialState::Corrupt: return InstallResult::Corrupt;
    case CredentialState::Unavailable: return InstallResult::Unavailable;
    case CredentialState::Empty: break;
    default: return InstallResult::Unavailable;
  }
  // Only one write call. A failed response may mean a committed write.
  const bool reported=storage.write_once(candidate);
  if(inspect(storage,existing)!=CredentialState::Valid||existing!=candidate)
    return InstallResult::RecoveryRequired;
  return reported?InstallResult::Installed:InstallResult::PresentAfterAmbiguousWrite;
}
}
