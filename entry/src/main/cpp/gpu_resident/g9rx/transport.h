#pragma once
#include "../cold_slot_broker.h"
#include "../uniform_locations.h"
#include "completion_gate.h"
#include "receipt_clock.h"
#include "stage_capability.h"
#include <EGL/egl.h>
#include <thread>
namespace grs::g9rx {
class Transport {
  struct Program {
    GLuint id = 0;
    UniformLocations uniforms;
  };
  Program program_;
  // 0..4 complete states: CURRENT,BASE,CANDIDATE,SELECTED,REPLAY. 5 WORK;
  // 6 TRIAL,7 SELECTED stream,8/9 PUBLIC pairs. Six optional trial archives.
  std::array<GLuint, 10> buffers_{};
  std::array<GLuint, 6> full_{};
  const Profile *profile_ = nullptr;
  Config config_;
  ControlIdentity identity_{};
  Permit permit_{};
  EGLDisplay display_ = EGL_NO_DISPLAY;
  EGLContext context_ = EGL_NO_CONTEXT;
  EGLSurface surface_ = EGL_NO_SURFACE;
  std::thread::id thread_;
  GLsync fence_ = nullptr;
  Completion256 tail_{};
  NativeEvidence evidence_{};
  Input128 lastInput_{};
  std::array<uint32_t, 128> lastConfig_{};
  uint32_t coldGl_ = 0, coldEgl_ = 0;
  uint32_t next_ = 0, current_ = 0, firstGl_ = 0, firstEgl_ = 0;
  coldbroker::GpuLease fullGpuCredit_;
  uint64_t rootBeginNs_ = 0, qualifiedClockGeneration_ = 0,
           commitObservedNs_ = 0;
  mutable bool clockFailed_ = false;
  bool ready_ = false, busy_ = false, mapped_ = false;
  using Check = bool (*)(void *, const Permit &, uint64_t);
  Check check_ = nullptr;
  void *checkUser_ = nullptr;
  bool ReadNow(CheckedTime &) const;
  bool Current() const;
  bool Good();
  bool Allowed() const;
  Result Denied() const;
  void U(const char *, uint32_t);
  void I(const char *, int32_t);
  void P(const char *, uint64_t);
  void Quad(const char *, const uint32_t *);
  bool Copy(GLuint, size_t, GLuint, size_t, size_t);
  bool Dispatch(const Pass &, uint32_t);
  void Bind(const Pass &);
  void Common(const Input128 &, const std::array<uint32_t, 8> &layout,
              const std::array<uint32_t, 8> &shader);

public:
  Result Create(const Config &, const ControlIdentity &,
                const SeedFileIdentity &);
  // T564 (problem T560 搂2): COMPUTE_ONLY eligibility, queried once on the
  // current owner context; vertex/fragment shader-storage blocks are recorded
  // but no longer gate compute.
  Result QueryComputeCaps(const ProfileRequirements &, StageCapabilityV2 *);
  Result Begin(const Permit &, const Input128 &, Check, void *);
  Result Pump();
  bool CommitAccepted(const ValidatedRecord &);
  bool Commit(const Permit &, const Completion256 &);
  std::optional<ValidatedRecord> AcceptedProof() const;
  std::optional<ValidatedBlockedControlRecord> BlockedProof() const;
  bool CaptureBuffer(uint32_t *dst, size_t words, uint32_t kind,
                     uint32_t index);
  uint64_t CommitObservedNs() const { return commitObservedNs_; }
  uint64_t RootBegin() const { return rootBeginNs_; }
  Digest SeedHash() const { return config_.seedSha; }
  bool RetireRejected(const Permit &);
  void Release(bool contextCurrent);
  bool Busy() const { return busy_; }
  uint32_t CurrentBank() const { return current_; }
  const Completion256 &Tail() const { return tail_; }
  const NativeEvidence &Evidence() const { return evidence_; }
  GLuint StateBuffer(uint32_t bank) const {
    return bank < 5 ? buffers_[bank] : 0;
  }
  size_t StateBytes() const { return profile_ ? profile_->stateBytes : 0; }
  const Profile *SelectedProfile() const { return profile_; }
  const Config &Configuration() const { return config_; }
  std::array<uint32_t, 8> ShaderHash() const;
  // Owner-only direct spans. Caller reserves full credit before invoking; no
  // intermediate vector or live mapped storage escapes this call.
  uint32_t ColdGl() const { return coldGl_; }
  uint32_t ColdEgl() const { return coldEgl_; }
  bool CaptureSpan(uint32_t *caller, size_t words, bool full,
                   const uint32_t *quota, uint64_t request, uint64_t trace,
                   uint64_t created);
};
const char *PipelineSource();
const char *PipelineSourceHash();
} // namespace grs::g9rx
