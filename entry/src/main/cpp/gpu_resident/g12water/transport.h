// T576 (problem T567): G12 water-SFP native transport.
// Independent profile G12_POROUS_SPF_Q20_V1: state tail carries
// waterHeader64/waterColumn16/waterOrigin4/waterSource16/waterPacket32;
// WORK appends the wet segments per the four resource rows. Dispatch runs
// the g12water pipeline tables (SOURCE16 / WET 6204 / DRY 2404).
#pragma once
#include "contract_generated.h"
#include "../g9spf/completion_gate.h"
#include "../g9spf/control_types.h"
#include "../g9spf/stage_capability.h"
#include "../g9spf/research_config.h"
#include "../uniform_locations.h"
#include <GLES3/gl3.h>
#include <EGL/egl.h>
#include <array>
#include <thread>
#if __has_include("g12water_pipeline.h")
#include "g12water_pipeline.h"
#else
struct G12Pass{uint32_t op;uint32_t role,j;int32_t micro;};
#endif
namespace grs::g12water {
using Result = g9spf::Result;
using Input128 = g9spf::Input128;
using Completion256 = g9spf::Completion256;
// MaterialConfig128: words 0..63 frozen parent bytes; 64..127 G12 section.
struct MaterialConfig128 { std::array<uint32_t, 128> words{}; };
struct SeedDescriptor {
  std::array<uint32_t, 8> recipeSha{};
  std::vector<uint32_t> bytes;   // exact seed raw bytes (U32 words)
};
struct G12Permit {
  g9spf::ControlIdentity identity{};
  uint64_t nonce = 0, plan = 0, frame = 0, deadline = 0, baseCommit = 0;
  uint32_t kind = 0, baseBank = 0, candidateBank = 1, arena = 0;
  uint32_t wet = 0;   // global BASE water inventory mode
  std::array<uint32_t, 8> parameterSha{}, inputSha{};
  bool operator==(const G12Permit& o) const {
    return identity == o.identity && nonce == o.nonce && plan == o.plan && frame == o.frame && deadline == o.deadline && baseCommit == o.baseCommit && kind == o.kind && baseBank == o.baseBank && candidateBank == o.candidateBank && arena == o.arena && wet == o.wet && parameterSha == o.parameterSha && inputSha == o.inputSha;
  }
};
class Transport {
 public:
  Result Create(uint32_t k, uint32_t kw, const MaterialConfig128&, const SeedDescriptor&, const g9spf::ControlIdentity&);
  Result QueryComputeCaps(const g9spf::ProfileRequirements&, g9spf::StageCapabilityV2*);
  Result Begin(const G12Permit&, const Input128&, g9spf::PermitCheck, void*);
  Result Pump();
  bool Commit(const G12Permit&, const Completion256&);
  bool RetireRejected(const G12Permit&);
  void Release(bool current);
  bool Busy() const { return busy_; }
  uint32_t CurrentBank() const { return current_; }
  GLuint StateBuffer(uint32_t bank) const { return bank < 5 ? buffers_[bank] : 0; }
  const Profile* SelectedProfile() const { return profile_; }
  const g9spf::NativeEvidence& Evidence() const { return evidence_; }
  const Completion256& Tail() const { return tail_; }
  static const char* PipelineSource();
  static const char* PipelineSourceHash();
 private:
  struct Program { GLuint id = 0; UniformLocations uniforms; };
  Program program_;
  std::array<GLuint, 10> buffers_{};
  const Profile* profile_ = nullptr;
  G12Permit permit_{};
  MaterialConfig128 config_{};
  SeedDescriptor seed_{};
  g9spf::ControlIdentity identity_{};
  EGLDisplay display_ = EGL_NO_DISPLAY; EGLContext context_ = EGL_NO_CONTEXT; EGLSurface surface_ = EGL_NO_SURFACE;
  std::thread::id thread_;
  GLsync fence_ = nullptr;
  Completion256 tail_{};
  g9spf::NativeEvidence evidence_{};
  uint32_t next_ = 0, current_ = 0, firstGl_ = 0, firstEgl_ = 0, profileIndex_ = 0;
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
  bool Dispatch(const G12Pass&, uint32_t index);
  void Bind(const G12Pass&);
  void Common(const Input128&);
};
}
