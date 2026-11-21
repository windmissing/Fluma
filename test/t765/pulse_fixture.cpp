#include "progress_clock_v3.h"
#include "source_claim.h"
#include "source_guard.h"
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>

using namespace grs::g9rx;

namespace {
constexpr uint64_t begin = 1000000000, deadline = 1250000000;
constexpr ControlIdentity scope{1, 2, 3, 4, 5, 6, 7, 8, 9};

void Check(const char *name, bool passed) {
  if (!passed) throw std::runtime_error(name);
  std::cout << name << " PASS\n";
}

ProgressClockV3 Ready() {
  ProgressClockV3 clock;
  clock.identity = scope;
  clock.open = clock.ready = clock.uiFresh = clock.gravityFresh = true;
  return clock;
}

void ExerciseClock() {
  auto clock = Ready();
  const auto result = clock.PulseLocked({scope, 1, begin}, begin);
  Check("first_actual_clock_pulse_baseline",
        result == ControlResult::OK && clock.baseline && clock.pending == 0 &&
        clock.counters.receiveNs == begin);
  RunControl run;
  PlanGrantV3 grant{};
  Check("zero_quota_claim",
        clock.ClaimLocked(begin + 1, true, run, grant) && grant.quanta == 0 &&
        run.sourcePhase && !clock.activeRemaining);

  constexpr uint64_t t = begin + 20000000;
  std::mutex protocol;
  std::condition_variable published;
  bool ready = false;
  ControlResult pulse = ControlResult::STALE_REJECTED;
  const uint64_t oldOwnerNow = t; // First edge is deliberately outside the lock.
  std::thread ui([&] {
    {
      std::lock_guard<std::mutex> lock(protocol);
      pulse = clock.PulseLocked({scope, 2, t + 1}, t + 1);
      ready = true;
    }
    published.notify_one();
  });
  SourceTimes locked{};
  {
    std::unique_lock<std::mutex> lock(protocol);
    published.wait(lock, [&] { return ready; });
    locked = {true, clock.hasPulse, t + 2, clock.counters.receiveNs, begin, begin, deadline};
  }
  ui.join();
  auto mixed = locked;
  mixed.now = oldOwnerNow;
  Check("old_owner_new_pulse", SourceTimeGuard(mixed) == SourceReject::PulseFromFuture);
  Check("locked_resample", pulse == ControlResult::OK && SourceTimeGuard(locked) == SourceReject::None);

  auto target = Ready();
  target.PulseLocked({scope, 1, begin}, begin);
  const auto targetReply = target.RegisterTargetLocked(scope, 0);
  Check("target_does_not_publish_pulse",
        targetReply == ControlResult::OK && target.counters.pulseSequence == 1 &&
        target.counters.receiveNs == begin && !target.Fresh(begin + 33333334));
  auto wrong = scope;
  ++wrong.context;
  Check("wrong_scope_does_not_refresh",
        target.PulseLocked({wrong, 2, begin + 2}, begin + 2) == ControlResult::STALE_REJECTED &&
        target.counters.receiveNs == begin && target.counters.pulseSequence == 1);
  Check("duplicate_sequence_does_not_refresh",
        target.PulseLocked({scope, 1, begin + 3}, begin + 3) == ControlResult::SEQUENCE_REJECTED &&
        target.counters.receiveNs == begin);

  auto pending = Ready();
  pending.PulseLocked({scope, 1, begin}, begin);
  pending.PulseLocked({scope, 2, begin + 9000000}, begin + 9000000);
  Check("pending_before_claim_rejected_without_mutation",
        pending.pending == 1 &&
        SourceClaimGate(true, pending.baseline, pending.Fresh(begin + 9000000),
                        true, true, pending.pending) == SourceClaim::QuotaAlreadyPending &&
        pending.pending == 1 && pending.grant.id == 0);
  auto gap = Ready();
  gap.PulseLocked({scope, 1, begin}, begin);
  Check("real_ui_gap_revokes",
        gap.PulseLocked({scope, 2, begin + 33333334}, begin + 33333333) ==
            ControlResult::CLOCK_GAP &&
        !gap.Fresh(begin + 33333333));
  auto stopped = Ready();
  stopped.PulseLocked({scope, 1, begin}, begin);
  stopped.RevokeLocked({ControlResult::STALE_REJECTED});
  Check("stop_is_not_repaired_by_pulse",
        stopped.PulseLocked({scope, 2, begin + 1}, begin + 1) == ControlResult::STALE_REJECTED &&
        !stopped.Fresh(begin + 1));
}
} // namespace

int main(int argc, char **) {
  try {
    if (argc == 2) {
      std::string name;
      uint64_t now, pulse, receipt, root, limit;
      unsigned clockOk, hasPulse;
      while (std::cin >> name >> clockOk >> hasPulse >> now >> pulse >> receipt >> root >> limit) {
        const SourceTimes sample{bool(clockOk), bool(hasPulse), now, pulse, receipt, root, limit};
        std::cout << name << ' ' << SourceRejectName(SourceTimeGuard(sample)) << '\n';
      }
      return std::cin.eof() ? 0 : 2;
    }
    ExerciseClock();
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
