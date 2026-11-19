#include "planning_control.h"
namespace grs::g9rx {
ControlResult PlanningControl::Recheck(const Permit &p, uint64_t now) const {
  if (p.identity != clock_.identity)
    return ControlResult::STALE_REJECTED;
  if (now > p.deadline)
    return ControlResult::DEADLINE;
  if (!clock_.Fresh(now) || !clock_.ready || !clock_.baseline ||
      clock_.mode != PlanningMode::RUNNABLE)
    return ControlResult::STALE_REJECTED;
  if (!clock_.grant.id || p.plan != clock_.grant.id || !run_.active ||
      p.plan != run_.plan || p.nonce != run_.nonce || !clock_.jobLive ||
      !run_.jobLive)
    return ControlResult::STALE_REJECTED;
  return ControlResult::OK;
}
ControlResult PlanningControl::BlockPlanningControlLocked(
    const ValidatedBlockedControlRecord &tail, uint64_t now) {
  const auto &p = tail.Permission();
  if (p.identity == clock_.retiredIdentity &&
      p.plan == clock_.counters.retiredPlan &&
      p.nonce == clock_.counters.retiredNonce)
    return ControlResult::ALREADY_RETIRED;
  const auto valid = Recheck(p, now);
  if (valid != ControlResult::OK)
    return valid;
  if (p.kind != 2 || clock_.activeRemaining == 0)
    return ControlResult::STALE_REJECTED;
  auto next = clock_.counters;
  if (!CheckedAdd(next.blockedActive, clock_.activeRemaining,
                  next.blockedActive) ||
      !CheckedAdd(next.blockedPending, clock_.pending, next.blockedPending) ||
      !CheckedAdd(next.blockedClearedCarry, next.carry,
                  next.blockedClearedCarry) ||
      !CheckedAdd(next.blockedEpisodes, 1, next.blockedEpisodes)) {
    clock_.fault = ControlResult::QUOTA_OVERFLOW;
    return ControlResult::QUOTA_OVERFLOW;
  }
  next.carry = 0;
  next.retiredPlan = p.plan;
  next.retiredNonce = p.nonce;
  next.blockedBasePhysics = p.baseCommit;
  next.blockedOnInputSeq = next.meaningfulInputSeq;
  next.blockedOnPulseSeq = next.pulseSequence;
  next.rearmInputSeq = next.rearmAfterPulseSeq = 0;
  if (!run_.RetireBlockedPlanLocked(p.plan, p.nonce))
    return ControlResult::STALE_REJECTED;
  clock_.counters = next;
  clock_.pending = clock_.activeRemaining = 0;
  clock_.grant = {};
  clock_.jobLive = false;
  clock_.activeRootKind = 0;
  clock_.retiredIdentity = p.identity;
  clock_.blockReason = tail.Words()[7];
  clock_.mode = PlanningMode::BLOCKED_WAIT;
  clock_.lastGoodStale = true;
  return ControlResult::OK;
}
ControlResult PlanningControl::AcceptQuantumLocked(const ValidatedRecord &tail,
                                                   uint64_t now) {
  const auto &p = tail.Permission();
  const auto valid = Recheck(p, now);
  if (valid != ControlResult::OK)
    return valid;
  if (p.kind != 2 || clock_.activeRemaining == 0)
    return ControlResult::STALE_REJECTED;
  if (clock_.counters.accepted == UINT64_MAX) {
    clock_.fault = ControlResult::QUOTA_OVERFLOW;
    return ControlResult::QUOTA_OVERFLOW;
  }
  ++clock_.counters.accepted;
  --clock_.activeRemaining;
  clock_.jobLive = run_.jobLive = false;
  clock_.activeRootKind = 0;
  if (clock_.activeRemaining == 0) {
    run_.active = false;
    run_.plan = run_.nonce = 0;
    run_.quantumOrdinal = run_.continuationStage = 0;
    clock_.grant = {};
  }
  return ControlResult::OK;
}
} // namespace grs::g9rx
