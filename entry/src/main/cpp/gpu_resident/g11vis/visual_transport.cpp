#include "visual_transport.h"
#include "../angles_generated.h"
#include "../cold_slot_broker.h"
#include "../program_cache/program_factory.h"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <sstream>
namespace grs::g11vis {
namespace {
uint64_t Now() {
  return uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(
                      std::chrono::steady_clock::now().time_since_epoch())
                      .count());
}
uint64_t InitialSequence() {
  static std::mutex mutex;
  static uint64_t n = 0;
  std::lock_guard<std::mutex> lock(mutex);
  return n == UINT64_MAX ? 0 : ++n;
}
template <class P>
void InitialPermit(P &p, const Scope &s, const ParentAdapter &adapter,
                   const Digest &shader) {
  p.identity.run = s.run;
  p.identity.owner = s.owner;
  p.identity.context = s.context;
  p.identity.inputEpoch = s.inputEpoch;
  p.identity.pageSession = s.pageSession;
  p.identity.permission = s.permission;
  p.identity.window = s.window;
  p.identity.config = s.config;
  p.identity.surface = s.surface;
  p.parameterSha = adapter.ParameterHash();
  p.layoutSha = adapter.PhysicalLayoutHash();
  p.shaderSha = shader;
  p.baseBank = 0;
  p.candidateBank = 2;
}
} // namespace
bool VisualTransport::Current() const {
  return context_ != EGL_NO_CONTEXT && thread_ == std::this_thread::get_id() &&
         eglGetCurrentContext() == context_ &&
         eglGetCurrentDisplay() == display_ &&
         eglGetCurrentSurface(EGL_DRAW) == surface_;
}
bool VisualTransport::Good() {
  const auto gl = glGetError();
  const auto egl = eglGetError();
  if (gl && !firstGl_)
    firstGl_ = gl;
  if (egl != EGL_SUCCESS && !firstEgl_)
    firstEgl_ = egl;
  return !firstGl_ && !firstEgl_;
}
bool VisualTransport::Allowed() const {
  const auto now = Now();
  return Current() && now >= permit_.rootBeginNs && now <= permit_.Deadline() &&
         check_ && check_(checkUser_, permit_, now);
}
VisualTransport::Result VisualTransport::Denied() const {
  return !Current()                   ? Result::CONTEXT_LOST
         : Now() > permit_.Deadline() ? Result::DEADLINE
                                      : Result::STALE;
}
void VisualTransport::U(const char *n, uint32_t x) {
  glUniform1ui(program_.uniforms.Find(n), x);
}
void VisualTransport::I(const char *n, int32_t x) {
  glUniform1i(program_.uniforms.Find(n), x);
}
void VisualTransport::P(const char *n, uint64_t x) {
  glUniform2ui(program_.uniforms.Find(n), uint32_t(x), uint32_t(x >> 32));
}
bool VisualTransport::Copy(GLuint from, size_t at, GLuint to, size_t dst,
                           size_t words) {
  if (!from || !to || from == to || at > SIZE_MAX / 4 || dst > SIZE_MAX / 4 ||
      words > SIZE_MAX / 4)
    return false;
  glBindBuffer(GL_COPY_READ_BUFFER, from);
  glBindBuffer(GL_COPY_WRITE_BUFFER, to);
  glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER,
                      GLintptr(at * 4), GLintptr(dst * 4),
                      GLsizeiptr(words * 4));
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
  return Good();
}
Digest VisualTransport::ShaderHash() const {
  return HexDigest(parent_ && parent_->Kind() == ParentKind::DM
                       ? PipelineSourceHashDm()
                       : PipelineSourceHashR1());
}
VisualTransport::Result
VisualTransport::QueryComputeCaps(const g9spf::ProfileRequirements &need,
                                  g9spf::StageCapabilityV2 *out) {
  // Composite compute eligibility mirrors the parent T564 gates: compute-stage
  // limits only; vertex/fragment counts are recorded but do not gate compute.
  if (!out || !Current())
    return Result::CONTEXT_LOST;
  g9spf::StageCapabilityV2 c{};
  c.context = context_;
  c.route = 3;
  c.buildGeneration = identity_.context;
  int32_t bad = 0;
  bad |= g9spf::QueryCapabilityI(c, GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS,
                                 c.ssboBindings, "COMPOSITE", 4);
  bad |= g9spf::QueryCapabilityI(c, GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS,
                                 c.computeBlocks, "COMPOSITE", 4);
  bad |= g9spf::QueryCapabilityI(c, GL_MAX_COMBINED_SHADER_STORAGE_BLOCKS,
                                 c.combinedBlocks, "COMPOSITE", 4);
  bad |= g9spf::QueryCapabilityI(c, GL_MAX_COMPUTE_SHARED_MEMORY_SIZE,
                                 c.sharedBytes, "COMPOSITE", 4096);
  bad |= g9spf::QueryCapabilityI(c, GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS,
                                 c.invocations, "COMPOSITE", 64);
  {
    GLint v[3] = {0, 0, 0};
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &v[0]);
    const auto s = int32_t(glGetError());
    c.sizeX = v[0];
    g9spf::RecordQuery(c, GL_MAX_COMPUTE_WORK_GROUP_SIZE + 0, v[0], s, 64,
                       "COMPOSITE");
    bad |= s;
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &v[1]);
    const auto s1 = int32_t(glGetError());
    c.sizeY = v[1];
    g9spf::RecordQuery(c, GL_MAX_COMPUTE_WORK_GROUP_SIZE + 1, v[1], s1, 1,
                       "COMPOSITE");
    bad |= s1;
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &v[2]);
    const auto s2 = int32_t(glGetError());
    c.sizeZ = v[2];
    g9spf::RecordQuery(c, GL_MAX_COMPUTE_WORK_GROUP_SIZE + 2, v[2], s2, 1,
                       "COMPOSITE");
    bad |= s2;
  }
  {
    GLint v = 0;
    glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_COUNT, &v);
    const auto s = int32_t(glGetError());
    c.countX = v;
    g9spf::RecordQuery(c, GL_MAX_COMPUTE_WORK_GROUP_COUNT, v, s, 1,
                       "COMPOSITE");
    bad |= s;
  }
  {
    GLint64 v = 0;
    glGetInteger64v(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &v);
    const auto s = int32_t(glGetError());
    c.maxBlockBytes = v;
    g9spf::RecordQuery(
        c, GL_MAX_SHADER_STORAGE_BLOCK_SIZE, int64_t(v), s,
        int32_t(std::max<size_t>(need.stateBytes,
                                 std::max(need.workBytes, need.arenaBytes))),
        "COMPOSITE");
    bad |= s;
  }
  bad |= g9spf::QueryCapabilityI(c, GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS,
                                 c.vertexBlocksQueried, "SCENE_ATTR", 0);
  bad |= g9spf::QueryCapabilityI(c, GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS,
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
    out->computeFailure = "COMPOSITE_LIMIT";
    return Result::UNSUPPORTED;
  }
  out->computeEligible = true;
  return Result::OK;
}

