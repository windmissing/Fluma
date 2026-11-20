#pragma once
#include "../resource_ledger.h"
#include "pipeline_roles.h"
#include "wire.h"
#include <EGL/egl.h>
#include <GLES3/gl31.h>
#include <thread>
namespace grs::g13 {
class Transport {
  resource501::Ledger ledger_;
  enum Uniform { Op, Scope, TrialJ, Micro, Block, Index, LayoutArray, TestHook };
  struct Locations {
    std::array<GLint, 8> at{};
    GLsizei layoutWords = 0;
    bool requireLayout = false;
    bool Prepare(GLuint);
  };
  struct Program {
    GLuint id = 0;
    Locations uniforms;
  };
  std::array<Program, kMaxPipelineRoles> programs_{};
  EGLContext context_ = EGL_NO_CONTEXT;
  std::thread::id owner_;
  std::array<GLuint, 3> banks_{};
  std::array<GLuint, 2> packs_{};
  GLuint work_ = 0, ctrl_ = 0;
  GLsync fence_ = nullptr;
  GLsync steadyFence_ = nullptr;
  size_t nextSteadyRole_ = 1;
  uint64_t programGeneration_ = 0;
  uint32_t testHook_ = 0;
  GLenum steadyProbeError_ = GL_NO_ERROR;
  std::array<uint32_t, 80> layout_{};
  uint32_t current_ = 0, candidate_ = 1, replay_ = 2, pack_ = 0,
           pendingPack_ = 1;
  size_t pass_ = 0;
  bool source_ = false, submitted_ = false, ready_ = false;
  Permit permit_{};
  Config config_{};
  const Profile *profile_ = nullptr;
  std::string capabilities_, compile_, copyStage_;
  GLenum firstGl_ = 0;
  std::array<uint32_t, 256> seedHeader_{};
  std::string seedReceipt_;
  uint32_t lastBase_ = 0, lastCandidate_ = 1;
  bool aborted_ = false;
  void *guardContext_ = nullptr;
  uint32_t (*guard_)(void *, uint64_t) = nullptr;
  EGLContext retiringContext_ = EGL_NO_CONTEXT;
  size_t retiringBytes_ = 0;
  void Bind(const Pass &, const Locations &);
  bool BuildRole(size_t);
  bool UploadRoleUniforms(size_t);
  bool Owner() const;
  bool CopyBuffer(GLuint, uint32_t *, size_t, size_t = 0,
                  GLenum *diagnosticGl = nullptr);
  uint32_t Allocate(GLuint &, size_t);

public:
  uint32_t Configure(const Config &, const Seed &, const Identity &);
  uint32_t PrepareSteady(bool cancelled);
  void SetGuard(void *context, uint32_t (*guard)(void *, uint64_t)) {
    guardContext_ = context;
    guard_ = guard;
  }
  void ContextRetired(EGLContext context) {
    ledger_.Retired(reinterpret_cast<uintptr_t>(context), 13);
    if (retiringContext_ == context) {
      retiringContext_ = EGL_NO_CONTEXT;
      retiringBytes_ = 0;
    }
  }
  bool RetirementPending() const { return retiringContext_ != EGL_NO_CONTEXT; }
  size_t RetiringBytes() const { return retiringBytes_; }
  uint32_t BeginRoot(const Permit &, const Input &, const Input &,
                     const std::array<uint32_t, 256> &);
  uint32_t PumpBounded(uint64_t now);
  uint32_t Poll(Completion &);
  void Abort();
  void Accept();
  void Retire(bool currentContext);
  bool Idle() const { return !Busy(); }
  bool BootReady() const { return ready_; }
  bool Ready() const { return ready_ && nextSteadyRole_ == PipelineRoleCount(); }
  bool Busy() const { return submitted_ || fence_ || steadyFence_; }
  GLuint Field() const { return banks_[current_]; }
  GLuint Pack() const { return packs_[pack_]; }
  const Profile *Layout() const { return profile_; }
  uint32_t CaptureSpan(bool full, uint32_t *dst, size_t words, size_t &written);
  std::string Resources() {
    auto s = ledger_.Copy();
    std::ostringstream o;
    o << "{\"domain\":\"LOGICAL_REQUEST_G13_DSO\",\"driverRSS\":\"NOT_"
         "MEASURED\",\"liveBytes\":\""
      << s.liveBytes << "\",\"retiringBytes\":\"" << s.retiringBytes
      << "\",\"logicalPeak\":\"" << s.logicalPeak
      << "\",\"incomplete\":" << s.incomplete << "}";
    return o.str();
  }
  const std::array<uint32_t, 256> &InitialHeader() const { return seedHeader_; }
  const Config &Configuration() const { return config_; }
  const Permit &Ticket() const { return permit_; }
  const std::string &Capabilities() const { return capabilities_; }
  const std::string &CompileReceipt() const { return compile_; }
  const std::string &CaptureStage() const { return copyStage_; }
  uint32_t RawGl() const { return firstGl_; }
  uint32_t CurrentBank() const { return current_; }
  uint32_t CapturedBaseBank() const { return lastBase_; }
  uint32_t CapturedCandidateBank() const { return lastCandidate_; }
  uint32_t CandidateBank() const { return candidate_; }
  uint32_t ReplayBank() const { return replay_; }
  uint32_t PendingPack() const { return pendingPack_; }
};
} // namespace grs::g13
