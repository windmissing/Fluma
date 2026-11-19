#include "progress_clock_v3.h"
#include <algorithm>
namespace grs::g9rx {
namespace {
bool Increment(uint64_t &n, uint64_t amount = 1) {
  uint64_t result;
  if (!CheckedAdd(n, amount, result))
    return false;
  n = result;
  return true;
}
} // namespace
ControlResult ProgressClockV3::Overflow() {
  fault = ControlResult::QUOTA_OVERFLOW;
  return fault;
}
bool ProgressClockV3::Fresh(uint64_t now) const {
  return fault == ControlResult::OK && open && hasPulse &&
         now >= counters.receiveNs && now - counters.receiveNs <= GapNs;
}
bool ProgressClockV3::Balanced() const {
  const auto &c = counters;
  auto right = WideUnsigned::Multiply(c.generated, 1000000000);
  if (!right.Add(c.carry) || !right.Add(c.clearedCarry) ||
      !right.Add(c.blockedClearedCarry) ||
      !(right == WideUnsigned::Multiply(c.creditedElapsedNs, 120)))
    return false;
  uint64_t sum = 0;
  for (auto n :
       {c.accepted, uint64_t(activeRemaining), uint64_t(pending), c.overload,
        c.revoked, c.blockedActive, c.blockedPending, c.blockedWait})
    if (!Increment(sum, n))
      return false;
  return sum == c.generated && c.carry < Denominator;
}
ControlResult ProgressClockV3::RevokeLocked(ControlFault reason) {
  auto next = counters;
  if (!Increment(next.revoked, uint64_t(pending) + activeRemaining) ||
      !Increment(next.clearedCarry, next.carry))
    return Overflow();
  next.carry = 0;
  counters = next;
  pending = activeRemaining = 0;
  grant = {};
  jobLive = false;
  activeRootKind = 0;
  open = ready = baseline = false;
  mode = PlanningMode::REVOKED;
  fault = reason.reason;
  return reason.reason;
}
ControlResult ProgressClockV3::PulseLocked(const PagePulseV3 &pulse,
                                           uint64_t now) {
  if (fault != ControlResult::OK)
    return fault;
  if (pulse.identity != identity || !open)
    return ControlResult::STALE_REJECTED;
  if (!pulse.sequence || pulse.sequence <= counters.pulseSequence)
    return ControlResult::SEQUENCE_REJECTED;
  auto next = counters;
  auto nextMode = mode;
  auto nextPending = pending;
  bool nextBaseline = baseline;
  if (hasPulse) {
    if (pulse.uiNs <= next.uiNs || now < next.receiveNs)
      return RevokeLocked({ControlResult::CONTROL_CLOCK_REVERSED});
    const auto nativeGap = now - next.receiveNs, uiGap = pulse.uiNs - next.uiNs;
    if (nativeGap > GapNs || uiGap > GapNs)
      return RevokeLocked({ControlResult::CLOCK_GAP});
    next.maxNativeGap = std::max(next.maxNativeGap, nativeGap);
    next.maxUiGap = std::max(next.maxUiGap, uiGap);
    if (ready && baseline) {
      const auto scaled = next.carry + 120 * nativeGap;
      const auto fresh = scaled / Denominator;
      next.carry = scaled % Denominator;
      if (!Increment(next.generated, fresh) ||
          !Increment(next.creditedElapsedNs, nativeGap))
        return Overflow();
      if (mode == PlanningMode::BLOCKED_WAIT ||
          mode == PlanningMode::REARM_PENDING) {
        if (!Increment(next.blockedWait, fresh) ||
            !Increment(next.blockedWaitNs, nativeGap) ||
            !Increment(next.blockedPulses))
          return Overflow();
        if (mode == PlanningMode::REARM_PENDING &&
            pulse.sequence > next.rearmAfterPulseSeq &&
            next.meaningfulInputSeq >= next.rearmInputSeq &&
            next.rearmInputSeq > next.blockedOnInputSeq) {
          if (!Increment(next.blockedClearedCarry, next.carry))
            return Overflow();
          next.carry = 0;
          nextMode = PlanningMode::RUNNABLE;
        }
      } else if (mode == PlanningMode::RUNNABLE) {
        const auto keep = std::min<uint64_t>(fresh, 4 - nextPending);
        nextPending += uint32_t(keep);
        if (!Increment(next.overload, fresh - keep))
          return Overflow();
      } else
        return ControlResult::STALE_REJECTED;
    } else if (ready)
      nextBaseline = true;
  } else if (ready)
    nextBaseline = true;
  if (!Increment(next.pulses))
    return Overflow();
  next.pulseSequence = pulse.sequence;
  next.uiNs = pulse.uiNs;
  next.receiveNs = now;
  counters = next;
  mode = nextMode;
  pending = nextPending;
  baseline = nextBaseline;
  hasPulse = true;
  return ControlResult::OK;
}
ControlResult ProgressClockV3::MeaningfulInput() {
  if (counters.meaningfulInputSeq == UINT64_MAX)
    return Overflow();
  ++counters.meaningfulInputSeq;
  if (mode == PlanningMode::BLOCKED_WAIT) {
    mode = PlanningMode::REARM_PENDING;
    counters.rearmAfterPulseSeq = counters.pulseSequence;
  }
  if (mode == PlanningMode::REARM_PENDING)
    counters.rearmInputSeq = counters.meaningfulInputSeq;
  return ControlResult::OK;
}
ControlResult
ProgressClockV3::RegisterTargetLocked(const ControlIdentity &expected,
                                      int32_t value) {
  if (fault != ControlResult::OK)
    return fault;
  if (expected != identity || !open)
    return ControlResult::STALE_REJECTED;
  if (value < -7680 || value > 7680)
    return ControlResult::INVALID_ARGUMENT;
  if (value == target)
    return ControlResult::OK;
  const auto result = MeaningfulInput();
  if (result == ControlResult::OK)
    target = value;
  return result;
}
ControlResult
ProgressClockV3::RegisterModeLocked(const ControlIdentity &expected,
                                    GravityMode value) {
  if (fault != ControlResult::OK)
    return fault;
  if (expected != identity || !open)
    return ControlResult::STALE_REJECTED;
  if (value != GravityMode::MANUAL && value != GravityMode::SENSOR)
    return ControlResult::INVALID_ARGUMENT;
  if (value == gravityMode)
    return ControlResult::OK;
  const auto result = MeaningfulInput();
  if (result == ControlResult::OK)
    gravityMode = value;
  return result;
}
ControlResult ProgressClockV3::RegisterSourceIntentLocked(
    const SourceIntentKey &key, const InputPacket &packet, size_t freeSlot) {
  if (fault != ControlResult::OK)
    return fault;
  if (!open || key.run != identity.run || key.owner != identity.owner ||
      key.context != identity.context ||
      key.inputEpoch != identity.inputEpoch ||
      key.pageSession != identity.pageSession)
    return ControlResult::STALE_REJECTED;
  if (freeSlot >= intents.size())
    return ControlResult::WOULD_BLOCK;
  if (key.sourceControlGeneration != sourceControlGeneration ||
      !key.userIntentSequence)
    return ControlResult::INVALID_ARGUMENT;
  if (packet.words[0] != 1481788999 || packet.words[1] != 3 ||
      packet.words[40] != 1)
    return ControlResult::INVALID_ARGUMENT;
  uint64_t newest = counters.sourceIntentSeq;
  for (size_t slot = 0; slot < intents.size(); ++slot)
    if (auto item = intents.Get(slot);
        item.generation == key.sourceControlGeneration)
      newest = std::max(newest, item.sequence);
  if (key.userIntentSequence <= newest)
    return ControlResult::ALREADY_RETIRED;
  if (counters.sourceBoundary == UINT64_MAX)
    return Overflow();
  const auto result = MeaningfulInput();
  if (result != ControlResult::OK)
    return result;
  intents.Set(freeSlot, {key.sourceControlGeneration, key.userIntentSequence});
  counters.sourceIntentSeq = key.userIntentSequence;
  ++counters.sourceBoundary;
  return ControlResult::OK;
}
ControlResult
ProgressClockV3::BeginSessionLocked(const RecoveryHandoff &handoff) {
  if (!handoff.identity.pageSession || handoff.identity.run != identity.run ||
      handoff.identity.pageSession <= identity.pageSession)
    return ControlResult::STALE_REJECTED;
  auto next = *this;
  const auto result = next.RevokeLocked({ControlResult::STALE_REJECTED});
  if (result == ControlResult::QUOTA_OVERFLOW)
    return Overflow();
  next.identity = handoff.identity;
  next.counters.session = handoff.identity.pageSession;
  next.counters.pulseSequence = next.counters.uiNs = next.counters.receiveNs =
      0;
  next.counters.sourceIntentSeq = 0;
  next.intents.Clear();
  next.counters.meaningfulInputSeq = next.counters.blockedOnInputSeq =
      next.counters.rearmInputSeq = next.counters.blockedOnPulseSeq =
          next.counters.rearmAfterPulseSeq = 0;
  next.open = true;
  next.hasPulse = false;
  next.mode = PlanningMode::RUNNABLE;
  next.fault = ControlResult::OK;
  *this = next;
  return ControlResult::OK;
}
ControlResult ProgressClockV3::ResetRunLocked(const RetiredRunHandoff &handoff,
                                              const RunControl &run) {
  if (!run.workspaceRetired || run.active || run.jobLive)
    return ControlResult::WOULD_BLOCK;
  if (!handoff.identity.run || handoff.identity.run == identity.run)
    return ControlResult::STALE_REJECTED;
  auto span = intents;
  *this = ProgressClockV3{};
  intents = span;
  intents.Clear();
  identity = handoff.identity;
  counters.session = identity.pageSession;
  open = true;
  return ControlResult::OK;
}
bool ProgressClockV3::CanClaim(uint64_t now, bool source,
                               const RunControl &run) const {
  return mode == PlanningMode::RUNNABLE && ready && baseline && Fresh(now) &&
         uiFresh && gravityFresh && !grant.id && !jobLive && !run.active &&
         run.workspaceRetired && (pending > 0 || source) &&
         counters.planSerial < UINT64_MAX;
}
bool ProgressClockV3::ClaimLocked(uint64_t now, bool source, RunControl &run,
                                  PlanGrantV3 &out) {
  if (!CanClaim(now, source, run))
    return false;
  grant = {++counters.planSerial,
           now,
           counters.carry,
           identity.pageSession,
           counters.sourceBoundary,
           counters.meaningfulInputSeq,
           pending,
           target};
  activeRemaining = pending;
  pending = 0;
  run.active = true;
  run.plan = grant.id;
  run.quantumOrdinal = 0;
  run.sourcePhase = source;
  out = grant;
  return true;
}
ControlResult ProgressClockV3::RetireSourcePlanLocked(uint64_t plan,
                                                      RunControl &run) {
  if (plan != grant.id || !run.active || run.plan != plan || activeRemaining ||
      jobLive || !run.workspaceRetired)
    return ControlResult::STALE_REJECTED;
  run.active = false;
  run.sourcePhase = false;
  run.plan = 0;
  grant = {};
  return ControlResult::OK;
}
} // namespace grs::g9rx