VisualTransport::Result VisualTransport::Create(const ParentAdapter &parent,
                                                const Words64 &visualConfig,
                                                const Scope &id) {
  if (parent_ || ready_ || busy_ || !id.run || !id.owner || !id.context ||
      !id.pageSession || !id.inputEpoch ||
      !ValidateVisualConfig(visualConfig, parent))
    return Result::INVALID;
  context_ = eglGetCurrentContext();
  display_ = eglGetCurrentDisplay();
  surface_ = eglGetCurrentSurface(EGL_DRAW);
  thread_ = std::this_thread::get_id();
  identity_ = id;
  if (!Current())
    return Result::CONTEXT_LOST;
  parent_ = parent;
  visual_ = SelectVisual(parent.Index(), parent.Layout().k, visualConfig[5]);
  visualConfig_ = visualConfig;
  if (!visual_)
    return Result::INVALID;
  const auto &p = parent.Layout();
  g9spf::StageCapabilityV2 caps{};
  if (QueryComputeCaps({p.stateBytes, visual_->workBytes, p.arenaBytes},
                       &caps) != Result::OK)
    return Result::UNSUPPORTED;
  GLenum error = 0;
  const auto *code =
      parent.Kind() == ParentKind::DM ? PipelineSourceDm() : PipelineSourceR1();
  const auto *hash = parent.Kind() == ParentKind::DM ? PipelineSourceHashDm()
                                                     : PipelineSourceHashR1();
  const auto *abi =
      parent.Kind() == ParentKind::DM ? kPipelineAbiDm : kPipelineAbiR1;
  if (!programcache::Build(1, code, hash, abi, program_.id, error) ||
      !programcache::Reflect(program_.uniforms, program_.id)) {
    if (error && !firstGl_)
      firstGl_ = error;
    return Result::SHADER_FAILED;
  }
  if (parent.Full()) {
    const auto *cold = SelectCold(parent.Index(), p.k, visual_->v);
    fullCreditContext_ = uint64_t(reinterpret_cast<uintptr_t>(context_));
    if (!cold || !GrsFullGpuReserve(&fullCredit_, fullCreditContext_,
                                    cold->fullGpuBytes))
      return Result::WOULD_BLOCK;
  }
  glGenBuffers(GLsizei(buffers_.size()), buffers_.data());
  auto allocate = [&](GLuint buffer, size_t bytes) {
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, GLsizeiptr(bytes), nullptr,
                 GL_DYNAMIC_COPY);
    auto *zero =
        glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, GLsizeiptr(bytes),
                         GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
    if (!zero)
      return false;
    std::memset(zero, 0, bytes);
    return glUnmapBuffer(GL_SHADER_STORAGE_BUFFER) == GL_TRUE && Good();
  };
  for (size_t i = 0; i < buffers_.size(); ++i)
    if (!allocate(buffers_[i], i < 5    ? p.stateBytes
                               : i == 5 ? visual_->workBytes
                                        : p.arenaBytes))
      return Result::GL_FAILED;
  if (parent.Full()) {
    glGenBuffers(6, full_.data());
    for (auto b : full_)
      if (!allocate(b, p.arenaBytes))
        return Result::GL_FAILED;
  }
  {
    std::vector<uint32_t> state;
    if (!parent.FillSeed(state, id, seedSHA_))
      return Result::INVALID;
    for (size_t i = 0; i < 5; ++i) {
      glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffers_[i]);
      glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, GLsizeiptr(p.stateBytes),
                      state.data());
    }
  }
  if (parent.Kind() == ParentKind::DM) {
    g9dm::Permit warm;
    InitialPermit(warm, id, parent, ShaderHash());
    permit_.parent = warm;
  } else {
    g9spf::Permit warm;
    InitialPermit(warm, id, parent, ShaderHash());
    permit_.parent = warm;
  }
  parent.RootConfig(frozenConfig_, permit_.parent, seedSHA_);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffers_[5]);
  glBufferSubData(GL_SHADER_STORAGE_BUFFER, GLintptr(p.work[9].begin), 512,
                  frozenConfig_.data());
  glBufferSubData(GL_SHADER_STORAGE_BUFFER,
                  GLintptr(visual_->visual[kSegControl] + 60544), 256,
                  visualConfig.data());
  if (!bootstrap_.ReserveInitial(identity_, *visual_, Now()))
    return bootstrap_.RawGl() ? Result::GL_FAILED : Result::WOULD_BLOCK;
  glUseProgram(program_.id);
  Input128 empty{};
  Common(empty);
  for (uint32_t b = 0; b < 4; ++b)
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, b,
                     buffers_[b == 0   ? 0
                              : b == 1 ? 0
                              : b == 2 ? 5
                                       : 6]);
  U("uOp", parent.Kind() == ParentKind::DM ? 12u : 0u);
  U("uPass", 0);
  glDispatchCompute(1, 1, 1);
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
  U("uOp", 0x7fffff01u);
  glDispatchCompute(1, 1, 1);
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
  if (!Good())
    return Result::GL_FAILED;
  const auto initial = InitialSequence();
  if (!initial)
    return Result::INVALID;
  const auto pipeline = ShaderHash();
  TypedParentInitialToken token(*parent_, identity_, seedSHA_, pipeline,
                                initial);
  const auto result = bootstrap_.BeginVisualBootstrap(
      token, visualConfig_, *visual_, buffers_[5], Now(), &bootstrapTicket_);
  return result == VisualBootstrap::Result::IN_PROGRESS   ? Result::IN_PROGRESS
         : result == VisualBootstrap::Result::WOULD_BLOCK ? Result::WOULD_BLOCK
                                                          : Result::GL_FAILED;
}
VisualTransport::Result VisualTransport::PollBootstrap(uint64_t now) {
  const auto before = bootstrap_.Receipt().words[2];
  const auto result = bootstrap_.PollVisualBootstrap(bootstrapTicket_, now);
  if (before == 1u && bootstrap_.Receipt().words[2] >= 2u) {
    const auto &b = bootstrap_.Receipt().words;
    const uint64_t begin = accept647::Pair(b.data(), 82);
    programcache::Warmup(program_.id, begin, now, bootstrap_.RawGl());
  }
  if (result == VisualBootstrap::Result::RAW_READY)
    return Result::OK;
  if (result == VisualBootstrap::Result::IN_PROGRESS)
    return Result::IN_PROGRESS;
  return Result::GL_FAILED;
}
bool VisualTransport::SealBootstrap(uint64_t now) {
  VisualSeedReceipt local;
  if (bootstrap_.SealVisualBootstrap(bootstrapTicket_, &local, now) !=
          VisualBootstrap::Result::SEALED ||
      bootstrap_.End(bootstrapTicket_, now) !=
          VisualBootstrap::Result::SEALED ||
      !bootstrap_.Ready() || programcache::Prepared(program_.id) != GL_NO_ERROR)
    return false;
  ready_ = true;
  return true;
}
std::string VisualTransport::BootstrapPage(uint32_t segment, uint32_t offset,
                                           uint64_t now) {
  std::array<uint8_t, 4096> caller{};
  SeedPageMeta meta;
  auto result =
      bootstrap_.ReadVisualBootstrap(bootstrapTicket_, segment, offset,
                                     caller.data(), caller.size(), &meta, now);
  if (result != VisualBootstrap::Result::RAW_READY)
    return "{\"state\":\"BOOTSTRAP_NOT_READABLE\"}";
  std::ostringstream out;
  out << "{\"revision\":\"G11_VIS_SEED_RAW_V1\",\"id\":\""
      << bootstrapTicket_.value << "\",\"segment\":" << segment
      << ",\"byteOffset\":" << offset << ",\"bytes\":" << meta.bytes
      << ",\"segmentSHA\":\"" << HexDigest(meta.segmentSHA) << "\",\"words\":[";
  for (size_t i = 0; i < meta.bytes / 4; ++i) {
    if (i)
      out << ',';
    uint32_t word;
    std::memcpy(&word, caller.data() + 4 * i, 4);
    out << word;
  }
  out << "]}";
  auto text = out.str();
  return text.size() <= 16384 ? text : "{\"state\":\"PAGE_UTF8_LIMIT\"}";
}
void VisualTransport::Common(const Input128 &input) {
  const auto &p = parent_->Layout();
  const auto &vis = *visual_;
  U("uK", p.k);
  U("uStateWords", uint32_t(p.stateBytes / 4));
  const char *state[] = {"uColumn", "uBed", "uFlow", "uPacket", "uSources"};
  for (size_t i = 0; i < 5; ++i)
    U(state[i], uint32_t(p.state[3 + i].begin / 4));
  const char *names[] = {
      "uFaces",      "uColumnStage",     "uHamilton",       "uOriginAudit",
      "uIndexes",    "uInput",           "uSummary",        "uControls",
      "uCompletion", "uConfig",          "uRootExtension",  "uWideLedger",
      "uContacts",   "uOriginExtension", "uStageDiagnostic"};
  for (size_t i = 0; i < 15; ++i)
    U(names[i], uint32_t(p.work[i].begin / 4));
  U("uVisMarkerCurrent", uint32_t(vis.visual[visualBanks_[0]] / 4));
  U("uVisMarkerActual", uint32_t(vis.visual[visualBanks_[1]] / 4));
  U("uVisMarkerReplay", uint32_t(vis.visual[visualBanks_[2]] / 4));
  U("uVisHeaderCurrent", visualBanks_[0]);
  U("uVisHeaderActual", visualBanks_[1]);
  U("uVisHeaderReplay", visualBanks_[2]);
  U("uVisGlyph", uint32_t(vis.visual[kSegGlyph] / 4));
  U("uVisPlan", uint32_t(vis.visual[kSegMaterialPlan] / 4));
  U("uVisControl", uint32_t(vis.visual[kSegControl] / 4));
  U("uVisMaps", uint32_t(vis.visual[kSegFourMaps] / 4));
  U("uVisV", vis.v);
  P("uVisBaseTicks", permit_.visualTicks);
  P("uVisBaseGeneration", permit_.visualGeneration);
  P("uVisTicks", permit_.visualTicks + (permit_.Kind() == 2u ? 256u : 0u));
  permit_.Visit([&](const auto &q) {
    P("uNonce", q.nonce);
    P("uRun", q.identity.run);
    P("uOwner", q.identity.owner);
    P("uContext", q.identity.context);
    P("uSurface", q.identity.surface);
    P("uGeometry", q.geometry);
    P("uConfigId", q.identity.config);
    P("uInputGeneration", q.identity.inputEpoch);
    P("uSession", q.identity.pageSession);
    P("uPlan", q.plan);
    P("uBaseCommit", q.baseCommit);
    P("uArenaGeneration", q.arenaGeneration);
    U("uCandidateBank", q.candidateBank);
    U("uBaseBank", q.baseBank);
    U("uArena", q.arena);
  });
  U("uRootKind", permit_.Kind());
  U("uRequiredPasses", permit_.Kind() == 1                 ? 19
                       : parent_->Kind() == ParentKind::DM ? 2923
                                                           : 2543);
  U("uRequiredAudits", permit_.Kind() == 1                 ? 0xcffffu
                       : parent_->Kind() == ParentKind::DM ? 0xfffffu
                                                           : 0xcffffu);
  if (parent_->Kind() == ParentKind::DM)
    U("uDmBase", uint32_t(g9dm::DM48ByteStart(*g9dm::SelectProfile(p.k)) / 4));
  U("uEventCapacity", p.eventCapacity);
  U("uPathCapacity", p.pathCapacity);
  U("uPathWord", p.eventCapacity * 64);
  const auto angle = int32_t(input[24]);
  const size_t at = size_t(angle + 7680) * 4;
  I("uAcceptedAngle", angle);
  I("uAngleSin", int32_t(kAngleWords[at + 1]));
  I("uAngleCos", int32_t(kAngleWords[at + 2]));
}
accept647::ProgramFacts VisualTransport::Facts() const {
  const auto &p = parent_->Layout();
  accept647::ProgramFacts f;
  f.producer = this;
  f.program = program_.id;
  f.context = reinterpret_cast<uintptr_t>(context_);
  f.owner = thread_;
  f.k = p.k;
  f.v = visual_->v;
  f.parameterRevision = parent_->ParameterRevision();
  f.eventCapacity = p.eventCapacity;
  f.pathCapacity = p.pathCapacity;
  f.stateBytes = p.stateBytes;
  f.workBytes = visual_->workBytes;
  f.baseVisualGeneration = visualGeneration_;
  f.baseVisualTicks = visualTicks_;
  f.parameter = parent_->ParameterHash();
  f.physicalSeed = seedSHA_;
  f.physicalLayout = parent_->PhysicalLayoutHash();
  f.actualShader = ShaderHash();
  f.physicalSpec = parent_->PhysicalSpecHash();
  f.compositeLayout = HexDigest(kCompositeLayoutSHA);
  f.activePipeline = f.actualShader;
  f.visualConfig = accept647::HashWords(visualConfig_.data(), 64);
  f.visualRecipe = HexDigest(kVisualRecipeSHA);
  f.actualVisualSeed = bootstrap_.SeedHash();
  return f;
}
VisualTransport::Result VisualTransport::Begin(const CompositePermit &p,
                                               const Input128 &input,
                                               Check check, void *user) {
  if (!ready_ || !bootstrap_.Ready() || busy_ || !Current())
    return Result::WOULD_BLOCK;
  if (!check || p.parent.index() != parent_->Index() || p.BaseBank() != 0 ||
      p.CandidateBank() != 2 || p.Kind() < 1 || p.Kind() > 2 ||
      p.visualGeneration != visualGeneration_ ||
      p.visualTicks != visualTicks_ || p.Quanta() > UINT64_MAX / 256 - 1 ||
      p.visualTicks != p.Quanta() * 256 || int32_t(input[24]) < -7680 ||
      int32_t(input[24]) > 7680)
    return Result::INVALID;
  permit_ = p;
  check_ = check;
  checkUser_ = user;
  if (!Allowed())
    return Denied();
  activeIdentity_ = accept647::ActiveProgramIdentity::ForComposite(
      Facts(), uint32_t(parent_->Kind()));
  expected_ = permit_.Visit([&](const auto &q) {
    return accept647::ExpectedForActivePipeline(
        *activeIdentity_, accept647::PermitView::Of(q, p.rootBeginNs),
        uint32_t(parent_->Kind()));
  });
  if (!expected_)
    return Result::INVALID;
  parent_->RootConfig(frozenConfig_, permit_.parent, seedSHA_);
  frozenInput_ = input;
  std::array<uint32_t, 232> upload{};
  if (!expected_->Serialize(upload.data(), 224))
    return Result::INVALID;
  const auto digest = expected_->Hash();
  std::copy(digest.begin(), digest.end(), upload.begin() + 224);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffers_[5]);
  glBufferSubData(GL_SHADER_STORAGE_BUFFER,
                  GLintptr(parent_->Layout().work[5].begin), 512, input.data());
  glBufferSubData(GL_SHADER_STORAGE_BUFFER,
                  GLintptr(parent_->Layout().work[9].begin), 512,
                  frozenConfig_.data());
  glBufferSubData(GL_SHADER_STORAGE_BUFFER,
                  GLintptr(visual_->visual[kSegControl] + 61312), 928,
                  upload.data());
  glUseProgram(program_.id);
  Common(input);
  if (!Good())
    return Result::GL_FAILED;
  tail_ = {};
  evidence_ = {};
  next_ = 0;
  busy_ = true;
  mapped_ = false;
  return Result::OK;
}
void VisualTransport::Bind(const G11Pass &pass) {
  GLuint read = buffers_[1], write = buffers_[permit_.CandidateBank()],
         events = buffers_[6];
  const bool dm = parent_->Kind() == ParentKind::DM;
  const auto op = pass.op;
  if (!pass.vis && (op == (dm ? 13u : 1u) || op == (dm ? 1u : 31u))) {
    read = buffers_[0];
    write = permit_.Kind() == 1 ? buffers_[2] : buffers_[1];
  }
  if (pass.role == 3u || pass.role == 4u)
    write = buffers_[4];
  if (!pass.vis && op == (dm ? 34u : 20u) &&
      (pass.role == 2u || pass.role == 4u)) {
    read = buffers_[7];
    write = buffers_[6];
    events = buffers_[1];
  }
  if (!pass.vis && op == (dm ? 35u : 21u) && pass.role == 4u) {
    read = buffers_[2];
    write = buffers_[4];
  }
  if (!pass.vis && op == (dm ? 40u : 26u)) {
    read = buffers_[3];
    write = buffers_[2];
  }
  if (!pass.vis && op == (dm ? 8u : 38u)) {
    read = buffers_[0];
    write = buffers_[4];
  }
  if (!pass.vis && op == (dm ? 9u : 39u)) {
    read = buffers_[2];
    write = buffers_[4];
  }
  if (!pass.vis && (op == (dm ? 11u : 41u) || op == (dm ? 43u : 29u))) {
    read = buffers_[1];
    write = buffers_[2];
  }
  if (pass.vis && permit_.Kind() == 1 && pass.role == 4u)
    read = buffers_[0];
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, read);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, write);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, buffers_[5]);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, events);
}
bool VisualTransport::Dispatch(const G11Pass &pass, uint32_t index) {
  if (!Allowed())
    return false;
  const auto &layout = parent_->Layout();
  const bool dm = parent_->Kind() == ParentKind::DM;
  if (permit_.Kind() == 1 && index == 0 &&
      !Copy(buffers_[0], 0, buffers_[1], 0, layout.stateBytes / 4))
    return false;
  if (!pass.vis && pass.op == (dm ? 36u : 22u) &&
      (!Copy(buffers_[2], 0, buffers_[3], 0, layout.stateBytes / 4) ||
       !Copy(buffers_[6], 0, buffers_[7], 0, layout.arenaBytes / 4)))
    return false;
  Bind(pass);
  U("uOp", pass.op);
  U("uRole", pass.role);
  U("uJslot", pass.j);
  U("uMicro", uint32_t(pass.micro));
  U("uPass", index);
  U("uVisPhase", pass.phase);
  U("uVisOldBank", pass.role == 4 ? 2 : 1);
  U("uVisNewBank", pass.role == 4 ? 2 : 1);
  U("uVisOrdinal", index);
  if (!Allowed())
    return false;
  glDispatchCompute(1, 1, 1);
  ++evidence_.dispatches;
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
  ++evidence_.barriers;
  if (!Good())
    return false;
  if (!witness_.After(pass, permit_.Nonce(), buffers_[5], *visual_,
                      visualBanks_[pass.role == 4 ? 2 : 1]))
    return false;
  if (!pass.vis && parent_->Full() && pass.role == 1 &&
      pass.op == (dm ? 35u : 21u)) {
    uint32_t trial = 0;
    for (uint32_t j = pass.j; j > 1; j >>= 1)
      ++trial;
    if (trial >= 6 ||
        !Copy(buffers_[6], 0, full_[trial], 0, layout.arenaBytes / 4))
      return false;
  }
  const uint32_t arena = permit_.Visit([](const auto &q) { return q.arena; });
  if (!pass.vis && pass.op == (dm ? 34u : 20u) && pass.role == 2 &&
      !Copy(buffers_[6], 0, buffers_[8 + arena], 0, layout.arenaBytes / 4))
    return false;
  if (!pass.vis && pass.op == (dm ? 9u : 39u) &&
      (!Copy(buffers_[6], 0, buffers_[7], 0, layout.arenaBytes / 4) ||
       !Copy(buffers_[6], 0, buffers_[8 + arena], 0, layout.arenaBytes / 4)))
    return false;
  return Allowed();
}
VisualTransport::Result VisualTransport::Pump() {
  if (!busy_)
    return Result::INVALID;
  if (!Allowed())
    return Denied();
  if (mapped_)
    return Result::OK;
  const bool source = permit_.Kind() == 1,
             dm = parent_->Kind() == ParentKind::DM;
  const G11Pass *table =
      dm ? (source ? kG11DmSourcePasses.data() : kG11DmQuantumPasses.data())
         : (source ? kG11R1SourcePasses.data() : kG11R1QuantumPasses.data());
  const uint32_t count = source ? 19 : dm ? 2923 : 2543;
  if (!fence_) {
    const auto begin = Now();
    uint32_t submitted = 0;
    while (next_ < count && submitted < 10 && Now() - begin < 8000000ull) {
      if (!Dispatch(table[next_], next_))
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
    fence_ = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    glFlush();
    if (!fence_ || !Good())
      return Result::GL_FAILED;
  }
  const auto result = glClientWaitSync(fence_, 0, 0);
  if (result == GL_TIMEOUT_EXPIRED)
    return Result::IN_PROGRESS;
  if (result != GL_ALREADY_SIGNALED && result != GL_CONDITION_SATISFIED ||
      !Good())
    return Result::GL_FAILED;
  evidence_.fenceNs = Now();
  if (!Allowed())
    return Denied();
  glBindBuffer(GL_COPY_READ_BUFFER, buffers_[5]);
  auto *raw = glMapBufferRange(GL_COPY_READ_BUFFER,
                               GLintptr(parent_->Layout().work[8].begin), 1024,
                               GL_MAP_READ_BIT);
  if (!raw)
    return Result::GL_FAILED;
  std::memcpy(tail_.words.data(), raw, 1024);
  const bool unmap = glUnmapBuffer(GL_COPY_READ_BUFFER) == GL_TRUE;
  if (!unmap || !Good())
    return Result::GL_FAILED;
  glDeleteSync(fence_);
  fence_ = nullptr;
  mapped_ = true;
  evidence_.firstGl = firstGl_;
  evidence_.firstEgl = firstEgl_;
  evidence_.observedNs = Now();
  evidence_.currentContext =
      reinterpret_cast<uintptr_t>(eglGetCurrentContext());
  evidence_.actualProgram = program_.id;
  evidence_.owner = std::this_thread::get_id();
  evidence_.fenceSignaled = true;
  evidence_.mapped = true;
  evidence_.current = Current();
  evidence_.liveGuard = Allowed();
  return evidence_.liveGuard ? Result::OK : Denied();
}
std::optional<ValidatedCompositeAccepted> VisualTransport::Accepted() const {
  if (!mapped_ || !expected_ || !activeIdentity_ || !Allowed())
    return {};
  auto actual = evidence_;
  actual.observedNs = Now();
  actual.current = Current();
  actual.liveGuard = Allowed();
  accept647::NativeMappedObservation observation(*activeIdentity_, tail_.words,
                                                 actual);
  return CompositeGate::Accepted(permit_, *expected_, observation);
}
std::optional<ValidatedCompositeBlocked> VisualTransport::Blocked() const {
  if (!mapped_ || !expected_ || !activeIdentity_ || !Allowed())
    return {};
  auto actual = evidence_;
  actual.observedNs = Now();
  actual.current = Current();
  actual.liveGuard = Allowed();
  accept647::NativeMappedObservation observation(*activeIdentity_, tail_.words,
                                                 actual);
  return CompositeGate::Blocked(permit_, *expected_, observation);
}
bool VisualTransport::CommitComposite(const ValidatedCompositeAccepted &proof,
                                      uint64_t now) {
  if (!busy_ || !mapped_ || !expected_ || proof.Producer() != this ||
      !(proof.Permission() == permit_) || proof.Words() != tail_.words ||
      proof.ExpectedHash() != expected_->Hash() || now > permit_.Deadline() ||
      !check_ || !check_(checkUser_, permit_, now))
    return false;
  if (tail_.words[6] != 2) {
    std::swap(buffers_[0], buffers_[2]);
    const auto old = visualBanks_;
    publishedBanks_ = old;
    visualBanks_ = {{old[1], old[2], old[0]}};
    visualGeneration_ = accept647::Pair(tail_.words.data(), 210);
    visualTicks_ = accept647::Pair(tail_.words.data(), 214);
    publishedArena_ = permit_.Visit([](const auto &q) { return q.arena; });
    publishedTail_ = tail_.words;
    publishedInput_ = frozenInput_;
    publishedConfig_ = frozenConfig_;
    published_ = true;
  }
  witness_.RootRetired(permit_.Nonce(), &tail_.words, true);
  busy_ = mapped_ = false;
  expected_.reset();
  activeIdentity_.reset();
  return true;
}
bool VisualTransport::RetireRejected(const CompositePermit &p) {
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
    const auto s = glClientWaitSync(fence_, 0, 0);
    if (s != GL_ALREADY_SIGNALED && s != GL_CONDITION_SATISFIED)
      return false;
    glDeleteSync(fence_);
    fence_ = nullptr;
  }
  witness_.RootRetired(permit_.Nonce(), mapped_ ? &tail_.words : nullptr,
                       false);
  busy_ = mapped_ = false;
  expected_.reset();
  activeIdentity_.reset();
  return Good();
}
size_t VisualTransport::ColdSegments(bool full,
                                     std::array<GpuSegment, 17> &out) const {
  if (!parent_ || !published_ || busy_ || full && !parent_->Full())
    return 0;
  const auto &p = parent_->Layout();
  size_t n = 0;
  out[n++] = {"BASE", buffers_[1], 0, p.stateBytes, 1};
  out[n++] = {"FINAL", buffers_[0], 0, p.stateBytes, 1};
  out[n++] = {"SELECTED_EVENT_PATH", buffers_[7], 0, p.arenaBytes, 64};
  out[n++] = {"AUTHORITATIVE_EVENT_PATH", buffers_[8 + publishedArena_], 0,
              p.arenaBytes, 64};
  if (full)
    for (size_t i = 0; i < 6; ++i)
      out[n++] = {"DISCOVERY_EVENT_PATH", full_[i], 0, p.arenaBytes, 64};
  const char *names[] = {"markerCURRENT", "markerACTUAL", "markerREPLAY",
                         "glyph",         "materialPlan", "control",
                         "fourMaps"};
  const size_t bytes[] = {visual_->v * 128u,
                          visual_->v * 128u,
                          visual_->v * 128u,
                          visual_->v * 32u,
                          2097152u,
                          65536u,
                          524288u};
  const uint32_t strides[] = {32, 32, 32, 8, 8, 1, 4};
  for (size_t i = 0; i < 7; ++i)
    out[n++] = {names[i], buffers_[5],
                visual_->visual[i < 3 ? publishedBanks_[i] : i], bytes[i],
                strides[i]};
  return n;
}
bool VisualTransport::Release(bool current, bool retired) {
  current = current && Current();
  if (!current && !retired)
    return false;
  if (!bootstrap_.Retire(current, retired))
    return false;
  if (busy_ && !retired && !RetireRejected(permit_))
    return false;
  if (!witness_.End(current, retired))
    return false;
  if (current) {
    if (fence_)
      glDeleteSync(fence_);
    glDeleteBuffers(10, buffers_.data());
    glDeleteBuffers(6, full_.data());
    if (program_.id)
      glDeleteProgram(program_.id);
  }
  if (fullCredit_) {
    if (current)
      GrsFullGpuFreed(fullCredit_);
    else if (retired)
      GrsFullGpuContextRetired(fullCreditContext_);
    fullCredit_ = 0;
  }
  if (program_.id)
    programcache::Forget(program_.id);
  buffers_ = {};
  full_ = {};
  program_ = {};
  fence_ = nullptr;
  bootstrapTicket_ = {};
  parent_.reset();
  visual_ = nullptr;
  ready_ = busy_ = mapped_ = published_ = false;
  activeIdentity_.reset();
  expected_.reset();
  visualBanks_ = publishedBanks_ = {{0, 1, 2}};
  visualGeneration_ = visualTicks_ = 0;
  firstGl_ = firstEgl_ = 0;
  context_ = EGL_NO_CONTEXT;
  return true;
}
} // namespace grs::g11vis
