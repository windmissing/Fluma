#pragma once
#include "../g9spf/stage_capability.h"
#include "../uniform_locations.h"
#include "bootstrap.h"
#include "composite_gate.h"
#include "g11vis_pipeline.h"
#include "phase_witness.h"
#include <EGL/egl.h>
#include <GLES3/gl31.h>
#include <memory>
#include <thread>
namespace grs::g11vis {
class Controller;
struct GpuSegment {
  const char *name = "";
  GLuint buffer = 0;
  size_t byteOffset = 0, bytes = 0;
  uint32_t strideWords = 1;
};
class VisualTransport {
public:
  using Result = g9spf::Result;
  using Check = bool (*)(void *, const CompositePermit &, uint64_t);
  struct CompositeCompletion {
    accept647::Tail words{};
  };
  Result Create(const ParentAdapter &, const Words64 &, const Scope &);
  Result PollBootstrap(uint64_t now);
  bool SealBootstrap(uint64_t now);
  std::string BootstrapPage(uint32_t segment, uint32_t offset, uint64_t now);
  PhaseWitness &Witness() { return witness_; }
  const PhaseWitness &Witness() const { return witness_; }
  void InvalidateBootstrap() { bootstrap_.Invalidate(); }
  bool RetireBootstrap() { return bootstrap_.Retire(Current(), false); }
  const VisualBootstrap &Bootstrap() const { return bootstrap_; }
  Result QueryComputeCaps(const g9spf::ProfileRequirements &,
                          g9spf::StageCapabilityV2 *);
  Result Begin(const CompositePermit &, const Input128 &, Check, void *);
  Result Pump();
  std::optional<ValidatedCompositeAccepted> Accepted() const;
  std::optional<ValidatedCompositeBlocked> Blocked() const;
  // This method performs no GL/EGL calls and is invoked under the owner's joint
  // lock.
  bool CommitComposite(const ValidatedCompositeAccepted &, uint64_t now);
  bool RetireRejected(const CompositePermit &);
  bool Release(bool current, bool contextActuallyRetired = false);
  bool Busy() const { return busy_; }
  bool Initialized() const { return ready_; }
  uint64_t ContextGeneration() const { return identity_.context; }
  bool HasValidatedPublication() const { return published_; }
  uint32_t CurrentBank() const { return 0; }
  GLuint StateBuffer(uint32_t bank) const {
    return bank < 5 ? buffers_[bank] : 0;
  }
  GLuint WorkBuffer() const { return buffers_[5]; }
  const VisualResources *Resources() const { return visual_; }
  const ParentAdapter *Parent() const { return parent_ ? &*parent_ : nullptr; }
  const Scope &Identity() const { return identity_; }
  const Digest &PhysicalSeed() const { return seedSHA_; }
  const Words64 &VisualConfiguration() const { return visualConfig_; }
  Digest ShaderHash() const;
  uint64_t VisualGeneration() const { return visualGeneration_; }
  uint64_t VisualTicks() const { return visualTicks_; }
  const CompositeCompletion &Tail() const { return tail_; }
  const accept647::Tail &PublishedTail() const { return publishedTail_; }
  const Input128 &FrozenInput() const { return publishedInput_; }
  const std::array<uint32_t, 128> &FrozenConfig() const {
    return publishedConfig_;
  }
  size_t PacketByteStart() const {
    return parent_ ? parent_->Layout().state[6].begin : 0;
  }
  size_t GlyphByteStart() const {
    return visual_ ? visual_->visual[kSegGlyph] : 0;
  }
  size_t GlyphCount() const { return visual_ ? visual_->v : 0; }
  std::array<uint32_t, 3> VisualBanks() const { return visualBanks_; }
  std::array<uint32_t, 3> ColdVisualBanks() const { return publishedBanks_; }
  size_t ColdSegments(bool full, std::array<GpuSegment, 17> &) const;
  bool Current() const;
  bool Healthy() const { return !firstGl_ && !firstEgl_; }
  const accept647::ActualEvidence &Evidence() const { return evidence_; }
  static const char *PipelineSourceR1();
  static const char *PipelineSourceHashR1();
  static const char *PipelineSourceDm();
  static const char *PipelineSourceHashDm();

private:
  struct Program {
    GLuint id = 0;
    UniformLocations uniforms;
  };
  Program program_;
  std::optional<ParentAdapter> parent_;
  const VisualResources *visual_ = nullptr;
  Scope identity_{};
  Words64 visualConfig_{};
  Digest seedSHA_{};
  std::array<GLuint, 10> buffers_{};
  std::array<GLuint, 6> full_{};
  uint64_t fullCredit_ = 0, fullCreditContext_ = 0;
  std::array<uint32_t, 3> visualBanks_{{0, 1, 2}}, publishedBanks_{{0, 1, 2}};
  PhaseWitness witness_;
  VisualBootstrap bootstrap_;
  BootstrapTicket bootstrapTicket_{};
  CompositePermit permit_{};
  std::optional<accept647::ActiveProgramIdentity> activeIdentity_;
  std::optional<accept647::ExpectedContract> expected_;
  Input128 frozenInput_{}, publishedInput_{};
  std::array<uint32_t, 128> frozenConfig_{}, publishedConfig_{};
  CompositeCompletion tail_{};
  accept647::Tail publishedTail_{};
  accept647::ActualEvidence evidence_{};
  EGLDisplay display_ = EGL_NO_DISPLAY;
  EGLContext context_ = EGL_NO_CONTEXT;
  EGLSurface surface_ = EGL_NO_SURFACE;
  std::thread::id thread_;
  GLsync fence_ = nullptr;
  Check check_ = nullptr;
  void *checkUser_ = nullptr;
  uint64_t visualGeneration_ = 0, visualTicks_ = 0;
  uint32_t next_ = 0, firstGl_ = 0, firstEgl_ = 0, publishedArena_ = 0;
  bool ready_ = false, busy_ = false, mapped_ = false, published_ = false;
  bool Good();
  bool Allowed() const;
  Result Denied() const;
  void U(const char *, uint32_t);
  void I(const char *, int32_t);
  void P(const char *, uint64_t);
  bool Copy(GLuint, size_t, GLuint, size_t, size_t);
  void Common(const Input128 &);
  void Bind(const G11Pass &);
  bool Dispatch(const G11Pass &, uint32_t);
  accept647::ProgramFacts Facts() const;
};
} // namespace grs::g11vis
