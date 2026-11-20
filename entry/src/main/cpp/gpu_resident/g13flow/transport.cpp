#include "transport.h"
#include "../program_cache/program_factory.h"
#include "../resource_gl.h"
#include "init_diagnostics.h"
#include <fstream>
namespace grs::g13 {
namespace {
constexpr size_t kProbeOffset = 18632;
constexpr std::array<uint32_t, 8> kExpectedFp32{
    0x3f800001u, 0x3f800000u, 1u, 0x00400000u, 7u, 0u, 0u, 0x46503332u};

bool QualifiedProbe(const uint32_t *words) {
  return std::equal(kExpectedFp32.begin(), kExpectedFp32.end(), words) &&
         std::equal(kExpectedFp32.begin(), kExpectedFp32.end(), words + 8);
}
} // namespace

bool Transport::Locations::Prepare(GLuint program) {
  static constexpr const char *names[]{
      "uOp", "uScope", "uJ", "uMicro", "uBlock", "uIndex", "uL[0]", "uTestHook"};
  at.fill(-1);
  layoutWords = 0;
  GLint count = 0;
  glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &count);
  if (count < 0 || count > GLint(at.size()))
    return false;
  for (GLint i = 0; i < count; ++i) {
    char name[32]{};
    GLint words = 0;
    GLenum type = 0;
    glGetActiveUniform(program, i, sizeof(name), nullptr, &words, &type, name);
    size_t slot = 0;
    while (slot < at.size() && std::strcmp(name, names[slot]) != 0 &&
           !(slot == LayoutArray && std::strcmp(name, "uL") == 0))
      ++slot;
    if (slot == at.size() || type != GL_UNSIGNED_INT || at[slot] >= 0 ||
        words < 1 || words > (slot == LayoutArray ? 80 : 1))
      return false;
    at[slot] = glGetUniformLocation(program, name);
    if (at[slot] < 0)
      return false;
    if (slot == LayoutArray)
      layoutWords = words;
  }
  if (at[Op] < 0 || at[Scope] < 0 || at[Index] < 0 ||
      (requireLayout && !layoutWords))
    return false;
  const char *blocks[]{"Base", "Candidate", "Work", "Control"};
  for (GLint binding = 0; binding < 4; ++binding) {
    const GLuint index = glGetProgramResourceIndex(
        program, GL_SHADER_STORAGE_BLOCK, blocks[binding]);
    if (index == GL_INVALID_INDEX) {
      if (binding == 3 || requireLayout)
        return false;
      continue;
    }
    constexpr GLenum property = GL_BUFFER_BINDING;
    GLint actual = -1;
    glGetProgramResourceiv(program, GL_SHADER_STORAGE_BLOCK, index, 1,
                           &property, 1, nullptr, &actual);
    if (actual != binding)
      return false;
  }
  return true;
}

bool Transport::BuildRole(size_t index) {
  struct LegacyIsolation {
    create468::Recorder *previous = create468::active;
    LegacyIsolation() { create468::active = nullptr; }
    ~LegacyIsolation() { create468::active = previous; }
  } isolated;
  auto &program = programs_[index];
  const auto &role = PipelineRoles()[index];
  std::string abi = std::string("G13_FOUR_SSBO_64_LANES|") +
                    kHash_layoutSHA + "|MATH=" + kHash_mathSHA +
                    "|GATES=" + kHash_gatesSHA +
                    "|ALGORITHM=" + kHash_algorithmSHA;
  if (PipelineRoleCount() > 1)
    abi += std::string("|ROLE=") + role.name;
  InitStageEvent("PROGRAM_BUILD", "ENTER", index, -1, role.sha);
  const bool built = programcache::Build(13, role.source, role.sha, abi.c_str(),
                                        program.id, firstGl_, {}, programGeneration_);
  InitStageEvent("PROGRAM_BUILD", "EXIT", index, firstGl_, role.sha);
  if (!built)
    return false;
  program.uniforms.requireLayout = index == 0;
  InitStageEvent("REFLECT", "ENTER", index, -1, role.sha);
  const bool reflected = programcache::Reflect(program.uniforms, program.id);
  const auto raw = glGetError();
  if (raw && !firstGl_)
    firstGl_ = raw;
  InitObservation().Stage("REFLECT", "EXIT", index, raw, role.sha,
                          -1, -1, -1, -1, 0, reflected && !raw ? 1 : 0);
  return reflected && !raw;
}

