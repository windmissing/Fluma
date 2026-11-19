#pragma once
#include "loaded_identity.h"
#include "planning_control.h"
#include "rx_protocol.h"
#include "transport.h"
namespace grs::g9rx {
class Controller {
  mutable RxProtocol protocol_;
  Transport transport_;
  Config config_;
  ControlIdentity identity_{};
  StageCapabilityV2 caps_{};
  ProgressClockV3 clock_;
  RunControl run_;
  PlanningControl planning_{clock_, run_};
  Permit active_{};
  Completion256 tail_{};
  uint64_t nonce_ = 0, frame_ = 0, commit_ = 0, quanta_ = 0, generation_ = 0,
           publication_ = 0, poseSequence_ = 0, poseObserved_ = 0;
  GLsync sceneFence_ = nullptr;
  bool configured_ = false, lastGood_ = false, stale_ = false, lease_ = false,
       coldPin_ = false;
  LoadedIdentity loaded_{};
  bool RecheckLocked(const Permit &, uint64_t) const;
  static bool Check(void *, const Permit &, uint64_t);
  void RevokeLocked(uint32_t);
  void DropHeadLocked(uint64_t, uint32_t);
  void ExpireLocked(uint64_t);
  bool FreezeLocked(uint64_t, uint64_t, uint64_t, Input128 &, Permit &);

public:
  Controller();
  RxProtocol &Protocol() { return protocol_; }
  Result ConfigureResearch(const Config &, const ControlIdentity &,
                           const void *loadedFunction);
  std::string Session() const;
  std::string LoadedReceipt() const;
  void InvalidateGeometry();
  bool SetActive(bool, const ControlIdentity &, CheckedTime);
  ControlResult Pulse(const PagePulseV3 &, CheckedTime);
  ControlResult Target(const ControlIdentity &, int32_t, CheckedTime);
  Result OfferSource(Input128, uint64_t intent, CheckedTime);
  Result BeginRoot(CheckedTime, uint64_t frame, uint64_t geometry);
  Result PumpBounded(CheckedTime);
  Result Stop();
  bool Drain();
  void Release(bool current);
  struct SceneView {
    bool valid = false, stale = false;
    GLuint buffer = 0;
    uint64_t generation = 0;
    ControlIdentity identity{};
    uint32_t k = 0;
  };
  SceneView AcquireScene();
  bool ReleaseScene(uint64_t, GLsync);
  bool PollSceneFence();
  bool CanPresent(uint64_t, uint64_t) const;
  bool PublishMapping(uint64_t generation, uint32_t width, uint32_t height,
                      double density, CheckedTime actualPresent);
  const Profile *TransportProfile() const {
    std::lock_guard<std::mutex> lock(protocol_.sync.protocol);
    return configured_ ? SelectProfile(config_.k) : nullptr;
  }
  bool ReadyToReconfigure() const {
    std::lock_guard<std::mutex> lock(protocol_.sync.protocol);
    return !transport_.Busy() && !sceneFence_ && !lease_ && !coldPin_;
  }
  bool FullTraceEnabled() const {
    std::lock_guard<std::mutex> lock(protocol_.sync.protocol);
    return configured_ && config_.fullTrialTrace;
  }
  void InvalidatePresentedMapping() {
    protocol_.sync.mappingPending.store(true);
    std::lock_guard<std::mutex> lock(protocol_.sync.protocol);
    protocol_.payload.metadata[3][2] = 0;
  }
  void MarkSceneFault() {
    InvalidatePresentedMapping();
    std::lock_guard<std::mutex> lock(protocol_.sync.protocol);
    stale_ = lastGood_;
  }
  bool ReadyForScene() const {
    std::lock_guard<std::mutex> lock(protocol_.sync.protocol);
    return configured_ && lastGood_ && !stale_ &&
           !protocol_.payload.control[RxProtocol::ROOT_LIVE] &&
           !transport_.Busy() && !lease_ && !sceneFence_ &&
           clock_.mode == PlanningMode::RUNNABLE &&
           protocol_.sync.ownerActive.load();
  }
  bool NeedsRetirement() const { return !protocol_.sync.ownerActive.load(); }
  bool RootLive() const {
    std::lock_guard<std::mutex> lock(protocol_.sync.protocol);
    return protocol_.payload.control[RxProtocol::ROOT_LIVE];
  }
  uint64_t SceneGeneration() const {
    std::lock_guard<std::mutex> lock(protocol_.sync.protocol);
    return generation_;
  }
  const StageCapabilityV2 &Caps() const { return caps_; }
  bool CaptureCold(uint32_t *, size_t, bool, uint64_t, uint64_t, uint64_t,
                   uint32_t *);
  std::string Status() const;
  const Completion256 &LastCompletion() const { return tail_; }
};
} // namespace grs::g9rx
