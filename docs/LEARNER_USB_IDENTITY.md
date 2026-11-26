# Learner USB identification — development protocol v1

## Objective and state
Extend passive USB discovery with a read-only application handshake. Host states: not detected -> detected generic interface -> learner protocol responding (unauthenticated), or dependency/port/timeout/protocol error. No state implies authenticated identity or provisioning.

## Wire schema
ASCII newline-framed request: HELLO,1,<nonce> (32 lowercase hex characters, generated afresh by host). Reply: LEARNER,1,<same nonce>,POCKET_LEARNER_PREVIEW. No student ID, MAC, progress, secret, or credential is transmitted. Nonce proves response correlation only, not authentication or device identity. No other commands supported. CRLF accepted, overlong/malformed/nonprintable lines discarded, bounded parser and processing per loop. Reply only for a complete valid request. Diagnostic output can coexist; host ignores unrelated lines within bounded timeout and byte/line limits.

## Boundaries
Use a separate learner firmware module, preserve display startup and math storage. No changes to power registers, partition layout, wireless, or saved learner state. Host opens only a selected COM port observed in a fresh filtered scan, with DTR/RTS disabled before open, bounded timeout, no bootloader reset request. OS/driver may still cause incidental reset; physical testing required. Explicit test administrator and CSRF authorization for browser action. No arbitrary serial commands exposed. Keep assignment PLANNED. A later authenticated protocol must establish device-specific credentials, freshness, host authorization, signed operation payloads, replay handling and durable ACK; do not reuse this unauthenticated probe as an authorization gate.

## Acceptance
Compile and test parser on host; native candidate build and app-only flash through verified repair utility; observe upright screen, touch, reconnect and handshake. Reject wrong nonce/version, overlong lines and unsupported commands. No student data and no writes. The previously visible e013e7f firmware remains rollback baseline.
