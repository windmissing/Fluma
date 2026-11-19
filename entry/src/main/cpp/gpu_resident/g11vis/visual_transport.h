// T566 (problem T563): G11 composite visual transport.
// Owns the composite pipeline program, the parent-layout state buffers plus
// the extended visual WORK, and the composite dispatch (parent passes with
// VIS ops interleaved per the T563 tables). The parent physics math runs
// unchanged; the visual stage never writes parent q/inventory.
#pragma once
#include "generated_contract.h"
#include "../g9spf/completion_gate.h"
#include "../g9spf/control_types.h"
#include "../g9spf/stage_capability.h"
#include "../g9spf/research_config.h"
#include "../uniform_locations.h"
#include <GLES3/gl3.h>
#include <EGL/egl.h>
#include <array>
#include <thread>
#include "g11vis_pipeline.h"
namespace grs::g11vis {
// Retained legacy R1 signature, never an authorization for CompositeGate.
// Create fails closed until the native seed identity contract is frozen.
struct LegacyCompositePermit {
  g9spf::Permit parent{};
  uint32_t compositeCode = 0;          // G11R / G11D
  uint64_t visualGeneration = 0;
  std::array<uint32_t, 8> visualConfigSha{}, pipelineSha{};
  bool operator==(const LegacyCompositePermit& o) const {
    return parent == o.parent && compositeCode == o.compositeCode && visualGeneration == o.visualGeneration && visualConfigSha == o.visualConfigSha && pipelineSha == o.pipelineSha;
  }
};
struct CompositeIdentity {
  uint32_t parentProfile = 0, compositeProfile = 0;
  std::array<uint32_t, 8> parentParameterSha{}, visualConfigSha{}, visualSeedRecipeSha{};
};
class VisualTransport {
 public:
  using Result = g9spf::Result;
  struct CompositeCompletion {
    std::array<uint32_t, 256> words{};
    static_assert(sizeof(words)==1024,"Completion256 exact tail");
    uint32_t visAuditBits = 0;   // bit18 PLAN_MAPPING, bit19 VIS_REPLAY_GLYPH
  };
  Result Create(uint32_t parentKind, uint32_t k, uint32_t v, const g9spf::Config&, const g9spf::ControlIdentity&, const g9spf::SeedFileIdentity&);
  Result QueryComputeCaps(const g9spf::ProfileRequirements&, g9spf::StageCapabilityV2*);
  Result Begin(const LegacyCompositePermit&, const g9spf::Input128&, g9spf::PermitCheck, void*);
  Result Pump();
  bool Commit(const LegacyCompositePermit&, const CompositeCompletion&);
  bool RetireRejected(const LegacyCompositePermit&);
  void Release(bool current);
  bool Busy() const { return busy_; }
  uint64_t ContextGeneration() const { return identity_.context; }
  bool HasValidatedPublication() const { return false; } // Native seed identity/typed owner not yet wired; raw entry cannot publish.
  size_t PacketByteStart() const { return parentProfile_?parentProfile_->state[6].begin:0; }
  uint32_t CurrentBank() const { return current_; }
  GLuint StateBuffer(uint32_t bank) const { return bank < 5 ? buffers_[bank] : 0; }
  GLuint WorkBuffer() const { return buffers_[5]; }
  const VisualResources* Resources() const { return visual_; }
  const CompositeCompletion& Tail() const { return tail_; }
  const g9spf::NativeEvidence& Evidence() const { return evidence_; }
  // Glyph span for scene consumption: same WORK buffer, glyph segment.
  size_t GlyphByteStart() const { return visual_ ? visual_->visual[kSegGlyph] : 0; }
  size_t GlyphCount() const { return visual_ ? visual_->v : 0; }
  static const char* PipelineSourceR1();
  static const char* PipelineSourceHashR1();
  static const char* PipelineSourceDm();
  static const char* PipelineSourceHashDm();
 private:
  struct Program { GLuint id = 0; UniformLocations uniforms; };
  Program program_;
  std::array<GLuint, 10> buffers_{};
  const g9spf::Profile* parentProfile_ = nullptr;
  const VisualResources* visual_ = nullptr;
  uint32_t parentKind_ = 0;   // 0 = R1, 1 = DM
  g9spf::Config config_;
  g9spf::ControlIdentity identity_{};
  LegacyCompositePermit permit_{};
  EGLDisplay display_ = EGL_NO_DISPLAY; EGLContext context_ = EGL_NO_CONTEXT; EGLSurface surface_ = EGL_NO_SURFACE;
  std::thread::id thread_;
  GLsync fence_ = nullptr;
  CompositeCompletion tail_{};
  g9spf::NativeEvidence evidence_{};
  uint32_t next_ = 0, current_ = 0, firstGl_ = 0, firstEgl_ = 0;
  bool ready_ = false, busy_ = false, mapped_ = false;
  g9spf::PermitCheck check_ = nullptr; void* checkUser_ = nullptr;
  bool Current() const;
  bool Good();
  bool Allowed() const;
  Result Denied() const;
  void U(const char*, uint32_t);
  void I(const char*, int32_t);
  void P(const char*, uint64_t);
  bool Copy(GLuint, size_t, GLuint, size_t, size_t);
  bool Dispatch(const G11Pass&, uint32_t index);
  void Bind(const G11Pass&);
  void Common(const g9spf::Input128&);
};
}
