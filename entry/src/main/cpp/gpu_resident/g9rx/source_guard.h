#pragma once
#include <cstdint>

namespace grs::g9rx {
inline constexpr uint64_t kSourcePulseGapNs = 33333333;
inline constexpr uint64_t kSourceRootLimitNs = 250000000;

enum class SourceReject {
  None, Clock, Context, NoPermit, OwnerInactive, RootNotLive, Scope, Generation,
  Permit, BeforeRoot, Deadline, MissingPulse, PulseFromFuture, PulseExpired,
  CommandFromFuture, CommandExpired, Queue, Pending, Typed, Commit
};

struct SourceTimes {
  bool clockOk = false, hasPulse = false;
  uint64_t now = 0, pulse = 0, receipt = 0, begin = 0, deadline = 0;
};

constexpr SourceReject SourceTimeGuard(const SourceTimes &t) {
  if (!t.clockOk) return SourceReject::Clock;
  if (t.now < t.begin) return SourceReject::BeforeRoot;
  if (t.now > t.deadline) return SourceReject::Deadline;
  if (!t.hasPulse) return SourceReject::MissingPulse;
  if (t.now < t.pulse) return SourceReject::PulseFromFuture;
  if (t.now - t.pulse > kSourcePulseGapNs) return SourceReject::PulseExpired;
  if (t.now < t.receipt) return SourceReject::CommandFromFuture;
  if (t.now - t.receipt > kSourceRootLimitNs) return SourceReject::CommandExpired;
  return SourceReject::None;
}

inline const char *SourceRejectName(SourceReject reason) {
  switch (reason) {
  case SourceReject::None: return "NONE";
  case SourceReject::Clock: return "CLOCK";
  case SourceReject::Context: return "CONTEXT";
  case SourceReject::NoPermit: return "NO_PERMIT";
  case SourceReject::OwnerInactive: return "OWNER_INACTIVE";
  case SourceReject::RootNotLive: return "ROOT_NOT_LIVE";
  case SourceReject::Scope: return "SCOPE";
  case SourceReject::Generation: return "GENERATION";
  case SourceReject::Permit: return "PERMIT";
  case SourceReject::BeforeRoot: return "BEFORE_ROOT";
  case SourceReject::Deadline: return "DEADLINE";
  case SourceReject::MissingPulse: return "MISSING_PAGE_PULSE";
  case SourceReject::PulseFromFuture: return "PULSE_FROM_FUTURE";
  case SourceReject::PulseExpired: return "PULSE_EXPIRED";
  case SourceReject::CommandFromFuture: return "COMMAND_FROM_FUTURE";
  case SourceReject::CommandExpired: return "COMMAND_EXPIRED";
  case SourceReject::Queue: return "FROZEN_QUEUE";
  case SourceReject::Pending: return "QUOTA_ALREADY_PENDING";
  case SourceReject::Typed: return "TYPED_PROOF";
  case SourceReject::Commit: return "COMMIT_PROOF";
  }
  return "UNKNOWN";
}
} // namespace grs::g9rx
