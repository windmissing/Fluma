#pragma once
#include <cstdint>
#include <cstring>

namespace grs::pc {
struct M1FenceIdentity {
  uintptr_t device=0,queue=0,fence=0;
  uint64_t geometry=0,nonce=0;
  constexpr bool Matches(M1FenceIdentity other) const {
    return device && queue && fence && geometry && nonce && device==other.device &&
        queue==other.queue && fence==other.fence && geometry==other.geometry && nonce==other.nonce;
  }
};

inline bool RecoverableM1ClockFault(const char *reason) {
  return reason && (!std::strcmp(reason,"CLOCK_GAP") || !std::strcmp(reason,"PULSE_QUEUE_OVERFLOW"));
}
inline bool RecoverableM1Deadline(const char *reason) {
  return reason && (!std::strcmp(reason,"ROOT_DEADLINE") || !std::strcmp(reason,"LATE_FENCE_SIGNAL") ||
                    !std::strcmp(reason,"ACCEPT_DEADLINE"));
}
inline bool M1SourceHoldRevoked(const char *reason) {
  return reason && !std::strcmp(reason,"SOURCE_HOLD_REVOKED");
}
inline bool CanRearmM1(const char *reason,bool ordinary,bool current,bool idle,bool inFlight,bool pinned,
                       bool clockOpen,bool retirementProven,bool retiredPending=false) {
  return ordinary && current && idle && !inFlight && !pinned &&
      ((!clockOpen && RecoverableM1ClockFault(reason)) ||
       (retirementProven && (RecoverableM1Deadline(reason) || (retiredPending && M1SourceHoldRevoked(reason)))));
}
}
