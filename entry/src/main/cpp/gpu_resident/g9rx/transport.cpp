#include "transport.h"
#if defined(GRS_G9_BOOT_ONLY) || defined(GRS_G9_BOOT_SOURCE)
#include "../g9_boot.h"
#endif
#ifdef GRS_G9_BOOT_SOURCE
#include "boot_source.h"
#include "source_shader.h"
#endif
#include "../angles_generated.h"
#include "../program_cache/program_factory.h"
#include "implementation_generated.h"
#include "pipeline_source.h"
#include "seed_generated.h"
#include <chrono>
#include <cstring>
#include <limits>
namespace grs::g9rx {
namespace {
#ifdef GRS_G9_BOOT_SOURCE
bool ReflectSource(GLuint program, UniformLocations &locations) {
  if (!locations.Prepare(program) || locations.count != std::size(kSourceUniforms))
    return false;
  uint64_t seen = 0;
  for (GLuint i = 0; i < locations.count; ++i) {
    char name[96]{};
    GLenum type = 0;
    GLint size = 0;
    glGetActiveUniform(program, i, sizeof(name), nullptr, &size, &type, name);
    for (size_t j = 0; j < std::size(kSourceUniforms); ++j)
      if (size == 1 && type == kSourceUniforms[j].type &&
          std::strcmp(name, kSourceUniforms[j].name) == 0 && locations.Find(name) >= 0)
        seen |= uint64_t(1) << j;
  }
  if (seen != (uint64_t(1) << std::size(kSourceUniforms)) - 1)
    return false;
  constexpr const char *blocks[]{"ReadState", "WriteState", "Workspace", "EventPath"};
  GLint lanes[3]{}, count = 0;
  glGetProgramiv(program, GL_COMPUTE_WORK_GROUP_SIZE, lanes);
  glGetProgramInterfaceiv(program, GL_SHADER_STORAGE_BLOCK, GL_ACTIVE_RESOURCES, &count);
  if (lanes[0] != 64 || lanes[1] != 1 || lanes[2] != 1 || count != 4)
    return false;
  for (GLint binding = 0; binding < count; ++binding) {
    const auto index = glGetProgramResourceIndex(program, GL_SHADER_STORAGE_BLOCK, blocks[binding]);
    if (index == GL_INVALID_INDEX)
      return false;
    constexpr GLenum property = GL_BUFFER_BINDING;
    GLint actual = -1;
    glGetProgramResourceiv(program, GL_SHADER_STORAGE_BLOCK, index, 1, &property, 1, nullptr, &actual);
    if (actual != binding)
      return false;
  }
  return true;
}
#endif
std::array<uint32_t, 8> ParseHash(const char *hex) {
  std::array<uint32_t, 8> result{};
  auto digit = [](char c) {
    return uint32_t(c <= '9' ? c - '0' : c - 'a' + 10);
  };
  for (size_t i = 0; i < 32; ++i) {
    uint32_t byte = (digit(hex[2 * i]) << 4) | digit(hex[2 * i + 1]);
    result[i / 4] |= byte << (8 * (i % 4));
  }
  return result;
}
} // namespace
bool Transport::Current() const {
  return context_ != EGL_NO_CONTEXT && thread_ == std::this_thread::get_id() &&
         eglGetCurrentContext() == context_ &&
         eglGetCurrentDisplay() == display_ &&
         eglGetCurrentSurface(EGL_DRAW) == surface_;
}
bool Transport::Good() {
  const auto gl = glGetError();
  const auto egl = eglGetError();
#ifdef GRS_G9_BOOT_SOURCE
  source_.gl = gl;
  source_.egl = egl;
#endif
  if (gl && !firstGl_)
    firstGl_ = gl;
  if (egl != EGL_SUCCESS && !firstEgl_)
    firstEgl_ = egl;
  return !firstGl_ && !firstEgl_;
}
bool Transport::ReadNow(CheckedTime &value) const {
  if (clockFailed_)
    return false;
  value = CheckedClockRead();
  if (!value.ok) {
    clockFailed_ = true;
    return false;
  }
  return true;
}
bool Transport::Allowed() const {
  CheckedTime now;
  return Current() && ReadNow(now) && now.ns >= rootBeginNs_ &&
         now.ns <= permit_.deadline && check_ &&
         check_(checkUser_, permit_, now.ns);
}
Result Transport::Denied() const {
  if (!Current())
    return Result::CONTEXT_LOST;
  CheckedTime now;
  if (!ReadNow(now))
    return Result::INPUT_CLOCK_UNVERIFIED;
  return now.ns > permit_.deadline ? Result::DEADLINE : Result::STALE;
}

void Transport::U(const char *n, uint32_t v) {
  glUniform1ui(program_.uniforms.Find(n), v);
}
void Transport::I(const char *n, int32_t v) {
  glUniform1i(program_.uniforms.Find(n), v);
}
void Transport::P(const char *n, uint64_t v) {
  glUniform2ui(program_.uniforms.Find(n), uint32_t(v), uint32_t(v >> 32));
}
void Transport::Quad(const char *n, const uint32_t *p) {
  glUniform4uiv(program_.uniforms.Find(n), 1, p);
}
std::array<uint32_t, 8> Transport::ShaderHash() const {
  return ParseHash(PipelineSourceHash());
}
Result Transport::QueryComputeCaps(const ProfileRequirements &need,
                                   StageCapabilityV2 *out) {
  // T564 (problem T560 搂2): compute-only eligibility. Called only on the cold
  // path (owner context, create/capability export); per-frame code never
  // queries the driver. Vertex/fragment shader-storage block counts are
  // recorded for the SCENE_ATTR domain but do not gate compute.
  if (!out || !Current())
    return Result::CONTEXT_LOST;
  StageCapabilityV2 c{};
  c.context = context_;
  c.route = 2;
  c.buildGeneration = identity_.context;
  int32_t bad = 0;
  bad |= QueryCapabilityI(c, GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS,
                          c.ssboBindings, "COMPUTE", 4);
  bad |= QueryCapabilityI(c, GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS,
                          c.computeBlocks, "COMPUTE", 4);
  bad |= QueryCapabilityI(c, GL_MAX_COMBINED_SHADER_STORAGE_BLOCKS,
                          c.combinedBlocks, "COMPUTE", 4);
  bad |= QueryCapabilityI(c, GL_MAX_COMPUTE_SHARED_MEMORY_SIZE, c.sharedBytes,
                          "COMPUTE", 4096);
  bad |= QueryCapabilityI(c, GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS,
                          c.invocations, "COMPUTE", 64);
  {
    GLint v[3] = {0, 0, 0};
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &v[0]);
    const auto s = int32_t(glGetError());
    c.sizeX = v[0];
    RecordQuery(c, GL_MAX_COMPUTE_WORK_GROUP_SIZE + 0, v[0], s, 64, "COMPUTE");
    bad |= s;
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &v[1]);
    const auto s1 = int32_t(glGetError());
    c.sizeY = v[1];
    RecordQuery(c, GL_MAX_COMPUTE_WORK_GROUP_SIZE + 1, v[1], s1, 1, "COMPUTE");
    bad |= s1;
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &v[2]);
    const auto s2 = int32_t(glGetError());
    c.sizeZ = v[2];
    RecordQuery(c, GL_MAX_COMPUTE_WORK_GROUP_SIZE + 2, v[2], s2, 1, "COMPUTE");
    bad |= s2;
  }
  {
    GLint v = 0;
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT,0,&v);
    const auto s = int32_t(glGetError());
    c.countX = v;
    RecordQuery(c, GL_MAX_COMPUTE_WORK_GROUP_COUNT, v, s, 1, "COMPUTE");
    bad |= s;
  }
  {
    GLint64 v = 0;
    glGetInteger64v(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &v);
    const auto s = int32_t(glGetError());
    c.maxBlockBytes = v;
    RecordQuery(
        c, GL_MAX_SHADER_STORAGE_BLOCK_SIZE, int64_t(v), s,
        int32_t(std::max<size_t>(need.stateBytes,
                                 std::max(need.workBytes, need.arenaBytes))),
        "COMPUTE");
    bad |= s;
  }
  bad |= QueryCapabilityI(c, GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS,
                          c.vertexBlocksQueried, "SCENE_ATTR", 0);
  bad |= QueryCapabilityI(c, GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS,
                          c.fragmentBlocksQueried, "SCENE_ATTR", 0);
  const auto gl = firstGl_;
  *out = c;
  if (bad || gl) {
    out->computeFailure = bad ? "UNSUPPORTED_QUERY" : "PRIOR_GL_ERROR";
    return Result::UNSUPPORTED;
  }
  if (c.ssboBindings < 4 || c.computeBlocks < 4 || c.combinedBlocks < 4 ||
      c.sharedBytes < 4096 || c.invocations < 64 || c.sizeX < 64 ||
      c.countX < 1 ||
      c.maxBlockBytes <
          int64_t(std::max<size_t>(
              need.stateBytes, std::max(need.workBytes, need.arenaBytes)))) {
    out->computeFailure = "COMPUTE_LIMIT";
    return Result::UNSUPPORTED;
  }
  out->computeEligible = true;
  return Result::OK;
}
Result Transport::Create(const Config &config, const ControlIdentity &id,
                         const SeedFileIdentity &seed, SubmitObserver observer) {
  clockFailed_ = false;
  firstGl_ = firstEgl_ = 0;
  if (ready_ || busy_ || !config.Valid() ||
      config.parameterSha != ParameterHash(config) || !id.run || !id.owner ||
      !id.context)
    return Result::CALIBRATION_CONFIG_REQUIRED;
  const auto *p = SelectProfile(config.k);
  if (!p)
    return Result::UNSUPPORTED;
  context_ = eglGetCurrentContext();
  display_ = eglGetCurrentDisplay();
  surface_ = eglGetCurrentSurface(EGL_DRAW);
  thread_ = std::this_thread::get_id();
  if (!Current())
    return Result::CONTEXT_LOST;
  GLint bindings = 0, blocks = 0, shared = 0, lanes = 0, vertex = 0,
        fragment = 0;
  GLint64 maximum = 0;
  glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &bindings);
  glGetIntegerv(GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS, &blocks);
  glGetIntegerv(GL_MAX_COMPUTE_SHARED_MEMORY_SIZE, &shared);
  glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &lanes);
  glGetIntegerv(GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS, &vertex);
  glGetIntegerv(GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS, &fragment);
  glGetInteger64v(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &maximum);
  // T564 (problem T560 搂2): compute init gates only on compute-stage limits.
  // Vertex/fragment shader-storage counts stay recorded (cold capability
  // export) but no longer gate the compute pipeline; the packet vertex stage
  // now uses integer instance attributes instead of a vertex SSBO.
  if (!Good() || bindings < 4 || blocks < 4 || shared < 4096 || lanes < 64 ||
      maximum <= 0 ||
      uint64_t(maximum) <
          std::max(p->arenaBytes, std::max(p->stateBytes, p->workBytes)))
    return Result::UNSUPPORTED;
