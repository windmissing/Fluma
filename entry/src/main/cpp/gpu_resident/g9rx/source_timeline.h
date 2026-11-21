#pragma once
#include "research_config.h"
#include "source_guard.h"

namespace grs::g9rx {
struct SourceGuard {
  SourceTimes times{};
  ControlIdentity expected{}, actual{};
  uint64_t clock = 0, lifecycle = 0, currentClock = 0, currentLifecycle = 0;
  uint64_t nonce = 0, plan = 0, currentNonce = 0, currentPlan = 0, sequence = 0;
  uint32_t pending = 0, cursor = 0, dispatches = 0;
  int rawErrno = 0;
  const char *phase = "";
  SourceReject reason = SourceReject::None;
  bool active = false, rootLive = false;
};

struct SourceEdge {
  const char *stage = "";
  uint64_t ns = 0, a = 0, b = 0;
  uint32_t cursor = 0, dispatches = 0;
};
struct SourcePulse {
  ControlIdentity scope{};
  uint64_t sequence = 0, ui = 0, receipt = 0, published = 0;
  uint64_t nativeGap = 0, uiGap = 0;
  uint32_t result = 0;
};
template<class T, size_t N> struct SourceLog {
  std::array<T, N> rows{};
  T last{};
  size_t size = 0, dropped = 0;
  void Add(T row) {
    last = row;
    if (size < N) rows[size++] = row;
    else ++dropped;
  }
};

inline Result SourceGuardResult(SourceReject reason) {
  switch (reason) {
  case SourceReject::None: return Result::OK;
  case SourceReject::Clock:
  case SourceReject::BeforeRoot:
  case SourceReject::PulseFromFuture:
  case SourceReject::CommandFromFuture: return Result::INPUT_CLOCK_UNVERIFIED;
  case SourceReject::Context: return Result::CONTEXT_LOST;
  case SourceReject::Deadline: return Result::DEADLINE;
  case SourceReject::Typed: return Result::INVALID;
  default: return Result::STALE;
  }
}
} // namespace grs::g9rx