bool Transport::UploadRoleUniforms(size_t index) {
  const auto &program = programs_[index];
  const auto &uniforms = program.uniforms;
  glUseProgram(program.id);
  if (uniforms.layoutWords)
    glUniform1uiv(uniforms.at[LayoutArray], uniforms.layoutWords, layout_.data());
  if (uniforms.at[TestHook] >= 0)
    glUniform1ui(uniforms.at[TestHook], testHook_);
  const auto raw = glGetError();
  if (raw && !firstGl_)
    firstGl_ = raw;
  return !raw;
}

bool Transport::Owner() const {
  return owner_ == std::this_thread::get_id() && context_ != EGL_NO_CONTEXT &&
         context_ == eglGetCurrentContext();
}
uint32_t Transport::Allocate(GLuint &b, size_t words) {
  glGenBuffers(1, &b);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, b);
  glBufferData(GL_SHADER_STORAGE_BUFFER, GLsizeiptr(words * 4), nullptr,
               GL_DYNAMIC_COPY);
  auto e = glGetError();
  if (e && !firstGl_)
    firstGl_ = e;
  return e ? 30 : 0;
}
uint32_t Transport::Configure(const Config &config, const Seed &seed,
                              const Identity &identity) {
  if (Busy() || RetirementPending())
    return 34;
  auto error = ValidateConfig(config, seed.diagnostic != 0);
  if (error)
    return error;
  Retire(true);
  context_ = eglGetCurrentContext();
  owner_ = std::this_thread::get_id();
  programGeneration_ = identity.config;
  if (context_ == EGL_NO_CONTEXT)
    return 10;
  resource501::Bind(&ledger_, 13);
  profile_ = FindProfile(config[4], config[5]);
  config_ = config;
  testHook_ = seed.testHook;
  seedReceipt_ = seed.receipt;
  if (seedReceipt_.size() > 448)
    return 30;
  InitObservation().State("CAPABILITY");
  InitStageEvent("CAPABILITY", "ENTER");
  GLint blocks = 0, group = 0, shared = 0, bindings = 0, invocations = 0,
        precision = 0, range[2] = {};
  glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &bindings);
  glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &invocations);
  GLint64 maxBlock = 0;
  glGetIntegerv(GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS, &blocks);
  glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &group);
  glGetIntegerv(GL_MAX_COMPUTE_SHARED_MEMORY_SIZE, &shared);
  glGetInteger64v(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &maxBlock);
  firstGl_ = glGetError();
  const GLenum resourceGl = firstGl_;
  glGetShaderPrecisionFormat(GL_VERTEX_SHADER, GL_HIGH_FLOAT, range,
                             &precision);
  const GLenum precisionGl = glGetError();
  if (!firstGl_)
    firstGl_ = precisionGl;
  InitStageEvent("CAPABILITY", "EXIT", 0, firstGl_);
  std::ostringstream cap;
  cap << "{\"stage\":\"CAP_COMPUTE\",\"blocks\":" << blocks
      << ",\"bindings\":" << bindings << ",\"invocations\":" << invocations
      << ",\"precisionSource\":\"VERTEX_HIGH_FLOAT_DECLARATION\"";
  cap << ",\"precisionThreshold\":23,\"precisionRawGL\":" << precisionGl
      << ",\"resourceRawGL\":" << resourceGl << ",\"precisionStatus\":\""
      << (precisionGl      ? "QUERY_ERROR"
          : precision < 23 ? "LOW_PRECISION"
                           : "PREFILTER_PASS")
      << "\",\"floatPrecision\":";
  if (precisionGl)
    cap << "null";
  else
    cap << precision;
  cap << ",\"precisionRange\":";
  if (precisionGl)
    cap << "null";
  else
    cap << '[' << range[0] << ',' << range[1] << ']';
  cap << ",\"driverEnvironmentSHA\":\"" << programcache::Environment()
      << "\",\"laneX\":" << group << ",\"shared\":" << shared
      << ",\"maxBlock\":\"" << maxBlock << "\",\"rawGL\":" << firstGl_
      << ",\"CAP_FP32\":\"NOT_MEASURED\",\"CAP_MATH\":\"NOT_MEASURED\",\"CAP_"
         "SCENE\":\"NOT_MEASURED\"}";
  auto driverString = [](GLenum key) {
    const auto *value = glGetString(key);
    return value ? std::string(reinterpret_cast<const char *>(value))
                 : std::string();
  };
  auto vendor = driverString(GL_VENDOR), renderer = driverString(GL_RENDERER),
       version = driverString(GL_VERSION),
       language = driverString(GL_SHADING_LANGUAGE_VERSION);
  capabilities_ = cap.str();
  capabilities_.pop_back();
  capabilities_ += ",\"vendor\":" + Quote(vendor) +
                   ",\"renderer\":" + Quote(renderer) +
                   ",\"version\":" + Quote(version) +
                   ",\"language\":" + Quote(language) + "}";
  if (capabilities_.size() > 1984)
    return 30;
  if (firstGl_ || blocks < 4 || bindings < 4 || invocations < 64 ||
      precision < 23 || group < 64 || shared < 4096 ||
      maxBlock < GLint64(profile_->ww * 4))
    return 29;
  InitObservation().State("PROGRAM_BUILD");
  if (!BuildRole(0))
    return 29;
  InitObservation().State("REFLECT");
  const auto prepared = programcache::Prepared(programs_[0].id);
  if (prepared != GL_NO_ERROR)
    return 29;
  InitObservation().State("ALLOCATE_SEED");
  InitStageEvent("ALLOCATE_SEED", "ENTER");
  compile_ = std::string("{\"pipelineSHA\":\"") + PipelineSha() +
             "\",\"variant\":\"" + PipelineVariant() +
             "\",\"bootSourceSHA\":\"" + PipelineRoles()[0].sha +
             "\",\"layoutSHA\":\"" + kHash_layoutSHA + "\",\"mathSHA\":\"" +
             kHash_mathSHA +
             "\",\"computeProgramLinked\":true,\"numericQualification\":\"NOT_"
             "MEASURED\"}";
  for (auto &b : banks_)
    if (auto e = Allocate(b, profile_->sw))
      return e;
  for (auto &b : packs_)
    if (auto e = Allocate(b, profile_->pw))
      return e;
  if (auto e = Allocate(work_, profile_->ww))
    return e;
  if (auto e = Allocate(ctrl_, 18688))
    return e;
  layout_[0] = profile_->nx;
  layout_[1] = profile_->ny;
  layout_[2] = profile_->nc;
  layout_[3] = profile_->nf;
  layout_[4] = profile_->sw;
  layout_[5] = profile_->ww;
  layout_[6] = profile_->pw;
  layout_[7] = profile_->mgCount;
  layout_[8] = profile_->groups;
  layout_[9] = profile_->state[2].offset;
  layout_[10] = profile_->state[3].offset;
  layout_[11] = profile_->state[4].offset;
  layout_[12] = 0;
  for (uint32_t i = 0; i < profile_->mgCount; i++) {
    for (uint32_t j = 0; j < 3; j++)
      layout_[40 + 3 * i + j] = profile_->mg[i][j];
    layout_[12] += profile_->mg[i][0] * profile_->mg[i][1];
  }
  for (uint32_t i = 0; i < 20; i++)
    layout_[16 + i] = profile_->work[i].offset;
  std::array<uint32_t, 18688> control{};
  std::copy(config.begin(), config.end(), control.begin());
  auto mathHash = FromHex(kHash_mathSHA), programHash = FromHex(PipelineRoles()[0].sha),
       deviceHash = FromHex(programcache::Environment());
  std::copy(mathHash.begin(), mathHash.end(), control.begin() + 704);
  std::copy(programHash.begin(), programHash.end(), control.begin() + 720);
  std::copy(deviceHash.begin(), deviceHash.end(), control.begin() + 728);
  control[744] = 1u;
  control[745] = 0x33800000u;
  control[746] = 0x3f800000u;
  control[747] = 0u;
  control[18648] = 0x34000000u; // one ULP at 1
  control[18649] = 0x00800000u; // smallest normal
  control[18650] = 0x3f000000u; // factor of one half
  control[18651] = 0x7f800000u; // +infinity
  control[18652] = 0x7fc00000u; // quiet NaN
  control[748] = 0x54442d18u;
  control[749] = 0x3ff921fbu;
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ctrl_);
  glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(control), control.data());
  const auto &boot = programs_[0].uniforms.at;
  if (!UploadRoleUniforms(0))
    return 29;
  if (PipelineTestHooks() && PipelineRoleCount() == 1 && boot[TestHook] < 0)
    return 29;
  // The initial state is generated and audited by the cold GPU opcode. Native
  // writes only its identity header. One-time audited fixture uploads are raw.
  std::array<uint32_t, 256> header{};
  header[0] = kMagic_state;
  header[1] = 1;
  header[2] = kMagic_profile;
  header[3] = profile_->sw;
  header[4] = profile_->nx;
  header[5] = profile_->ny;
  header[6] = config[6];
  Put(header.data() + 8, identity.run);
  Put(header.data() + 10, identity.context);
  Put(header.data() + 12, identity.surface);
  Put(header.data() + 14, identity.geometry);
  header[26] = config[23];
  header[27] = config[21];
  header[28] = config[22];
  header[29] = config[26];
  header[30] = config[20];
  header[206] = config[20];
  header[7] = 1;
  header[202] = config[24];
  header[203] = config[25];
  header[204] = config[27];
  header[205] = config[26];
  if (config[27] != 0) {
    header[200] = 0xffffffffu;
    header[201] = 0xffffffffu;
  }
  for (auto pair : std::array<std::pair<size_t, size_t>, 5>{
           {{32, 76}, {40, 36}, {48, 44}, {56, 52}, {64, 60}}})
    std::copy_n(config.data() + pair.second, 8, header.data() + pair.first);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, banks_[0]);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, work_);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ctrl_);
  InitStageEvent("ALLOCATE_SEED", "EXIT");
  InitObservation().State("INIT_DISPATCH64");
  InitStageEvent("INIT_DISPATCH64", "ENTER", 64);
  glUniform1ui(boot[Op], 64);
  for (uint32_t i = 0; i < 3; i++) {
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, banks_[i]);
    glUniform1ui(boot[Scope], i == 0 ? 0 : 1);
    glDispatchCompute(1, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
  }
  for (auto b : packs_) {
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, b);
    glUniform1ui(boot[Scope], 2);
    glDispatchCompute(1, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
  }
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, banks_[0]);
  glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(header), header.data());
  if (config[7] != 0) {
    if (seed.path.empty())
      return 28;
    std::ifstream file(seed.path, std::ios::binary | std::ios::ate);
    if (!file || file.tellg() != std::streamoff(profile_->sw * 4))
      return 28;
    std::unique_ptr<uint32_t[]> raw(new (std::nothrow) uint32_t[profile_->sw]);
    if (!raw)
      return 30;
    resource501::HostScope rawSeed(&ledger_, "G13_RAW_SEED", raw.get(),
                                   profile_->sw * 4, profile_->sw * 4);
    file.seekg(0);
    file.read(reinterpret_cast<char *>(raw.get()), profile_->sw * 4);
    auto hash = Hash(raw.get(), profile_->sw * 4);
    if (!file || Hex(hash.data()) != seed.sha ||
        seed.sha != Hex(config.data() + 60))
      return 3;
    if (raw[0] != kMagic_state || raw[1] != 1 || raw[2] != kMagic_profile ||
        raw[3] != profile_->sw || raw[4] != profile_->nx ||
        raw[5] != profile_->ny || raw[6] != config[6])
      return 1;
    for (size_t i = 223; i < 256; i++)
      if (raw[i])
        return 5;
    for (size_t i = 167; i < 192; i += 8)
      if (raw[i])
        return 5;
    if (config[7] == 2) {
      if (raw[7] != 1 || U64(raw.get() + 18) >= UINT64_MAX / 4 - 1)
        return 28;
      for (size_t i = 18; i < 32; i++)
        header[i] = raw[i];
      for (size_t i = 80; i < 223; i++)
        header[i] = raw[i];
    }
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, profile_->sw * 4, raw.get());
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(header), header.data());
  }
  InitStageEvent("INIT_DISPATCH64", "EXIT", 64);
  seedHeader_ = header;
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, banks_[0]);
  InitObservation().State("INIT_AUDIT65");
  InitStageEvent("INIT_AUDIT65", "ENTER", 65);
  glUniform1ui(boot[Op], 65);
  glDispatchCompute(1, 1, 1);
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
  InitStageEvent("INIT_AUDIT65", "EXIT", 65);
  InitStageEvent("INIT_FP32", "ENTER", 66);
  glUniform1ui(boot[Op], 66);
  for (uint32_t replay = 0; replay < 2; ++replay) {
    glUniform1ui(boot[Scope], replay);
    glDispatchCompute(1, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT |
                    GL_BUFFER_UPDATE_BARRIER_BIT);
  }
  const auto probeGl = glGetError();
  if (probeGl && !firstGl_)
    firstGl_ = probeGl;
  InitStageEvent("INIT_FP32", "EXIT", 66, probeGl);

  InitObservation().State("INIT_FENCE_PENDING");
  InitStageEvent("INIT_FENCE_PENDING", "ENTER");
  fence_ = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
  glFlush();
  if (!fence_)
    return 30;
  submitted_ = false;
  pass_ = SIZE_MAX;
  current_ = 0;
  candidate_ = 1;
  replay_ = 2;
  pack_ = 0;
  pendingPack_ = 1;
  return 0;
}
uint32_t Transport::PrepareSteady(bool cancelled) {
  if (!Owner() || !BootReady())
    return 10;
  if (nextSteadyRole_ >= PipelineRoleCount())
    return 0;
  const auto index = nextSteadyRole_;
  const auto &role = PipelineRoles()[index];
  auto &program = programs_[index];
  if (steadyFence_) {
    const GLenum status = glClientWaitSync(steadyFence_, 0, 0);
    if (status == GL_TIMEOUT_EXPIRED)
      return 34;
    if (status == GL_WAIT_FAILED) {
      const auto error = glGetError();
      if (error && !firstGl_)
        firstGl_ = error;
    }
    glDeleteSync(steadyFence_);
    steadyFence_ = nullptr;
    if (cancelled)
      return 11;
    std::array<uint32_t, 16> probe{};
    const bool qualified = status != GL_WAIT_FAILED && !steadyProbeError_ &&
        CopyBuffer(ctrl_, probe.data(), probe.size(), kProbeOffset * sizeof(uint32_t)) &&
        QualifiedProbe(probe.data());
    InitObservation().Stage("ROLE_FP32", "EXIT", index, firstGl_, role.sha,
                            -1, status, -1, -1, 0, qualified ? 1 : 0);
    if (!qualified)
      return firstGl_ || status == GL_WAIT_FAILED ? 29 : 27;
    if (programcache::Prepared(program.id) != GL_NO_ERROR)
      return 29;
    ++nextSteadyRole_;
    return Ready() ? 0 : 34;
  }
  if (cancelled)
    return 11;
  if (!BuildRole(index) || !UploadRoleUniforms(index))
    return 29;
  InitStageEvent("ROLE_FP32", "ENTER", index, -1, role.sha);
  const std::array<uint32_t, 16> clear{};
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ctrl_);
  glBufferSubData(GL_SHADER_STORAGE_BUFFER, kProbeOffset * sizeof(uint32_t),
                  sizeof(clear), clear.data());
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ctrl_);
  glUniform1ui(program.uniforms.at[Op], 66);
  for (uint32_t replay = 0; replay < 2; ++replay) {
    glUniform1ui(program.uniforms.at[Scope], replay);
    glDispatchCompute(1, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
  }
  steadyProbeError_ = glGetError();
  if (steadyProbeError_ && !firstGl_)
    firstGl_ = steadyProbeError_;
  steadyFence_ = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
  glFlush();
  return steadyFence_ ? 34 : 30;
}
void Transport::Bind(const Pass &p, const Locations &locations) {
  GLuint base = banks_[current_], out = banks_[p.scope ? replay_ : candidate_];
  if (p.op == OP_SOURCE_REPLAY_COPY || p.op == OP_SOURCE_REPLAY_INSTALL ||
      p.op == OP_SOURCE_REPLAY_AUDIT)
    out = banks_[replay_];
  if (p.op == OP_ROOT_SCOPE_COMPARE || p.op == OP_SOURCE_COMPARE) {
    base = banks_[candidate_];
    out = banks_[replay_];
  }
  if (p.op == OP_PACK_VIEW || p.op == OP_PACK_AUDIT) {
    base = banks_[candidate_];
    out = packs_[pendingPack_];
  }
  if (p.op == OP_ROOT_TIME_PREPARE || p.op == OP_ROOT_COMPLETION_SEAL ||
      p.op == OP_SOURCE_CONTROL_PREPARE || p.op == OP_SOURCE_COMPLETION_SEAL)
    out = banks_[candidate_];
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, base);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, out);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, work_);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ctrl_);
  glUniform1ui(locations.at[Op], p.op);
  glUniform1ui(locations.at[Scope], p.scope);
  glUniform1ui(locations.at[TrialJ], p.J);
  glUniform1ui(locations.at[Micro], p.micro);
  glUniform1ui(locations.at[Block], p.block);
  glUniform1ui(locations.at[Index], uint32_t(pass_));
}
uint32_t Transport::BeginRoot(const Permit &permit, const Input &original,
                              const Input &canonical,
                              const std::array<uint32_t, 256> &quota) {
  if (!Owner() || !Ready())
    return 10;
  if (Busy())
    return 34;
  permit_ = permit;
  lastBase_ = current_;
  lastCandidate_ = candidate_;
  aborted_ = false;
  source_ = permit[1] >= 50;
  std::array<uint32_t, 1024> capsule{};
  std::copy(original.begin(), original.end(), capsule.begin() + 64);
  std::copy(canonical.begin(), canonical.end(), capsule.begin() + 192);
  std::copy(config_.begin(), config_.end(), capsule.begin() + 832);
  auto receipt = [&](size_t at, size_t words, uint32_t tag,
                     const std::string &json) {
    if (json.size() > (words - 16) * 4)
      return false;
    capsule[at] = tag;
    capsule[at + 1] = 1;
    capsule[at + 2] = 1;
    capsule[at + 3] = uint32_t(words);
    capsule[at + 4] = uint32_t(json.size());
    auto hash = Hash(json.data(), json.size());
    std::copy(hash.begin(), hash.end(), capsule.begin() + at + 6);
    capsule[at + 15] = kSeal;
    std::memcpy(capsule.data() + at + 16, json.data(), json.size());
    return true;
  };
  std::ostringstream receiptMeta;
  receiptMeta << "{\"nativeNs\":\"" << U64(canonical.data() + 22)
              << "\",\"serial\":\"" << U64(canonical.data() + 106)
              << "\",\"sourceSeq\":\"" << U64(canonical.data() + 20)
              << "\",\"normalization\":\"STAMP_ONLY\"}";
  if (!receipt(0, 64, 8, receiptMeta.str()) ||
      !receipt(448, 128, 10, seedReceipt_.empty() ? "{}" : seedReceipt_) ||
      !receipt(320, 128, 9,
               std::string("{\"fixtureSHA\":\"") + Hex(config_.data() + 68) +
                   "\"}") ||
      !receipt(576, 128, 11,
               std::string("{\"boundary\":") + std::to_string(config_[6]) +
                   ",\"bottomBits\":" + std::to_string(config_[32]) +
                   ",\"topBits\":" + std::to_string(config_[33]) + "}") ||
      !receipt(704, 128, 12,
               std::string("{\"baseQuantum\":\"") +
                   std::to_string(U64(permit.data() + 88)) +
                   "\",\"sourceSeq\":\"" +
                   std::to_string(U64(permit.data() + 90)) +
                   "\",\"modeSource\":\"STATE_HEADER_24_31_192_222\"}") ||
      !receipt(960, 64, 13,
               std::string("{\"boot\":\"") + std::to_string(kBoot) +
                   "\",\"nonce\":\"" + std::to_string(U64(permit.data() + 8)) +
                   "\",\"domain\":\"" +
                   (permit[2] ? "COMPUTE_ONLY_NATIVE_CONTROL"
                              : "PAGE_NATIVE_CONTROL") +
                   "\"}"))
    return 30;
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ctrl_);
  glBufferSubData(GL_SHADER_STORAGE_BUFFER, 512, sizeof(permit), permit.data());
  glBufferSubData(GL_SHADER_STORAGE_BUFFER, 1024, sizeof(quota), quota.data());
  glBufferSubData(GL_SHADER_STORAGE_BUFFER, 4096, sizeof(capsule),
                  capsule.data());
  pass_ = 0;
  submitted_ = true;
  return 0;
}
uint32_t Transport::PumpBounded(uint64_t now) {
  if (!Owner())
    return 10;
  if (!submitted_)
    return 0;
  const auto limit = source_ ? 16u : 820u;
  const auto *passes = source_ ? k_source : k_quantum;
  uint64_t begin = Now();
  size_t lastRole = PipelineRoleCount();
  for (uint32_t count = 0; pass_ < limit && count < 10; count++) {
    now = Now();
    if (!now || now > U64(permit_.data() + 38))
      return 9;
    if (guard_) {
      auto denial = guard_(guardContext_, now);
      if (denial)
        return denial;
    }
    if (now - begin >= 8000000ull)
      break;
    const auto &p = passes[pass_];
    const auto role = PipelineRoleForOpcode(p.op);
    if (role >= PipelineRoleCount() || !programs_[role].id)
      return 29;
    if (lastRole != role) {
      glUseProgram(programs_[role].id);
      lastRole = role;
    }
    Bind(p, programs_[role].uniforms);
    glDispatchCompute(1, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    ++pass_;
    auto e = glGetError();
    if (e) {
      if (!firstGl_)
        firstGl_ = e;
      return 10;
    }
  }
  if (pass_ == limit) {
    fence_ = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    glFlush();
    submitted_ = false;
    if (!fence_)
      return 30;
  }
  return 0;
}
bool Transport::CopyBuffer(GLuint b, uint32_t *dest, size_t words,
                           size_t offset) {
  glBindBuffer(GL_COPY_READ_BUFFER, b);
  void *p = glMapBufferRange(GL_COPY_READ_BUFFER, GLintptr(offset * 4),
                             GLsizeiptr(words * 4), GL_MAP_READ_BIT);
  if (!p) {
    copyStage_ = "MAP_FAILED";
    auto raw = glGetError();
    if (raw && !firstGl_)
      firstGl_ = raw;
    return false;
  }
  std::memcpy(dest, p, words * 4);
  bool ok = glUnmapBuffer(GL_COPY_READ_BUFFER) == GL_TRUE;
  auto e = glGetError();
  if (e && !firstGl_)
    firstGl_ = e;
  if (!ok)
    copyStage_ = "UNMAP_FAILED";
  else if (e)
    copyStage_ = "COPY_GL_ERROR";
  else
    copyStage_ = "COPIED";
  return ok && !e;
}
uint32_t Transport::Poll(Completion &out) {
  if (!Owner())
    return 10;
  if (!fence_)
    return 34;
  GLenum s = glClientWaitSync(fence_, 0, 0);
  if (pass_ == SIZE_MAX)
    InitStageEvent("INIT_FENCE_POLL", "EXIT", 0, -1, nullptr, -1, s);
  if (s == GL_TIMEOUT_EXPIRED)
    return 34;
  if (s == GL_WAIT_FAILED) {
    auto e = glGetError();
    if (!firstGl_)
      firstGl_ = e;
    return 10;
  }
  glDeleteSync(fence_);
  fence_ = nullptr;
  if (aborted_) {
    aborted_ = false;
    return 11;
  }
  if (pass_ == SIZE_MAX) {
    Completion cold{};
    if (!CopyBuffer(ctrl_, cold.data(), 256, 18432))
      return 10;
    pass_ = 0;
    InitStageEvent("INIT_TAIL_VALID", "EXIT", 65, -1, nullptr, -1, s, cold[255],
                   cold[7]);
    if (cold[255] != kSeal || cold[7])
      return cold[7] ? cold[7] : 28;
    // Cold-only words, observed after the existing init fence. Both independent
    // runs use the same linked compute program and the same uploaded operands.
    const bool observed =
        cold[207] == kExpectedFp32.back() && cold[215] == kExpectedFp32.back();
    const bool qualified =
        observed && !firstGl_ && QualifiedProbe(cold.data() + 200);
    std::ostringstream proof;
    proof << ",\"fp32Source\":\"SAME_PROGRAM_COLD_COMPUTE\",\"fp32Words\":[";
    for (size_t i = 200; i < 216; ++i) {
      if (i != 200)
        proof << ',';
      proof << cold[i];
    }
    proof << "],\"fp32RawGL\":" << firstGl_;
    auto capability = capabilities_;
    const std::string pending = "\"CAP_FP32\":\"NOT_MEASURED\"";
    capability.replace(capability.find(pending), pending.size(),
                       std::string("\"CAP_FP32\":\"") +
                           (qualified  ? "QUALIFIED"
                            : observed ? "FAILED"
                                       : "NOT_MEASURED") +
                           "\"");
    capability.insert(capability.size() - 1, proof.str());
    if (capability.size() > 1984)
      return 30;
    capabilities_ = std::move(capability);
    InitStageEvent("INIT_FP32_RESULT", "EXIT", 66, firstGl_);
    if (!qualified)
      return firstGl_ ? 29 : 27;
    InitObservation().State("INIT_TAIL_VALID");
    ready_ = true;
    return 0;
  }
  if (!CopyBuffer(ctrl_, out.data(), 256, 18432))
    return 10;
  return 0;
}
void Transport::Abort() {
  if (submitted_ && !fence_) {
    submitted_ = false;
    aborted_ = true;
    fence_ = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    glFlush();
  }
}
void Transport::Accept() {
  std::swap(current_, candidate_);
  std::swap(pack_, pendingPack_);
}
void Transport::Retire(bool same) {
  const bool pending = context_ != EGL_NO_CONTEXT && (Busy() || !same);
  if (pending) {
    ledger_.Tombstone(reinterpret_cast<uintptr_t>(context_), 13);
    resource501::Bind(nullptr, 0);
  }
  if (context_ != EGL_NO_CONTEXT && (Busy() || !same)) {
    retiringContext_ = context_;
    if (profile_)
      retiringBytes_ = 3ull * profile_->sw * 4 + profile_->ww * 4 + 74752 +
                       2ull * profile_->pw * 4;
  }
  ready_ = false;
  submitted_ = false;
  if (same && context_ == eglGetCurrentContext()) {
    if (fence_)
      glDeleteSync(fence_);
    if (steadyFence_)
      glDeleteSync(steadyFence_);
    for (auto b : banks_)
      if (b)
        glDeleteBuffers(1, &b);
    for (auto b : packs_)
      if (b)
        glDeleteBuffers(1, &b);
    if (work_)
      glDeleteBuffers(1, &work_);
    if (ctrl_)
      glDeleteBuffers(1, &ctrl_);
    for (const auto &program : programs_)
      if (program.id)
        glDeleteProgram(program.id);
  }
  for (const auto &program : programs_)
    if (program.id)
      programcache::Forget(program.id, context_, programGeneration_);
  programs_ = {};
  banks_ = {};
  packs_ = {};
  work_ = ctrl_ = 0;
  fence_ = steadyFence_ = nullptr;
  steadyProbeError_ = GL_NO_ERROR;
  nextSteadyRole_ = 1;
  context_ = EGL_NO_CONTEXT;
}
uint32_t Transport::CaptureSpan(bool full, uint32_t *dst, size_t words,
                                size_t &written) {
  written = 0;
  if (!Owner() || Busy())
    return 34;
  size_t needed = 2 * profile_->sw + 18688 +
                  (full ? profile_->sw + profile_->ww + 2 * profile_->pw : 0);
  if (words < needed)
    return 30;
  auto copy = [&](GLuint b, size_t n) {
    if (!CopyBuffer(b, dst + written, n))
      return false;
    written += n;
    return true;
  };
  if (!copy(banks_[lastBase_], profile_->sw) ||
      !copy(banks_[lastCandidate_], profile_->sw))
    return 10;
  if (full &&
      (!copy(banks_[replay_], profile_->sw) || !copy(work_, profile_->ww)))
    return 10;
  if (!copy(ctrl_, 18688))
    return 10;
  if (full &&
      (!copy(packs_[0], profile_->pw) || !copy(packs_[1], profile_->pw)))
    return 10;
  return 0;
}
} // namespace grs::g13