#ifdef GRS_G9_BOOT_SOURCE
  source_ = {};
  sourceCacheAttempted_ = false;
  source_.ownerTid = uint64_t(syscall(SYS_gettid));
  source_.eglContext = reinterpret_cast<uintptr_t>(context_);
  static const char mappedBootAnchor = 0;
  g9boot::Trace boot{"RX", kBootPipelineSHA, kBootRoleSetSHA, id.run,
                    id.owner, id.context, &mappedBootAnchor};
  if (!g9boot::Build(boot, kBootPipeline, "GNR1|" kRxCoherentRevision "|" kRxImplementationSHA,
                     program_.id, program_.uniforms, firstGl_, firstEgl_, observer)) {
    Release(true);
    return Result::SHADER_FAILED;
  }
  source_.bootProgram = program_.id;
#elif defined(GRS_G9_BOOT_ONLY)
  g9boot::Trace boot{"RX", PipelineSourceHash(), BootRoleSetSHA(), id.run,
                    id.owner, id.context, reinterpret_cast<const void *>(&PipelineSource)};
  if (!g9boot::Build(boot, PipelineSource(), "GNR1|" kRxCoherentRevision "|" kRxImplementationSHA,
                     program_.id, program_.uniforms, firstGl_, firstEgl_, observer)) {
    Release(true);
    return Result::SHADER_FAILED;
  }
