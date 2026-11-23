#pragma once
#include <cstring>

namespace grs::pc {
inline bool RecoverableM1ClockFault(const char *reason) {
  return reason && (!std::strcmp(reason,"CLOCK_GAP") || !std::strcmp(reason,"PULSE_QUEUE_OVERFLOW"));
}
inline bool RecoverableM1Deadline(const char *reason) {
  return reason && (!std::strcmp(reason,"ROOT_DEADLINE") || !std::strcmp(reason,"LATE_FENCE_SIGNAL") ||
                    !std::strcmp(reason,"ACCEPT_DEADLINE"));
}
inline bool CanRearmM1(const char *reason,bool ordinary,bool current,bool idle,bool inFlight,bool pinned,
                       bool clockOpen,bool retirementProven) {
  return ordinary && current && idle && !inFlight && !pinned &&
      ((!clockOpen && RecoverableM1ClockFault(reason)) || (retirementProven && RecoverableM1Deadline(reason)));
}
}
