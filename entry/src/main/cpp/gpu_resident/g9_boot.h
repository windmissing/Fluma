#pragma once
#include "g9_boot_capture.h"
#include "program_cache/mapped_build_id.h"
#include "program_cache/program_factory.h"
#include "uniform_locations.h"
#include <hilog/log.h>
#include <cstdio>
#include <iterator>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>

namespace grs::g9boot {
inline constexpr size_t kSourceLimit = 48 * 1024;
inline constexpr GLuint kWarmOpcode = 0x7fffff00;
inline constexpr uint32_t kSeal = 0x641647;

struct Trace {
  const char *family, *sourceSHA, *roleSetSHA;
  uint64_t run, owner, context;
  char buildId[129]{};
  SubmitObserver downstream{};
  const Capture *capture = nullptr;

  Trace(const char *family, const char *source, const char *roles, uint64_t run,
        uint64_t owner, uint64_t context, const void *mapped,
        const Capture *capture = nullptr)
      : family(family), sourceSHA(source), roleSetSHA(roles), run(run),
        owner(owner), context(context), capture(capture) {
#ifdef GRS_G9_DM_BOOT_CAPTURE
    if (!this->capture && std::strcmp(family, "DM") == 0)
      this->capture = CurrentCapture();
#endif
    BuildIdForAddress(mapped, buildId);
  }

  void Record(const char *stage, int64_t raw = -1, int64_t gl = -1,
              int64_t egl = -1, int64_t error = -1,
              int64_t seal = -1, const char *audit = "",
              const uint32_t *workTail = nullptr) const {
    timespec time{};
    clock_gettime(CLOCK_MONOTONIC, &time);
    char ticket[64]{};
    if (capture)
      std::snprintf(ticket, sizeof(ticket), ",\"ticket\":\"%llu\"",
                    static_cast<unsigned long long>(capture->ticket));
    char row[1100]{};
    const int length = std::snprintf(row, sizeof(row),
        "{\"schema\":\"G9BootV1\",\"family\":\"%s\",\"role\":\"BOOT\",\"stage\":\"%s\","
        "\"pid\":%d,\"tid\":%ld,\"run\":\"%llu\",\"owner\":\"%llu\","
        "\"context\":\"%llu\",\"ns\":\"%llu\",\"sourceSHA\":\"%s\","
        "\"roleSetSHA\":\"%s\",\"mappedBuildId\":\"%s\","
        "\"raw\":%lld,\"gl\":%lld,\"egl\":%lld,\"error\":%lld,\"seal\":%lld%s%s}",
        family, stage, int(getpid()), syscall(SYS_gettid),
        static_cast<unsigned long long>(run), static_cast<unsigned long long>(owner),
        static_cast<unsigned long long>(context),
        static_cast<unsigned long long>(uint64_t(time.tv_sec) * 1000000000 + time.tv_nsec),
        sourceSHA, roleSetSHA, buildId, static_cast<long long>(raw),
        static_cast<long long>(gl), static_cast<long long>(egl),
        static_cast<long long>(error), static_cast<long long>(seal), ticket, audit);
    if (capture && capture->record && length > 0 && size_t(length) < sizeof(row))
      capture->record(capture->context, row, size_t(length), workTail);
    OH_LOG_Print(LOG_APP, LOG_INFO, 0xE734, "G9Boot", "%{public}s", row);
    if (std::strcmp(family, "DM") == 0) {
      std::fprintf(stderr, "%s\n", row);
      std::fflush(stderr);
    }
  }