#else
  const char* actualSource=PipelineSource();const size_t actualBytes=std::strlen(actualSource);
  if(actualBytes!=kRxCanonicalShaderBytes||programcache::Hash(actualSource,actualBytes)!=PipelineSourceHash()||std::strcmp(PipelineSourceHash(),kPipelineSHA)!=0||std::strcmp(kRxCaptureHygiene,"RX_CAPTURE_SCOPE_HYGIENE_V1")!=0)
    return Result::SOURCE_IDENTITY_MISMATCH;
  GLenum error = 0;
  if (!programcache::Build(1, PipelineSource(), PipelineSourceHash(),
                           "GNR1|" kRxCoherentRevision "|" kRxImplementationSHA,
                           program_.id, error, observer) ||
      !programcache::Reflect(program_.uniforms, program_.id)) {
    if (error && !firstGl_)
      firstGl_ = error;
    Release(true);
    return Result::SHADER_FAILED;
  }
#endif
  profile_ = p;
  config_ = config;
  identity_ = id;
  glGenBuffers(GLsizei(buffers_.size()), buffers_.data());
  for (size_t i = 0; i < buffers_.size(); ++i) {
    size_t bytes = i < 5    ? p->stateBytes
                   : i == 5 ? p->workBytes
                            : p->arenaBytes;
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffers_[i]);
    glBufferData(GL_SHADER_STORAGE_BUFFER, GLsizeiptr(bytes), nullptr,
                 GL_DYNAMIC_COPY);
    if (!Good()) {
      Release(true);
      return Result::GL_FAILED;
    }
  }
  if (config.fullTrialTrace &&
      !fullGpuCredit_.Reserve(reinterpret_cast<uint64_t>(context_),
                              6 * p->arenaBytes)) {
    Release(true);
    return Result::WOULD_BLOCK;
  }
  if (config.fullTrialTrace) {
    glGenBuffers(GLsizei(full_.size()), full_.data());
    for (auto buffer : full_) {
      glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);
      glBufferData(GL_SHADER_STORAGE_BUFFER, GLsizeiptr(p->arenaBytes), nullptr,
                   GL_DYNAMIC_COPY);
      if (!Good()) {
        Release(true);
        return Result::GL_FAILED;
      }
    }
  }
  // Cold raw seed upload is an input file, not a host simulation. The first
  // GPU identity/map/mass audit validates all supplied seed words before use.
  std::vector<uint32_t> state(p->stateBytes / 4);
  if (!FillTemplate(state.data(), state.size(), config.k)) {
    Release(true);
    return Result::INVALID;
  }
  if (HashWords(state.data(), state.size()) !=
      Digest{config.capsule64[48], config.capsule64[49], config.capsule64[50],
             config.capsule64[51], config.capsule64[52], config.capsule64[53],
             config.capsule64[54], config.capsule64[55]}) {
    Release(true);
    return Result::INVALID;
  }
  Put(state.data(), 4, id.run);
  Put(state.data(), 6, id.owner);
  Put(state.data(), 8, id.context);
  Put(state.data(), 14, id.inputEpoch);
  std::copy(config.parameterSha.begin(), config.parameterSha.end(),
            state.begin() + 42);
  config_.seedSha = HashWords(state.data(), state.size());
  for (size_t i = 0; i < 5; ++i) {
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffers_[i]);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, GLsizeiptr(p->stateBytes),
                    state.data());
  }
  if (!Good()) {
    Release(true);
    return Result::GL_FAILED;
  }
  // Read-only physical warmup: ROOT_INIT writes only private WORK controls.
  glUseProgram(program_.id);
  std::array<uint32_t, 128> initialConfig{};
  initialConfig[0] = config.muStart;
  initialConfig[1] = config.muStop;
  initialConfig[2] = config.band;
  initialConfig[3] = config.entrainRate;
  initialConfig[4] = config.shear;
  initialConfig[5] = config.settleTicks;
  initialConfig[6] = config.revision;
  initialConfig[7] = config.g;
  std::copy(config.capsule64.begin(), config.capsule64.end(),
            initialConfig.begin() + 64);
  std::copy(config_.parameterSha.begin(), config_.parameterSha.end(),
            initialConfig.begin() + 16);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffers_[5]);
  glBufferSubData(GL_SHADER_STORAGE_BUFFER, profile_->work[9].begin, 512,
                  initialConfig.data());
  Input128 warm{};
  permit_.identity = id;
  Common(warm, {}, {});
  Bind(kQuantumPasses[0]);
  U("uOp", uint32_t(Op::ROOT_INIT));
  U("uPass", 0);
