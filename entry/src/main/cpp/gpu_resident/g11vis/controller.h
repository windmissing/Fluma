#pragma once
#include "../g9dm/input_gate.h"
#include "../g9dm/progress_clock_v3.h"
#include "../g9spf/input_gate.h"
#include "../g9spf/progress_clock_v3.h"
#include "cold.h"
#include "visual_transport.h"
#include <atomic>
#include <mutex>
namespace grs::g11vis {
struct R1Control {
  using Clock = g9spf::ProgressClockV3;
  using Identity = g9spf::ControlIdentity;
  using Run = g9spf::RunControl;
  using Gate = g9spf::InputGate;
  using Permit = g9spf::Permit;
  using Pulse = g9spf::PagePulseV3;
  using Handoff = g9spf::RecoveryHandoff;
  using Intent = g9spf::SourceIntentKey;
  using Packet = g9spf::InputPacket;
  using Config = g9spf::Config;
  using Grant = g9spf::PlanGrantV3;
  using Mode = g9spf::PlanningMode;
  using Code = g9spf::ControlResult;
  Clock clock;
  Run run;
  Gate gate;
};
struct DMControl {
  using Clock = g9dm::ProgressClockV3;
  using Identity = g9dm::ControlIdentity;
  using Run = g9dm::RunControl;
  using Gate = g9dm::InputGate;
  using Permit = g9dm::Permit;
  using Pulse = g9dm::PagePulseV3;
  using Handoff = g9dm::RecoveryHandoff;
  using Intent = g9dm::SourceIntentKey;
  using Packet = g9dm::InputPacket;
  using Config = g9dm::Config;
  using Grant = g9dm::PlanGrantV3;
  using Mode = g9dm::PlanningMode;
  using Code = g9dm::ControlResult;
  Clock clock;
  Run run;
  Gate gate;
};
class CompositeScene;
class Controller {
public:
  using Result = VisualTransport::Result;
  class SceneLease {
    friend class Controller;
    friend class CompositeScene;
    VisualTransport *transport_ = nullptr;
    uint64_t generation_ = 0, serial_ = 0;
    std::string token_;
    SceneLease(VisualTransport &t, uint64_t g, uint64_t s, std::string token)
        : transport_(&t), generation_(g), serial_(s), token_(std::move(token)) {
    }

  public:
    SceneLease(const SceneLease &) = delete;
    SceneLease(SceneLease &&) = default;
  };

private:
  mutable std::recursive_mutex
      joint_; // publication/time/FIFO/lease only; never held across a GL call
  std::variant<R1Control, DMControl> control_;
  VisualTransport gpu_;
  Cold cold_;
  Scope id_{};
  CompositePermit active_{};
  std::array<Input128, 16> queue_{};
  size_t head_ = 0, size_ = 0;
  Input128 consumedSource_{}, consumedImpulse_{};
  bool sourceConsumed_ = false, impulseConsumed_ = false;
  uint64_t nonce_ = 0, commit_ = 0, quanta_ = 0, frame_ = 0, geometry_ = 0,
           leaseSerial_ = 0;
  GLsync sceneFence_ = nullptr;
  bool configured_ = false, rawReady_ = false, session_ = false, root_ = false,
       lease_ = false, stale_ = false, publication_ = false;
  std::atomic<bool> revoked_{true}, invalidated_{false};
  const char *result_ = "OFF";
  std::string presented_;
  static bool Check(void *, const CompositePermit &, uint64_t);
  bool Recheck(const CompositePermit &, uint64_t) const;
  void Fault(const char *);
  std::string TokenLocked() const;

public:
  static uint64_t Now();
  Result Configure(const ParentAdapter &, const Words64 &, const Scope &);
  Result Pump(uint64_t frame, uint64_t geometry);
  bool SealBootstrap();
  std::string BootstrapPage(uint32_t segment, uint32_t offset);
  std::string BootstrapState() const;
  bool BeginSession(const Scope &);
  bool SetActive(bool, const Scope &);
  bool Pulse(uint64_t session, uint64_t seq, uint64_t uiNs);
  Result Offer(Input128, uint64_t intent);
  Result Model(const Input128 &, uint64_t intent);
  bool Target(int32_t, const Scope &);
  void RevokeAsync() {
    invalidated_.store(true, std::memory_order_release);
    revoked_.store(true, std::memory_order_release);
  }
  void Stop();
  bool Drain();
  bool Release(bool current, bool actualContextRetired = false);
  bool PollSceneFence();
  std::optional<SceneLease> AcquireScene();
  bool CanPresent(const SceneLease &) const;
  bool ReleaseScene(const SceneLease &, GLsync);
  void Presented(const SceneLease &);
  void SceneFault(const SceneLease &);
  std::string Status() const;
  std::string ScopeJson() const;
  std::string ColdBegin(const std::string &, bool);
  std::string ColdState(uint64_t);
  std::string ColdPage(uint64_t, uint32_t, uint32_t);
  std::string ColdEnd(uint64_t);
  std::string ColdCapabilities() const;
  bool ArmWitness(uint64_t nonce, uint32_t micro, uint32_t phase,
                  uint32_t role);
  std::string WitnessState() const;
  std::string WitnessPage(uint32_t);
  bool EndWitness();
  VisualTransport &Gpu() {
    return gpu_;
  } // owner-thread diagnostics; no caller may publish banks
  bool PendingOwnerWork() const {
    return gpu_.Busy() || cold_.Building() || gpu_.Witness().NeedsPump() ||
           (configured_ && !invalidated_.load(std::memory_order_acquire) &&
            !rawReady_ && !gpu_.Initialized());
  }
  bool Busy() const { return gpu_.Busy(); }
  bool Configured() const { return configured_; }
};
} // namespace grs::g11vis
