#pragma once
#include "transport.h"
namespace grs::g13 {
// All methods except immutable snapshot copies execute on the GL owner. The
// bridge serializes commands; no GL/map/hash happens under its UI mutex.
class Controller {
  Transport transport_;
  Identity identity_{};
  Config config_{};
  Permit permit_{};
  Completion tail_{};
  struct Command {
    Input original{}, canonical{};
    uint64_t serial = 0;
  };
  std::array<Command, 16> fifo_{};
  size_t head_ = 0, count_ = 0;
  uint64_t nonce_ = 0, plan_ = 0, commit_ = 0, quantum_ = 0, lastPulse_ = 0,
           pulseSeq_ = 0, lastUiNs_ = 0, earned_ = 0, reserved_ = 0,
           consumed_ = 0, discarded_ = 0, clockRemainder_ = 0,
           commandSerial_ = 0, lastSource_ = 0;
  bool configured_ = false, live_ = false, active_ = false, cancelled_ = false,
       sceneLease_ = false, coldPin_ = false;
  uint32_t failure_ = 0;
  GLsync sceneFence_ = nullptr;
  std::array<uint32_t, 256> quota_{};
  uint32_t diagnostic_ = 0, forcedJ_ = 0;
  bool nativeControl_ = false;
  uint64_t maxUiGap_ = 0, maxNativeGap_ = 0, pulses_ = 0;
  bool coldHold_ = false, rebind_ = false;
  Identity acceptedIdentity_{};
  uint64_t acceptedNonce_ = 0;
  std::string baseToken_;
  uint64_t issuedNonce_ = 0, issuedPlan_ = 0;
  Completion acceptedTail_{};
  bool runFault_ = false;
  uint64_t lastRootPoll_ = 0;
  uint32_t lastRootError_ = 0, lastRootOutcome_ = 0;
  bool lastRootGpuTail_ = false;
  uint64_t clearedCarry_ = 0, lastSourceIntent_ = 0, staleFrames_ = 0,
           clockFault_ = 0;
  uint64_t overload_ = 0, revokedQuota_ = 0, newQuantumFrames_ = 0,
           sourceFrames_ = 0, repeatFrames_ = 0;
  void *authorityContext_ = nullptr;
  bool (*authority_)(void *) = nullptr;
  static uint32_t Guard(void *, uint64_t);
  uint32_t AcceptPulse(uint64_t, uint64_t, uint64_t, uint64_t, bool);
  uint32_t Complete(uint64_t);
  bool Fresh(uint64_t) const;
  bool RetireSceneFence();
  void SnapshotQuota(const Permit &, uint64_t);

public:
  struct SceneLease {
    GLuint field = 0, pack = 0;
    const Profile *profile = nullptr;
    uint64_t generation = 0;
    Identity identity{};
    bool valid = false;
  };
  void SetAuthority(void *context, bool (*check)(void *)) {
    authorityContext_ = context;
    authority_ = check;
  }
  uint32_t Configure(const Config &, const Seed &, const Identity &);
  uint32_t Offer(const Input &, uint64_t);
  uint32_t Pulse(uint64_t session, uint64_t sequence, uint64_t rawUiNs,
                 uint64_t now);
  uint32_t NativePulse(uint64_t sequence, uint64_t now);
  uint32_t BeginRoot(const Permit &);
  uint32_t BeginGranted(uint64_t now);
  uint32_t PumpBounded(uint64_t now);
  bool Poll(Completion &) const;
  uint32_t Rebind(uint64_t surface, uint64_t geometry);
  void SetActive(bool, const Identity &);
  void Stop();
  void Release(bool current);
  bool Ready() const { return configured_ && transport_.Ready(); }
  void ContextRetired(EGLContext context) {
    transport_.ContextRetired(context);
  }
  SceneLease AcquireScene();
  bool ReleaseScene(uint64_t, GLsync);
  bool CanPresent(const SceneLease &, uint64_t) const;
  uint32_t CaptureSpan(bool, uint32_t *, size_t, size_t &);
  bool PinCold();
  void UnpinCold() { coldPin_ = false; }
  void HoldCold(bool value) { coldHold_ = value; }
  bool RootIdle() const { return !live_ && !transport_.Busy(); }
  std::string Status() const;
  const Identity &Scope() const { return identity_; }
  const Completion &LastCompletion() const { return tail_; }
  std::string FieldToken() const;
  const std::string &BaseToken() const { return baseToken_; }
  std::string ColdToken() const;
  std::string RootReceipt() const;
  const Completion &AcceptedCompletion() const { return acceptedTail_; }
  void RecordSceneStale() { ++staleFrames_; }
  void RecordSceneSwap(bool source, bool repeated) {
    if (repeated)
      ++repeatFrames_;
    else if (source)
      ++sourceFrames_;
    else
      ++newQuantumFrames_;
  }
  Transport &Gpu() { return transport_; }
  uint64_t Commit() const { return commit_; }
};
} // namespace grs::g13