#if defined(GRS_G9_BOOT_ONLY) || defined(GRS_G9_BOOT_SOURCE)
  const auto audit = g9boot::RunAudit<CheckedTime>(
      boot, program_.id, program_.uniforms.Find("uOp"), buffers_[5],
      profile_->work[7].begin, [this](CheckedTime &time) { return ReadNow(time); },
      firstGl_, firstEgl_);
#ifdef GRS_G9_BOOT_SOURCE
  if (audit == g9boot::Audit::Ok) {
    const auto *shader = PipelineSource();
    const size_t bytes = std::strlen(shader);
    if (!bytes || bytes > g9boot::kSourceLimit ||
        programcache::Hash(shader, bytes) != PipelineSourceHash()) {
      Release(true);
      return Result::SOURCE_IDENTITY_MISMATCH;
    }
    const std::string key = "GNR1|" kRxCoherentRevision "|" kRxImplementationSHA
                            "|ROLE=SOURCE|ROLE_SET=" + std::string(SourceRoleSetSHA());
    GLenum error = GL_NO_ERROR;
    Program source{};
    OH_LOG_Print(LOG_APP, LOG_INFO, 0xE734, "G9RxSource",
                 "SOURCE_BUILD_ENTER run=%{public}llu bytes=%{public}zu sha=%{public}s roles=%{public}s",
                 static_cast<unsigned long long>(id.run), bytes, PipelineSourceHash(), SourceRoleSetSHA());
    const bool built = programcache::Build(1, shader, PipelineSourceHash(), key.c_str(),
                                           source.id, error, observer, 0, "G9_SOURCE");
    const bool reflected = built && ReflectSource(source.id, source.uniforms);
    const bool good = Good();
    OH_LOG_Print(LOG_APP, LOG_INFO, 0xE734, "G9RxSource",
                 "SOURCE_REFLECT_EXIT program=%{public}u activeUniforms=%{public}u qualified=%{public}d",
                 source.id, source.uniforms.count, reflected && good && error == GL_NO_ERROR);
    if (!reflected || !good || error) {
      if (error && !firstGl_)
        firstGl_ = error;
      if (source.id) {
        programcache::Forget(source.id);
        glDeleteProgram(source.id);
      }
      Release(true);
      return Result::SHADER_FAILED;
    }
    programcache::Forget(program_.id);
    glUseProgram(0);
    glDeleteProgram(program_.id);
    program_ = source;
    if (!Good()) {
      Release(true);
      return Result::GL_FAILED;
    }
    source_.program = source.id;
    current_ = 0;
    ready_ = true;
    return Result::OK;
  }
#endif
  Release(true);
  switch (audit) {
  case g9boot::Audit::Ok: return Result::BOOT_AUDIT_OK;
  case g9boot::Audit::Clock: return Result::INPUT_CLOCK_UNVERIFIED;
  case g9boot::Audit::Gl: return Result::GL_FAILED;
  case g9boot::Audit::Rejected: return Result::INVALID;
  }
  return Result::INVALID;
#else
  CheckedTime begin;
  if (!ReadNow(begin)) {
    Release(true);
    return Result::INPUT_CLOCK_UNVERIFIED;
  }
  glDispatchCompute(1, 1, 1);
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
  U("uOp", 0x7fffff00u);
  glDispatchCompute(1, 1, 1);
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
  glFinish();
  CheckedTime end;
  if (!ReadNow(end)) {
    Release(true);
    return Result::INPUT_CLOCK_UNVERIFIED;
  }
  const auto errorWarm = glGetError();
  programcache::Warmup(program_.id, begin.ns, end.ns, errorWarm);
  if (errorWarm || programcache::Prepared(program_.id) != GL_NO_ERROR ||
      !Good()) {
    if (errorWarm && !firstGl_)
      firstGl_ = errorWarm;
    Release(true);
    return Result::GL_FAILED;
  }
  glBindBuffer(GL_COPY_READ_BUFFER, buffers_[5]);
  const auto *init = static_cast<const uint32_t *>(glMapBufferRange(
      GL_COPY_READ_BUFFER, profile_->work[7].begin, 8, GL_MAP_READ_BIT));
  if (!init) {
    Release(true);
    return Result::GL_FAILED;
  }
  bool audited = init[0] == 0 && init[1] == 0x641647;
  bool unmapped = glUnmapBuffer(GL_COPY_READ_BUFFER) == GL_TRUE;
  if (!audited || !unmapped || !Good()) {
    Release(true);
    return Result::INVALID;
  }
  current_ = 0;
  ready_ = true;
  return Result::OK;