  SubmitObserver ProgramObserver() {
    return {this, [](void *context, SubmitStage s, uint64_t root, uint32_t code,
                     uint32_t ordinal, uint64_t ns) {
      auto &trace = *static_cast<Trace *>(context);
      switch (s) {
      case SubmitStage::ShaderEnter: trace.Record("SHADER_COMPILE_ENTER"); break;
      case SubmitStage::ShaderExit: trace.Record("SHADER_COMPILE_EXIT", code); break;
      case SubmitStage::ProgramEnter: trace.Record("PROGRAM_LINK_ENTER"); break;
      case SubmitStage::ProgramExit: trace.Record("PROGRAM_LINK_EXIT", code); break;
      default: break;
      }
      trace.downstream.Emit(s, root, code, ordinal, ns);
    }};
  }
};

// Active interface of the T733-reviewed BOOT closure. EventPath is declared
// in the common ABI, but the closure never accesses it.
struct Reflection {
  UniformLocations &locations;
  bool Prepare(GLuint program) {
    if (!locations.Prepare(program))
      return false;
    constexpr const char *scalar[]{
        "uControls", "uColumn", "uPacket", "uFlow", "uBed", "uSources",
        "uConfig", "uOriginAudit", "uK", "uOp", "uSummary"};
    constexpr const char *pair[]{
        "uRun", "uOwner", "uContext", "uBaseCommit", "uInputGeneration"};
    uint32_t scalars = 0, pairs = 0;
    for (GLuint i = 0; i < locations.count; ++i) {
      char name[96]{};
      GLenum type = 0;
      GLint words = 0;
      glGetActiveUniform(program, i, sizeof(name), nullptr, &words, &type, name);
      for (size_t j = 0; j < std::size(scalar); ++j)
        if (std::strcmp(name, scalar[j]) == 0 && words == 1 &&
            type == GL_UNSIGNED_INT && locations.Find(name) >= 0)
          scalars |= 1u << j;
      for (size_t j = 0; j < std::size(pair); ++j)
        if (std::strcmp(name, pair[j]) == 0 && words == 1 &&
            type == GL_UNSIGNED_INT_VEC2 && locations.Find(name) >= 0)
          pairs |= 1u << j;
    }
    if (scalars != (1u << std::size(scalar)) - 1 ||
        pairs != (1u << std::size(pair)) - 1)
      return false;
    GLint lanes[3]{};
    glGetProgramiv(program, GL_COMPUTE_WORK_GROUP_SIZE, lanes);
    if (lanes[0] != 64 || lanes[1] != 1 || lanes[2] != 1)
      return false;
    constexpr const char *blocks[]{"ReadState", "WriteState", "Workspace", "EventPath"};
    for (GLint binding = 0; binding < 4; ++binding) {
      const auto index = glGetProgramResourceIndex(program, GL_SHADER_STORAGE_BLOCK,
                                                  blocks[binding]);
      if (index == GL_INVALID_INDEX) {
        if (binding < 3)
          return false;
        continue;
      }
      constexpr GLenum property = GL_BUFFER_BINDING;
      GLint actual = -1;
      glGetProgramResourceiv(program, GL_SHADER_STORAGE_BLOCK, index, 1, &property,
                             1, nullptr, &actual);
      if (actual != binding)
        return false;
    }
    return true;
  }
};

inline bool Build(Trace &trace, const char *source, const char *abi,
                  GLuint &program, UniformLocations &locations,
                  uint32_t &firstGl, uint32_t &firstEgl, SubmitObserver observer) {
  const size_t bytes = std::strlen(source);
  if (!bytes || bytes > kSourceLimit ||
      programcache::Hash(source, bytes) != trace.sourceSHA)
    return false;
  trace.downstream = observer;
  GLenum raw = GL_NO_ERROR;
  const std::string key = std::string(abi) + "|ROLE=BOOT|ROLE_SET=" + trace.roleSetSHA;
  trace.Record("BUILD_ENTER", bytes);
  const bool built = programcache::Build(1, source, trace.sourceSHA, key.c_str(),
                                         program, raw, trace.ProgramObserver(), 0,
                                         "G9_BOOT");
  if (raw && !firstGl)
    firstGl = raw;
  trace.Record("BUILD_EXIT", built, firstGl);
  if (!built)
    return false;
  Reflection reflection{locations};
  trace.Record("REFLECT_ENTER");
  const bool reflected = programcache::Reflect(reflection, program);
  raw = glGetError();
  if (raw && !firstGl)
    firstGl = raw;
  const auto egl = eglGetError();
  if (egl != EGL_SUCCESS && !firstEgl)
    firstEgl = egl;
  const bool qualified = reflected && !firstGl && !firstEgl;
  trace.Record("REFLECT_EXIT", qualified, firstGl, egl);
  return qualified;
}

enum class Audit { Ok, Clock, Gl, Rejected };

template <class Time, class ReadTime>
Audit RunAudit(const Trace &trace, GLuint program, GLint opcodeLocation,
               GLuint work, size_t tailByteOffset, ReadTime readTime,
               uint32_t &firstGl, uint32_t &firstEgl) {
  GLenum sampledGl = GL_NO_ERROR;
  EGLint sampledEgl = EGL_SUCCESS;
  auto errors = [&] {
    sampledGl = glGetError();
    sampledEgl = eglGetError();
    if (sampledGl && !firstGl)
      firstGl = sampledGl;
    if (sampledEgl != EGL_SUCCESS && !firstEgl)
      firstEgl = sampledEgl;
    return !firstGl && !firstEgl;
  };
  auto rawEgl = [&] { return firstEgl ? EGLint(firstEgl) : EGL_SUCCESS; };
  Time begin, end;
  if (!readTime(begin))
    return Audit::Clock;
  trace.Record("ROOT_INIT_ENTER");
  glDispatchCompute(1, 1, 1);
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
  trace.Record("ROOT_INIT_EXIT");
  glUniform1ui(opcodeLocation, kWarmOpcode);
  trace.Record("WARM_ENTER");
  glDispatchCompute(1, 1, 1);
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
  trace.Record("WARM_EXIT");
  const auto fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
  const bool fenceCreated = fence != nullptr;
  trace.Record("FINISH_ENTER", fenceCreated);
  glFinish();
  const auto status = fence ? glClientWaitSync(fence, 0, 0) : GL_WAIT_FAILED;
  if (fence)
    glDeleteSync(fence);
  const bool waitOk = status == GL_ALREADY_SIGNALED || status == GL_CONDITION_SATISFIED;
  const bool sameApi = errors();
  char detail[300]{};
  if (trace.capture) {
    char wait[24] = "null";
    if (fenceCreated)
      std::snprintf(wait, sizeof(wait), "%u", unsigned(status));
    std::snprintf(detail, sizeof(detail),
        ",\"fenceWaitRaw\":%s,\"fenceCompleted\":%s,\"glSample\":%u,\"eglSample\":%d",
        wait, waitOk ? "true" : "false", unsigned(sampledGl), int(sampledEgl));
  }
  trace.Record("FINISH_EXIT", status, firstGl, rawEgl(), -1, -1, detail);
  if (!waitOk || !sameApi)
    return Audit::Gl;
  if (!readTime(end))
    return Audit::Clock;
  glBindBuffer(GL_COPY_READ_BUFFER, work);
  trace.Record("MAP_ENTER", tailByteOffset);
  const auto *tail = static_cast<const uint32_t *>(glMapBufferRange(
      GL_COPY_READ_BUFFER, GLintptr(tailByteOffset), 2 * sizeof(uint32_t), GL_MAP_READ_BIT));
  errors();
  if (trace.capture)
    std::snprintf(detail, sizeof(detail),
        ",\"workObject\":%u,\"workByteOffset\":%zu,\"workRequestedBytes\":8,\"glSample\":%u,\"eglSample\":%d",
        work, tailByteOffset, unsigned(sampledGl), int(sampledEgl));
  trace.Record("MAP_EXIT", tail != nullptr, firstGl, rawEgl(), -1, -1, detail);
  if (!tail)
    return Audit::Gl;
  const uint32_t rawTail[]{tail[0], tail[1]};
  const auto error = rawTail[0], seal = rawTail[1];
  const auto unmapped = glUnmapBuffer(GL_COPY_READ_BUFFER);
  const bool readOk = errors();
  const bool validTail = unmapped == GL_TRUE && readOk;
  if (trace.capture)
    std::snprintf(detail, sizeof(detail),
        ",\"workObject\":%u,\"workByteOffset\":%zu,\"workTailBytes\":%u,\"glSample\":%u,\"eglSample\":%d",
        work, tailByteOffset, validTail ? 8u : 0u, unsigned(sampledGl), int(sampledEgl));
  trace.Record("UNMAP_EXIT", unmapped, firstGl, rawEgl(), error, seal, detail,
               validTail ? rawTail : nullptr);
  if (unmapped != GL_TRUE || !readOk)
    return Audit::Gl;
  if (error || seal != kSeal) {
    trace.Record("BOOT_AUDIT_REJECTED", 0, firstGl, rawEgl(), error, seal);
    return Audit::Rejected;
  }
  // A linked program is cache-eligible only after the real initialization
  // and both tail words are verified.
  const auto prepared = programcache::Prepared(program);
  if (prepared && !firstGl)
    firstGl = prepared;
  const bool preparedOk = errors();
  trace.Record("PREPARE_EXIT", preparedOk, firstGl, rawEgl(), error, seal);
  if (!preparedOk)
    return Audit::Gl;
  programcache::Warmup(program, begin.ns, end.ns, GL_NO_ERROR);
  trace.Record("BOOT_AUDIT_OK", 1, firstGl, rawEgl(), error, seal);
  return Audit::Ok;
}
} // namespace grs::g9boot
