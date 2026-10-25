// P-013 P0 v1: synchronous atomic frame contract; no borrowed JS storage.
#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <ctime>
namespace p013 {
constexpr size_t kFloats = 512, kColumns = 128, kTelemetryCapacity = 2048;
constexpr double kSafeInteger = 9007199254740991.0;
enum class Status : int {
  OK = 0,
  NOT_READY = 1,
  INACTIVE = 2,
  INVALID_ARGUMENT = 100,
  VERSION_MISMATCH = 101,
  STALE_FRAME = 102,
  STALE_SURFACE = 103,
  WRONG_THREAD = 104,
  SESSION_BUSY = 105,
  RELEASED = 106,
  EGL_INIT_FAILED = 200,
  CONTEXT_LOST = 201,
  BAD_SURFACE = 202,
  SHADER_COMPILE_FAILED = 300,
  GL_RESOURCE_FAILED = 301,
  GL_DRAW_FAILED = 302,
  COLORSPACE_UNSUPPORTED = 304,
  SWAP_FAILED = 303
};
enum class State {
  WAIT_WINDOW,
  WINDOW_AVAILABLE,
  READY,
  SUSPENDED,
  FAILED,
  RELEASED
};
inline const char* StateName(State s) {
  switch (s) {
    case State::WAIT_WINDOW:
      return "WAIT_WINDOW";
    case State::WINDOW_AVAILABLE:
      return "WINDOW_AVAILABLE";
    case State::READY:
      return "READY";
    case State::SUSPENDED:
      return "SUSPENDED";
    case State::FAILED:
      return "FAILED";
    default:
      return "RELEASED";
  }
}
inline double NowMs() {
  timespec t{};
  clock_gettime(CLOCK_MONOTONIC, &t);
  return t.tv_sec * 1000.0 + t.tv_nsec / 1e6;
}
struct FrameInput {
  double sessionId = 0, frameId = 0, snapshotId = 0, surfaceEpoch = 0,
         fieldEpoch = 0, geometryEpoch = 0, continuityEpoch = 0;
  double appearanceConfigRevision = 0, lightConfigRevision = 0, frameTimeNs = 0;
  double surfaceWidthPx = 0, surfaceHeightPx = 0, boxXPx = 0, boxYPx = 0,
         boxWidthPx = 0, boxHeightPx = 0, capacity = 0;
  double T_dataLayoutVersion = 0, zOriginVersion = 0;
  bool visualInverted = false, diagnosticEnabled = false;
};
struct LightParams {
  double la = 0, ld = 0, lmin = 0, lmax = 0, lref = 0, ellLightMaterial = 0,
         lightX = 0, lightY = 0, lightZ = 0, sigmaCandidate = 0;
};
inline bool SafeInteger(double v) {
  return std::isfinite(v) && v >= 0 && v <= kSafeInteger && std::floor(v) == v;
}
inline Status Validate(const FrameInput& f, const LightParams& l,
                       const std::array<float, kFloats>& a) {
  const double ids[] = {f.sessionId,          f.frameId,
                        f.snapshotId,         f.surfaceEpoch,
                        f.fieldEpoch,         f.geometryEpoch,
                        f.continuityEpoch,    f.appearanceConfigRevision,
                        f.lightConfigRevision};
  for (double v : ids)
    if (!SafeInteger(v)) return Status::INVALID_ARGUMENT;
  if (f.frameId < 1 || !std::isfinite(f.frameTimeNs) || f.frameTimeNs < 0)
    return Status::INVALID_ARGUMENT;
  if (f.T_dataLayoutVersion != 1 || f.zOriginVersion != 1)
    return Status::VERSION_MISMATCH;
  if (!SafeInteger(f.surfaceWidthPx) || !SafeInteger(f.surfaceHeightPx) ||
      f.surfaceWidthPx < 1 || f.surfaceHeightPx < 1 ||
      f.surfaceWidthPx > 2147483647 || f.surfaceHeightPx > 2147483647)
    return Status::INVALID_ARGUMENT;
  const double numbers[] = {f.boxXPx,
                            f.boxYPx,
                            f.boxWidthPx,
                            f.boxHeightPx,
                            f.capacity,
                            l.la,
                            l.ld,
                            l.lmin,
                            l.lmax,
                            l.lref,
                            l.ellLightMaterial,
                            l.lightX,
                            l.lightY,
                            l.lightZ,
                            l.sigmaCandidate};
  for (double v : numbers)
    if (!std::isfinite(v) || !std::isfinite(static_cast<float>(v)))
      return Status::INVALID_ARGUMENT;
  if (static_cast<float>(f.capacity) <= 0 ||
      static_cast<float>(f.boxWidthPx) <= 0 ||
      static_cast<float>(f.boxHeightPx) <= 0 || f.capacity <= 0 ||
      f.boxXPx < 0 || f.boxYPx < 0 || f.boxWidthPx <= 0 || f.boxHeightPx <= 0 ||
      f.boxXPx + f.boxWidthPx > f.surfaceWidthPx ||
      f.boxYPx + f.boxHeightPx > f.surfaceHeightPx)
    return Status::INVALID_ARGUMENT;
  if (l.ld < 0 || l.lmin < 0 || l.lmax < l.lmin || l.lref < l.lmin ||
      l.lref > l.lmax || l.ellLightMaterial <= 0 || l.sigmaCandidate < 0 ||
      std::hypot(std::hypot(l.lightX, l.lightY), l.lightZ) == 0 ||
      !std::isfinite(std::hypot(std::hypot(l.lightX, l.lightY), l.lightZ)) ||
      l.lref - l.lmin > 1 || l.lmax - l.lref > 1)
    return Status::INVALID_ARGUMENT;
  const float pxPerMaterial = static_cast<float>(f.boxHeightPx / f.capacity);
  const float ellPx =
      static_cast<float>(l.ellLightMaterial * f.boxHeightPx / f.capacity);
  if (!std::isfinite(pxPerMaterial) || pxPerMaterial <= 0 ||
      !std::isfinite(ellPx) || ellPx <= 0)
    return Status::INVALID_ARGUMENT;
  for (size_t c = 0; c < kColumns; c++) {
    const float s = a[c * 4], w = a[c * 4 + 1], b = a[c * 4 + 2],
                m = a[c * 4 + 3];
    if (!std::isfinite(s) || !std::isfinite(w) || !std::isfinite(b) ||
        !std::isfinite(m) || b < 0 || b > m || m > s || s > w || w > f.capacity)
      return Status::INVALID_ARGUMENT;
  }
  return Status::OK;
}
struct Telemetry {
  size_t nativeTypedByteLength = 0, nativeTypedByteOffset = 0, nativeBufferByteLength = 0;
  bool compositionProfile = false;
  uint64_t probeSourceHash = 0;
  FrameInput frame{};
  LightParams light{}, inputLight{};
  Status code = Status::NOT_READY;
  State state = State::WAIT_WINDOW;
  uint64_t tdataHash = 0, compileLogHash = 0;
  double contextEpoch = 0, bridgeValidateMs = 0, bridgeCopyMs = 0,
         bridgeCpuMs = 0, uploadCpuMs = 0, uploadPathMs = 0;
  double eglInitMs = 0, makeCurrentMs = 0, unbindMs = 0, swapMs = 0,
         lifecycleLockWaitMs = 0, shaderBuildMs = 0, uniformCpuMs = 0,
         drawCpuMs = 0;
  uint64_t uploadedFrameId = 0, submittedFrameId = 0, lastSwapFrameId = 0,
           ownerThreadId = 0, frameGap = 0;
  uint64_t uploadCount = 0, textureAllocCount = 0, surfaceCreateCount = 0,
           surfaceDestroyCount = 0, contextCreateCount = 0,
           contextDestroyCount = 0;
  uint64_t textureAbandonCount = 0, programAbandonCount = 0,
           vaoAbandonCount = 0;
  uint64_t contextLossCount = 0, recoveryAttemptCount = 0,
           staleLifecycleCount = 0;
  unsigned eglError = 0, glError = 0;
  bool swapSucceeded = false, recoveryRequired = false;
  int textureLiveCount = 0, programLiveCount = 0, vaoLiveCount = 0;
  const char* errorStage = "NONE";
};
}  // namespace p013