#endif
}
bool Transport::Copy(GLuint from, size_t fromWord, GLuint to, size_t toWord,
                     size_t words) {
  if (!Allowed() || from == to || fromWord > SIZE_MAX / 4 ||
      toWord > SIZE_MAX / 4 || words > SIZE_MAX / 4)
    return false;
  glBindBuffer(GL_COPY_READ_BUFFER, from);
  glBindBuffer(GL_COPY_WRITE_BUFFER, to);
  glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER,
                      GLintptr(fromWord * 4), GLintptr(toWord * 4),
                      GLsizeiptr(words * 4));
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
#ifdef GRS_G9_BOOT_SOURCE
  ++source_.copies;
#endif
  return Good() && Allowed();
}
void Transport::Common(const Input128 &input,
                       const std::array<uint32_t, 8> &layout,
                       const std::array<uint32_t, 8> &shader) {
  const auto &p = *profile_;
  U("uK", p.k);
  U("uStateWords", uint32_t(p.stateBytes / 4));
  U("uColumn", uint32_t(p.state[3].begin / 4));
  U("uBed", uint32_t(p.state[4].begin / 4));
  U("uFlow", uint32_t(p.state[5].begin / 4));
  U("uPacket", uint32_t(p.state[6].begin / 4));
  U("uSources", uint32_t(p.state[7].begin / 4));
  const char *names[] = {
      "uFaces",      "uColumnStage",     "uHamilton",       "uOriginAudit",
      "uIndexes",    "uInput",           "uSummary",        "uControls",
      "uCompletion", "uConfig",          "uRootExtension",  "uWideLedger",
      "uContacts",   "uOriginExtension", "uStageDiagnostic"};
  for (size_t i = 0; i < 15; ++i)
    U(names[i], uint32_t(p.work[i].begin / 4));
  P("uNonce", permit_.nonce);
  P("uRun", permit_.identity.run);
  P("uOwner", permit_.identity.owner);
  P("uContext", permit_.identity.context);
  P("uSurface", permit_.identity.surface);
  P("uGeometry", permit_.geometry);
  P("uConfigId", permit_.identity.config);
  P("uInputGeneration", permit_.identity.inputEpoch);
  P("uSession", permit_.identity.pageSession);
  P("uPlan", permit_.plan);
  P("uBaseCommit", permit_.baseCommit);
  P("uArenaGeneration", permit_.arenaGeneration);
  U("uCandidateBank", permit_.candidateBank);
  U("uBaseBank", permit_.baseBank);
  U("uArena", permit_.arena);
  U("uRootKind", permit_.kind);
  U("uRequiredPasses", permit_.kind == 1 ? 12 : 2404);
  U("uRequiredAudits", permit_.kind == 1 ? 65535u : 262143u);
  U("uDmBase", uint32_t((p.workBytes - 24576) / 4));
  U("uEventCapacity", p.eventCapacity);
  U("uPathCapacity", p.pathCapacity);
  U("uPathWord", p.eventCapacity * 64);
  const auto angle = int32_t(input[24]);
  const auto at = size_t(angle + 7680) * 4;
  I("uAcceptedAngle", angle);
  I("uAngleSin", int32_t(kAngleWords[at + 1]));
  I("uAngleCos", int32_t(kAngleWords[at + 2]));
  (void)layout;
  (void)shader;
}
Result Transport::Begin(const Permit &p, const Input128 &input, Check check,
                        void *user) {
#ifdef GRS_G9_BOOT_SOURCE
  if (p.kind != 1 || p.ordinal || input[40] != 1 || p.shaderSha != ShaderHash())
    return Result::INVALID;
#endif
  if (!ready_ || busy_ || !Current())
    return Result::WOULD_BLOCK;
  if (p.baseBank != current_ || p.candidateBank >= 5 ||
      p.baseBank == p.candidateBank || p.kind < 1 || p.kind > 2 ||
      int32_t(input[24]) < -7680 || int32_t(input[24]) > 7680 || !check)
    return Result::INVALID;
  if (clockFailed_ && p.clockGeneration > qualifiedClockGeneration_) {
    auto now = CheckedClockRead();
    auto resolution = ClockResolution();
    if (now.ok && resolution.ok && RoundingQualified())
      clockFailed_ = false;
  }
  qualifiedClockGeneration_ = p.clockGeneration;
  rootBeginNs_ = p.rootBeginNs;
  CheckedTime begin;
  if (!ReadNow(begin))
    return Result::INPUT_CLOCK_UNVERIFIED;
  if (begin.ns < rootBeginNs_ || begin.ns > p.deadline)
    return Result::DEADLINE;
  permit_ = p;
#ifdef GRS_G9_BOOT_SOURCE
  source_.permit = p;
  source_.candidateObject = buffers_[p.candidateBank];
#endif
  check_ = check;
  checkUser_ = user;
  if (!Allowed())
    return Denied();
  std::array<uint32_t, 128> config{};
  config[0] = config_.muStart;
  config[1] = config_.muStop;
  config[2] = config_.band;
  config[3] = config_.entrainRate;
  config[4] = config_.shear;
  config[5] = config_.settleTicks;
  config[6] = config_.revision;
  config[7] = config_.g;
  for (size_t i = 0; i < 8; ++i) {
    config[16 + i] = p.parameterSha[i];
    config[24 + i] = p.layoutSha[i];
    config[32 + i] = p.shaderSha[i];
    config[40 + i] = p.inputSha[i];
    config[48 + i] = config_.seedSha[i];
  }
  std::copy(config_.capsule64.begin(), config_.capsule64.end(),
            config.begin() + 64);
  lastInput_ = input;
  lastConfig_ = config;
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffers_[5]);
  glBufferSubData(GL_SHADER_STORAGE_BUFFER, GLintptr(profile_->work[5].begin),
                  512, input.data());
  glBufferSubData(GL_SHADER_STORAGE_BUFFER, GLintptr(profile_->work[9].begin),
                  512, config.data());
  glUseProgram(program_.id);
  Common(input, p.layoutSha, p.shaderSha);
