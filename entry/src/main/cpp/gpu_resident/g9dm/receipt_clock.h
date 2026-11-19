#pragma once
#include <cerrno>
#include <cfenv>
#include <cstdint>
#include <time.h>
namespace grs::g9dm {
struct CheckedTime {
  bool ok = false;
  uint64_t ns = 0;
  int rawErrno = 0;
};
inline thread_local int firstClockApiError = 0;
inline thread_local uint32_t firstClockApiStage = 0;
inline CheckedTime ClockFailure(uint64_t ns, int error, uint32_t stage) {
  if (!firstClockApiStage) {
    firstClockApiStage = stage;
    firstClockApiError = error;
  }
  return {false, ns, error};
}
#ifdef GRS_G9RX_TEST_HOOKS
using HarnessClockReader = CheckedTime (*)();
inline HarnessClockReader harnessClockReader = nullptr,
                          harnessResolutionReader = nullptr;
#endif
inline CheckedTime CheckedClockRead() {
#ifdef GRS_G9RX_TEST_HOOKS
  if (harnessClockReader) {
    auto value = harnessClockReader();
    return value.ok ? value : ClockFailure(value.ns, value.rawErrno, 1);
  }
#endif
  timespec t{};
  if (clock_gettime(CLOCK_MONOTONIC, &t))
    return ClockFailure(0, errno, 1);
  if (t.tv_sec < 0 || t.tv_nsec < 0 || t.tv_nsec >= 1000000000 ||
      uint64_t(t.tv_sec) > (UINT64_MAX - uint64_t(t.tv_nsec)) / 1000000000)
    return ClockFailure(0, ERANGE, 1);
  uint64_t ns = uint64_t(t.tv_sec) * 1000000000 + uint64_t(t.tv_nsec);
  static thread_local bool seen = false;
  static thread_local uint64_t last = 0;
  if (seen && ns < last)
    return ClockFailure(ns, ERANGE, 1);
  seen = true;
  last = ns;
  return {true, ns, 0};
}
inline CheckedTime ClockResolution() {
#ifdef GRS_G9RX_TEST_HOOKS
  if (harnessResolutionReader) {
    auto value = harnessResolutionReader();
    return value.ok ? value : ClockFailure(value.ns, value.rawErrno, 2);
  }
#endif
  timespec t{};
  if (clock_getres(CLOCK_MONOTONIC, &t))
    return ClockFailure(0, errno, 2);
  if (t.tv_sec < 0 || t.tv_nsec < 0 || t.tv_nsec >= 1000000000 ||
      uint64_t(t.tv_sec) > (UINT64_MAX - uint64_t(t.tv_nsec)) / 1000000000)
    return ClockFailure(0, ERANGE, 2);
  uint64_t ns = uint64_t(t.tv_sec) * 1000000000 + t.tv_nsec;
  return ns > 0 && ns <= 1000000 ? CheckedTime{true, ns, 0}
                                 : ClockFailure(ns, ERANGE, 2);
}
inline bool RoundingQualified() { return std::fegetround() == FE_TONEAREST; }
} // namespace grs::g9dm
