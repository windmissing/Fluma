#pragma once
#include "../program_cache/mapped_build_id.h"
#include "../program_cache/program_observer.h"
#include "wire.h"
#include <cstdio>
#include <cstring>
#include <hilog/log.h>
#include <mutex>
#include <string_view>
#include <sys/syscall.h>
#include <unistd.h>
namespace grs::g13 {
struct InitStage {
  uint64_t seq = 0, ns = 0, boot = 0, attempt = 0, ticket = 0, queue = 0,
           window = 0, context = 0, tid = 0;
  int64_t rawNapi = -1, rawEGL = -1, rawGL = -1, fence = -1, ctrl255 = -1,
          ctrl7 = -1;
  uint32_t op = 0;
  int32_t apiReturn = -1;
  char phase[32]{}, edge[16]{}, sourceSHA[65]{};
  uint8_t pad[15]{};
};
static_assert(sizeof(InitStage) == 256);
struct ConfigTicket {
  uint64_t boot = 0, attempt = 0, queueSerial = 0, cancelGeneration = 0;
  Identity proposed{};
  std::array<uint32_t, 8> configRawSHA{}, seedReceiptSHA{};
};
struct InitControl {
  uint64_t samples = 0, stageCount = 0, dropped = 0, truncated = 0,
           decodeCount = 0, handledTicket = 0;
  uint64_t queuedTicket = 0;
  ConfigTicket ticket{};
  uint64_t window = 0, context = 0;
  char lifecycle[40] = "UNCONFIGURED", firstFailure[512]{}, lastProgress[64]{};
  uint64_t firstFailureAttempt = 0;
  uint32_t logBytes[2]{}, logReported[2]{};
};
struct InitDecodeFields {
  uint64_t attempt = 0, rawReturnedLength = 0, normalizedElementCount = 0,
           backingBytes = 0;
  int64_t offset = -1;
  int32_t rawNapi = -1, typedKind = -1, status[6] = {-1, -1, -1, -1, -1, -1};
  uint32_t flags = 0, lengthUnit = 0, copiedWords = 0;
  char stage[40]{}, profileId[48]{}, profileEvidenceSHA[65]{};
  std::array<uint32_t, 128> snapshot{};
};
struct InitDecode {
  InitDecodeFields value{};
  std::array<uint8_t, 1024 - sizeof(InitDecodeFields)> pad{};
};
static_assert(sizeof(InitDecode) == 1024);
struct InitPayload {
  std::array<InitDecode, 4> decode{};
  std::array<InitStage, 64> stages{};
  std::array<std::array<char, 4096>, 2> logs{};
  InitControl control{};
  std::array<uint8_t, 4096 - sizeof(InitControl)> pad{};
};
static_assert(sizeof(InitPayload) == 32768);
inline void LogFirstFrameStage(const InitStage &row, const ConfigTicket &ticket) {
  const std::string_view phase(row.phase);
  if (phase != "PROGRAM_BUILD" && phase != "SHADER_COMPILE" &&
      phase != "SHADER_COMPILE_STATUS" && phase != "PROGRAM_LINK" &&
      phase != "PROGRAM_LINK_STATUS" && phase != "REFLECT" &&
      phase != "CACHE_BINARY_LOAD" && phase != "COMPUTE_READY" &&
      phase != "FIRST_VALID_SWAP" && phase != "OWNER_EXCEPTION" &&
      phase != "CONFIG_CANCELLED")
    return;
  static const struct Module {
    char buildId[129]{};
    Module() {
      BuildIdForAddress(reinterpret_cast<const void *>(&LogFirstFrameStage), buildId);
    }
  } module;
  OH_LOG_Print(LOG_APP, LOG_INFO, 0xE720, "G13Init",
               "boot=%{public}llu attempt=%{public}llu queue=%{public}llu run=%{public}llu context=%{public}llu surface=%{public}llu ns=%{public}llu phase=%{public}s edge=%{public}s raw=%{public}lld apiReturn=%{public}d sourceSHA=%{public}s mappedBuildId=%{public}s",
               static_cast<unsigned long long>(row.boot),
               static_cast<unsigned long long>(row.attempt),
               static_cast<unsigned long long>(row.queue),
               static_cast<unsigned long long>(ticket.proposed.run),
               static_cast<unsigned long long>(ticket.proposed.context),
               static_cast<unsigned long long>(ticket.proposed.surface),
               static_cast<unsigned long long>(row.ns), row.phase, row.edge,
               static_cast<long long>(row.rawGL), row.apiReturn, row.sourceSHA,
               module.buildId);
}
class InitDiagnostic {
  mutable std::mutex mutex_;
  InitPayload payload_{};

public:
  void Queued(uint64_t attempt) {
    std::lock_guard<std::mutex> lock(mutex_);
    payload_.control.queuedTicket = attempt;
  }
  void Ticket(const ConfigTicket &t) {
    std::lock_guard<std::mutex> lock(mutex_);
    payload_.control.ticket = t;
  }
  void Environment(void *w, void *c) {
    std::lock_guard<std::mutex> lock(mutex_);
    payload_.control.window = reinterpret_cast<uintptr_t>(w);
    payload_.control.context = reinterpret_cast<uintptr_t>(c);
  }
  void State(const char *state) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::snprintf(payload_.control.lifecycle, 40, "%s", state);
  }
  void Failure(const std::string &reason, uint64_t attempt) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto &c = payload_.control;
    if (!c.firstFailure[0]) {
      std::snprintf(c.firstFailure, 512, "%s", reason.c_str());
      c.firstFailureAttempt = attempt;
      if (reason.size() >= 512)
        ++c.truncated;
    }
  }
  void Decode(const InitDecode &record, bool valid) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto &c = payload_.control;
    auto index =
        c.decodeCount < 4 ? c.decodeCount : 1 + (c.decodeCount - 4) % 3;
    if (c.decodeCount >= 4)
      ++c.dropped;
    ++c.decodeCount;
    payload_.decode[index] = record;
    if (!valid)
      std::strcpy(c.lifecycle, "DECODE_REJECTED");
  }
  void Stage(const char *phase, const char *edge, uint32_t op = 0,
             int64_t rawGL = -1, const char *source = nullptr,
             int64_t rawEGL = -1, int64_t fence = -1, int64_t seal = -1,
             int64_t error = -1, uint64_t explicitAttempt = 0,
             int32_t apiReturn = -1) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto &c = payload_.control;
    size_t index =
        c.stageCount < 64 ? c.stageCount : 32 + (c.stageCount - 64) % 32;
    if (c.stageCount >= 64)
      ++c.dropped;
    ++c.stageCount;
    auto &row = payload_.stages[index];
    row = {};
    row.seq = ++c.samples;
    row.ns = Now();
    row.boot = kBoot;
    row.attempt = explicitAttempt ? explicitAttempt : c.ticket.attempt;
    row.ticket = explicitAttempt ? 0 : c.ticket.attempt;
    row.queue = explicitAttempt ? 0 : c.ticket.queueSerial;
    row.window = c.window;
    row.context = c.context;
    row.tid = syscall(SYS_gettid);
    row.op = op;
    row.apiReturn = apiReturn;
    row.rawGL = rawGL;
    row.rawEGL = rawEGL;
    row.fence = fence;
    row.ctrl255 = seal;
    row.ctrl7 = error;
    std::snprintf(row.phase, 32, "%s", phase);
    std::snprintf(row.edge, 16, "%s", edge);
    if (source)
      std::snprintf(row.sourceSHA, 65, "%s", source);
    std::snprintf(c.lastProgress, 64, "%s:%s", phase, edge);
    LogFirstFrameStage(row, c.ticket);
  }
  void Log(uint32_t index, const char *bytes, size_t length, int64_t reported) {
    std::lock_guard<std::mutex> lock(mutex_);
    index %= 2;
    auto &c = payload_.control;
    auto n = std::min<size_t>(4096, length);
    std::memcpy(payload_.logs[index].data(), bytes, n);
    c.logBytes[index] = n;
    c.logReported[index] = reported < 0 ? 0 : uint32_t(reported);
    if (n < length || reported > int64_t(n))
      ++c.truncated;
  }
  void Handled(uint64_t id) {
    std::lock_guard<std::mutex> lock(mutex_);
    payload_.control.handledTicket = id;
  }
  std::string Summary() const {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto &c = payload_.control;
    return "{\"schema\":\"G13InitDiagnosticV1\",\"rawBytes\":32768,"
           "\"currentLifecycle\":" +
           Quote(c.lifecycle) + ",\"lastProgress\":" + Quote(c.lastProgress) +
           ",\"firstConfigFailure\":" + Quote(c.firstFailure) +
           ",\"firstFailureAttempt\":\"" +
           std::to_string(c.firstFailureAttempt) + "\",\"queuedTicket\":\"" +
           std::to_string(c.queuedTicket) + "\",\"handledTicket\":\"" +
           std::to_string(c.handledTicket) +
           "\",\"dropped\":" + std::to_string(c.dropped) +
           ",\"truncated\":" + std::to_string(c.truncated) +
           ",\"unobservedRaw\":-1}";
  }
  std::string Page(uint64_t attempt, uint32_t page) const {
    std::array<uint8_t, 8192> bytes{};
    uint64_t sample = 0;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (page >= 4 ||
          (!attempt ? false
                    : attempt != payload_.control.ticket.attempt &&
                          attempt != payload_.control.firstFailureAttempt))
        return "{\"state\":\"STALE_ATTEMPT_OR_PAGE\"}";
      std::memcpy(bytes.data(),
                  reinterpret_cast<const uint8_t *>(&payload_) + 8192 * page,
                  8192);
      sample = payload_.control.samples;
    }
    static const char *table =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string encoded;
    encoded.reserve(10924);
    for (size_t i = 0; i < bytes.size(); i += 3) {
      size_t n = std::min<size_t>(3, bytes.size() - i);
      uint32_t v = uint32_t(bytes[i]) << 16;
      if (n > 1)
        v |= uint32_t(bytes[i + 1]) << 8;
      if (n > 2)
        v |= bytes[i + 2];
      encoded += table[(v >> 18) & 63];
      encoded += table[(v >> 12) & 63];
      encoded += n > 1 ? table[(v >> 6) & 63] : '=';
      encoded += n > 2 ? table[v & 63] : '=';
    }
    return "{\"schema\":\"G13InitDiagnosticV1\",\"state\":\"SNAPSHOT\","
           "\"page\":" +
           std::to_string(page) + ",\"sampleSeq\":\"" + std::to_string(sample) +
           "\",\"offsetBytes\":" + std::to_string(page * 8192) +
           ",\"rawBytes\":8192,\"base64\":" + Quote(encoded) + "}";
  }
};
inline InitDiagnostic &InitObservation() {
  static InitDiagnostic object;
  return object;
}
inline void InitStageEvent(const char *phase, const char *edge, uint32_t op = 0,
                           int64_t gl = -1, const char *sha = nullptr,
                           int64_t egl = -1, int64_t fence = -1,
                           int64_t seal = -1, int64_t error = -1) {
  InitObservation().Stage(phase, edge, op, gl, sha, egl, fence, seal, error);
}
inline void InstallInitObserver() {
  programcache::route13Hook = {
      nullptr,
      [](void *, const char *phase, const char *edge, uint32_t op, int64_t raw,
         const char *sha) { InitStageEvent(phase, edge, op, raw, sha); },
      [](void *, uint32_t slot, const char *bytes, size_t length,
         int64_t reported) {
        InitObservation().Log(slot, bytes, length, reported);
      }};
}
} // namespace grs::g13