#ifdef GRS_G9_BOOT_SOURCE
  GLint current = 0;
  glGetIntegerv(GL_CURRENT_PROGRAM, &current);
  source_.currentProgram = current;
  if (GLuint(current) != source_.program)
    return Result::GL_FAILED;
#endif
  if (!Good())
    return Result::GL_FAILED;
  evidence_ = {};
  tail_ = {};
  next_ = 0;
  busy_ = true;
  mapped_ = false;
  return Result::OK;
}
void Transport::Bind(const Pass &pass) {
  GLuint read = buffers_[1], write = buffers_[permit_.candidateBank],
         events = buffers_[6];
  if (pass.op == Op::BASE_FREEZE || pass.op == Op::SOURCE_VALIDATE_INPUT) {
    read = buffers_[current_];
    write = permit_.kind == 1 ? buffers_[permit_.candidateBank] : buffers_[1];
  }
  if (pass.role == 3 || pass.role == 4)
    write = buffers_[4];
  if (pass.op == Op::FINAL_ENERGY_CURSORS &&
      (pass.role == 2 || pass.role == 4)) {
    read = buffers_[7];
    write = buffers_[6];
    events = buffers_[1];
  }
  if (pass.op == Op::SCOPE_SEAL_AND_CHOICE && pass.role == 4) {
    read = buffers_[permit_.candidateBank];
    write = buffers_[4];
  }
  if (pass.op == Op::COMPARE_SELECTED_AUTHORITATIVE) {
    read = buffers_[3];
    write = buffers_[permit_.candidateBank];
  }
  if (pass.op == Op::SOURCE_AUDIT_REPLAY) {
    read = buffers_[current_];
    write = buffers_[4];
  }
  if (pass.op == Op::SOURCE_COMPARE) {
    read = buffers_[permit_.candidateBank];
    write = buffers_[4];
  }
  if (pass.op == Op::SOURCE_SEAL || pass.op == Op::ROOT_SEAL) {
    read = buffers_[1];
    write = buffers_[permit_.candidateBank];
  }
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, read);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, write);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, buffers_[5]);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, events);
#ifdef GRS_G9_BOOT_SOURCE
  if (permit_.kind == 1 && next_ < source_.passes.size())
    source_.passes[next_].objects = {read, write, buffers_[5], events};
#endif
}
bool Transport::Dispatch(const Pass &pass, uint32_t index) {
  if (!Allowed())
    return false;
  // BASE is frozen once; every trial starts from this exact complete copy.
  if (permit_.kind == 1 && index == 0 &&
      !Copy(buffers_[current_], 0, buffers_[1], 0, profile_->stateBytes / 4))
    return false;
  if (pass.op == Op::DISCOVERY_COMMIT_SELECTION) {
    if (!Copy(buffers_[permit_.candidateBank], 0, buffers_[3], 0,
              profile_->stateBytes / 4) ||
        !Copy(buffers_[6], 0, buffers_[7], 0, profile_->arenaBytes / 4))
      return false;
  }
  Bind(pass);
  U("uOp", uint32_t(pass.op));
  U("uRole", pass.role);
  U("uJslot", pass.j);
  U("uMicro", uint32_t(pass.micro));
  U("uPass", index);
  if (!Allowed())
    return false;
  glDispatchCompute(1, 1, 1);
  ++evidence_.dispatches;
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
  ++evidence_.barriers;
#ifdef GRS_G9_BOOT_SOURCE
  auto &entry = source_.passes[index];
  entry.op = uint32_t(pass.op);
  entry.role = pass.role;
  entry.j = pass.j;
  entry.micro = uint32_t(pass.micro);
  source_.dispatches = evidence_.dispatches;
  source_.barriers = evidence_.barriers;
#endif
  if (!Good())
    return false;
  if (config_.fullTrialTrace && pass.role == 1 &&
      pass.op == Op::SCOPE_SEAL_AND_CHOICE) {
    uint32_t trial = 0;
    for (uint32_t j = pass.j; j > 1; j >>= 1)
      ++trial;
    if (!Copy(buffers_[6], 0, full_[trial], 0, profile_->arenaBytes / 4))
      return false;
  }
  if (pass.op == Op::FINAL_ENERGY_CURSORS && pass.role == 2) {
    if (!Copy(buffers_[6], 0, buffers_[8 + permit_.arena], 0,
              profile_->arenaBytes / 4))
      return false;
  }
  if (pass.op == Op::SOURCE_COMPARE) {
    if (!Copy(buffers_[6], 0, buffers_[7], 0, profile_->arenaBytes / 4) ||
        !Copy(buffers_[6], 0, buffers_[8 + permit_.arena], 0,
              profile_->arenaBytes / 4))
      return false;
  }
  return Allowed();
}
Result Transport::Pump() {
  if (!busy_)
    return Result::INVALID;
  if (!Allowed())
    return Denied();
  if (mapped_)
    return Result::OK;
  const uint32_t count = permit_.kind == 1 ? 12 : 2404;
  if (!fence_) {
    CheckedTime begin;
    if (!ReadNow(begin))
      return Result::INPUT_CLOCK_UNVERIFIED;
    uint32_t submitted = 0;
    while (next_ < count && submitted < 10) {
      CheckedTime now;
      if (!ReadNow(now) || now.ns < begin.ns)
        return Result::INPUT_CLOCK_UNVERIFIED;
      if (now.ns - begin.ns >= 8000000)
        break;
      const auto &pass =
          permit_.kind == 1 ? kSourcePasses[next_] : kQuantumPasses[next_];
      if (!Dispatch(pass, next_))
        return !Allowed() ? Denied() : Result::GL_FAILED;
      ++next_;
      ++submitted;
    }
    if (!Allowed())
      return Denied();
    glFlush();
    if (!Good())
      return Result::GL_FAILED;
    if (next_ != count)
      return Result::IN_PROGRESS;
    if (!Allowed())
      return Denied();
    fence_ = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
#ifdef GRS_G9_BOOT_SOURCE
    source_.fenceCreated = fence_ != nullptr;
#endif
    glFlush();
    if (!fence_ || !Good())
      return Result::GL_FAILED;
  }
  if (!Allowed())
    return Denied();
  const auto result = glClientWaitSync(fence_, 0, 0);
#ifdef GRS_G9_BOOT_SOURCE
  source_.rawWait = result;
  ++source_.fencePolls;
  if (result == GL_WAIT_FAILED) {
    Good();
    return Result::GL_FAILED;
  }
#endif
  if (result == GL_WAIT_FAILED || !Good())
    return Result::GL_FAILED;
  if (result == GL_TIMEOUT_EXPIRED)
    return Result::IN_PROGRESS;
#ifdef GRS_G9_BOOT_SOURCE
  if (result != GL_ALREADY_SIGNALED && result != GL_CONDITION_SATISFIED)
    return Result::GL_FAILED;
#endif
  CheckedTime fenceTime;
  if (!ReadNow(fenceTime))
    return Result::INPUT_CLOCK_UNVERIFIED;
  evidence_.fenceNs = fenceTime.ns;
  evidence_.fenceObserved = true;
#ifdef GRS_G9_BOOT_SOURCE
  source_.fenceNs = fenceTime.ns;
#endif
  if (!Allowed())
    return Denied();
  glBindBuffer(GL_COPY_READ_BUFFER, buffers_[5]);
  const auto offset = profile_->work[8].begin;
#ifdef GRS_G9_BOOT_SOURCE
  source_.work = buffers_[5];
  source_.workOffset = offset;
#endif
  auto *mapped = glMapBufferRange(GL_COPY_READ_BUFFER, GLintptr(offset), 1024,
                                  GL_MAP_READ_BIT);
#ifdef GRS_G9_BOOT_SOURCE
  source_.mapReturned = mapped != nullptr;
#endif
  if (!mapped) {
#ifdef GRS_G9_BOOT_SOURCE
    Good();
#endif
    return Result::GL_FAILED;
  }
  std::memcpy(tail_.data(), mapped, 1024);
  const bool unmapped = glUnmapBuffer(GL_COPY_READ_BUFFER) == GL_TRUE;
#ifdef GRS_G9_BOOT_SOURCE
  source_.unmapped = unmapped;
  if (!unmapped) {
    Good();
    return Result::GL_FAILED;
  }
#endif
  if (!unmapped || !Good())
    return Result::GL_FAILED;
  glDeleteSync(fence_);
  fence_ = nullptr;
  evidence_.gl = firstGl_;
  evidence_.egl = firstEgl_;
  mapped_ = true;
#ifdef GRS_G9_BOOT_SOURCE
  source_.mapped = true;
  source_.tail = tail_;
#endif
  return Result::OK;
}
bool Transport::CommitAccepted(const ValidatedRecord &proof) {
  // The proof was created on this owner's still-current mapped flight; no GL
  // calls here, so the controller can atomically promote the FIFO and banks.
  CheckedTime commitTime;
  if (!ReadNow(commitTime))
    return false;
  const auto &p = proof.Permission();
  const auto &w = proof.Words();
  if (!proof.ProducedBy(this) || !busy_ || !mapped_ ||
      thread_ != std::this_thread::get_id() || !(p == permit_) || w != tail_ ||
      !check_ || !check_(checkUser_, p, commitTime.ns))
    return false;
#ifdef GRS_G9_BOOT_SOURCE
  if (!Current() || p.kind != 1 || program_.id != source_.currentProgram ||
      p.shaderSha != ShaderHash())
    return false;
#endif
  if (w[6] != 2)
    std::swap(buffers_[current_], buffers_[p.candidateBank]);
  commitObservedNs_ = commitTime.ns;
#ifdef GRS_G9_BOOT_SOURCE
  source_.commitNs = commitTime.ns;
  source_.committedObject = buffers_[current_];
#endif
  busy_ = mapped_ = false;
  return true;
}
bool Transport::Commit(const Permit &p, const Completion256 &w) {
  auto proof = AcceptedProof();
  return proof && p == proof->Permission() && w == proof->Words() &&
         CommitAccepted(*proof);
}
#ifdef GRS_G9_BOOT_SOURCE
void Transport::PrepareSourceCache() {
  if (sourceCacheAttempted_ || !ready_ || busy_ || !Current() || !commitObservedNs_ ||
      permit_.kind != 1 || tail_[6] != 1 || !evidence_.fenceObserved)
    return;
  sourceCacheAttempted_ = true;
  const auto result = programcache::Prepared(program_.id);
  if (result == GL_NO_ERROR)
    programcache::Warmup(program_.id, rootBeginNs_, evidence_.fenceNs, GL_NO_ERROR);
}
#endif

bool Transport::RetireRejected(const Permit &p) {
  if (!busy_)
    return true;
  if (!(p == permit_) || !Current())
    return false;
  if (!mapped_) {
    if (!fence_) {
      fence_ = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
      glFlush();
      return false;
    }
    const auto result = glClientWaitSync(fence_, 0, 0);
    if (result != GL_ALREADY_SIGNALED && result != GL_CONDITION_SATISFIED)
      return false;
    glDeleteSync(fence_);
    fence_ = nullptr;
  }
  busy_ = mapped_ = false;
  return Good();
}
std::optional<ValidatedRecord> Transport::AcceptedProof() const {
  CheckedTime now;
  if (!ReadNow(now))
    return {};
  if (!busy_ || !mapped_ || !Current() || !check_ ||
      !check_(checkUser_, permit_, now.ns) || evidence_.fenceNs < rootBeginNs_)
    return {};
  return CompletionGate::Accepted(
      FrozenExpectedView(*profile_, permit_, lastInput_, config_.seedSha,
                         ShaderHash(), this),
      tail_, evidence_);
}
std::optional<ValidatedBlockedControlRecord> Transport::BlockedProof() const {
  CheckedTime now;
  if (!ReadNow(now))
    return {};
  if (!busy_ || !mapped_ || !Current() || !check_ ||
      !check_(checkUser_, permit_, now.ns) || evidence_.fenceNs < rootBeginNs_)
    return {};
  return CompletionGate::Blocked(FrozenExpectedView(*profile_, permit_,
                                                    lastInput_, config_.seedSha,
                                                    ShaderHash(), this),
                                 tail_, evidence_);
}
bool Transport::CaptureBuffer(uint32_t *dst, size_t words, uint32_t kind,
                              uint32_t index) {
  if (!dst || !Current() || busy_)
    return false;
  GLuint b = kind == 0   ? (index < 5 ? buffers_[index] : 0)
             : kind == 1 ? (index < 4 ? buffers_[6 + index] : 0)
                         : (index < 6 ? full_[index] : 0);
  if (!b)
    return false;
  size_t exact =
      kind == 0 ? profile_->stateBytes / 4 : profile_->arenaBytes / 4;
  if (words != exact)
    return false;
  glBindBuffer(GL_COPY_READ_BUFFER, b);
  const auto *raw = static_cast<const uint32_t *>(
      glMapBufferRange(GL_COPY_READ_BUFFER, 0, exact * 4, GL_MAP_READ_BIT));
  if (!raw)
    return false;
  std::memcpy(dst, raw, exact * 4);
  return glUnmapBuffer(GL_COPY_READ_BUFFER) == GL_TRUE && Good();
}
void Transport::Release(bool current) {
  current = current && Current();
  if (program_.id)
    programcache::Forget(program_.id);
  if (current) {
    if (fence_)
      glDeleteSync(fence_);
    glDeleteBuffers(GLsizei(buffers_.size()), buffers_.data());
    glDeleteBuffers(GLsizei(full_.size()), full_.data());
    if (program_.id)
      glDeleteProgram(program_.id);
  }
  if (current)
    fullGpuCredit_.ReleaseAfterFenceDelete();
  else
    fullGpuCredit_.ContextLost();
  buffers_ = {};
  full_ = {};
  program_ = {};
  fence_ = nullptr;
  ready_ = busy_ = mapped_ = false;
  profile_ = nullptr;
  context_ = EGL_NO_CONTEXT;
}
} // namespace grs::g9rx
