// P-013 P0 v1. The mutex protects the borrowed window through unbind.
#include "napi_bridge.h"

#include <napi/native_api.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <memory>
#include <mutex>
#include <new>
#include <string>
#include <thread>

#include "egl_context.h"
#include "p013_shader_sources.h"
#include "shader_pass.h"
#include "tdata_uploader.h"
namespace p013 {
namespace {
struct FrameStorage {
  std::array<float, kFloats> staging{};
  std::array<Telemetry, kTelemetryCapacity> ring{};
};
struct Session {
  std::mutex mutex;
  EglContext egl;
  ShaderPass shader;
  TDataUploader uploader;
  std::unique_ptr<FrameStorage> storage;
  size_t head = 0, count = 0;
  uint64_t dropped = 0, serial = 0, id = 0, attempt = 0, lastSwap = 0;
  std::thread::id owner{};
  bool ownerBound = false, released = false;
  Status lastError = Status::OK;
  std::string componentId;
  OH_NativeXComponent* component = nullptr;
} g;
void Put(napi_env e, napi_value o, const char* k, double v) {
  if (!o) return;
  napi_value x = nullptr;
  napi_create_double(e, v, &x);
  napi_set_named_property(e, o, k, x);
}
void Put(napi_env e, napi_value o, const char* k, bool v) {
  if (!o) return;
  napi_value x = nullptr;
  napi_get_boolean(e, v, &x);
  napi_set_named_property(e, o, k, x);
}
void Put(napi_env e, napi_value o, const char* k, const char* v) {
  if (!o) return;
  napi_value x = nullptr;
  napi_create_string_utf8(e, v, NAPI_AUTO_LENGTH, &x);
  napi_set_named_property(e, o, k, x);
}
napi_value Object(napi_env e) {
  napi_value o = nullptr;
  if (napi_create_object(e, &o) != napi_ok || !o) {
    napi_throw_error(e, "P013_NAPI_ALLOCATION",
                     "Cannot allocate renderer result");
    return nullptr;
  }
  return o;
}
bool Number(napi_env e, napi_value o, const char* k, double& out) {
  napi_value v = nullptr;
  napi_valuetype t;
  return napi_get_named_property(e, o, k, &v) == napi_ok &&
         napi_typeof(e, v, &t) == napi_ok && t == napi_number &&
         napi_get_value_double(e, v, &out) == napi_ok;
}
bool Boolean(napi_env e, napi_value o, const char* k, bool& out) {
  napi_value v = nullptr;
  napi_valuetype t;
  return napi_get_named_property(e, o, k, &v) == napi_ok &&
         napi_typeof(e, v, &t) == napi_ok && t == napi_boolean &&
         napi_get_value_bool(e, v, &out) == napi_ok;
}
bool String(napi_env e, napi_value v, std::string& out) {
  napi_valuetype t;
  if (napi_typeof(e, v, &t) != napi_ok || t != napi_string) return false;
  char b[256]{};
  size_t n = 0;
  if (napi_get_value_string_utf8(e, v, b, sizeof(b), &n) != napi_ok ||
      n >= sizeof(b) - 1)
    return false;
  out.assign(b, n);
  return true;
}
Status Owner() {
  return g.ownerBound && g.owner != std::this_thread::get_id()
             ? Status::WRONG_THREAD
             : Status::OK;
}
napi_value Control(napi_env e, Status code) {
  napi_value o = Object(e);
  Put(e, o, "code", static_cast<double>(code));
  Put(e, o, "sessionId", static_cast<double>(g.id));
  Put(e, o, "state", StateName(g.egl.state));
  Put(e, o, "surfaceEpoch", static_cast<double>(g.egl.surfaceEpoch));
  Put(e, o, "contextEpoch", static_cast<double>(g.egl.contextEpoch));
  return o;
}
void ResetGpu() {
  g.shader.Reset(false);
  g.uploader.Reset(false);
}
void Destroy(ResetReason reason) {
  // Normal owner teardown deletes names while current. Callback/error teardown
  // abandons registered names only when their owning context is destroyed.
  if (g.egl.state == State::READY && g.egl.active && !g.egl.current &&
      (g.shader.Program() || g.uploader.Texture())) {
    SurfaceInfo info{};
    if (g.egl.EnsureCurrent({g.egl.surfaceEpoch, g.egl.width, g.egl.height},
                            info) == Status::OK) {
      g.shader.Reset(true);
      g.uploader.Reset(true);
      g.egl.Unbind();
    }
  }
  g.egl.DestroyUnbound(reason);
  if (!g.egl.current) ResetGpu();
}
void Created(OH_NativeXComponent* c, void* w) {
  std::lock_guard<std::mutex> lock(g.mutex);
  if (c != g.component || g.released) return;
  const auto epoch = g.egl.surfaceEpoch;
  g.egl.OnWindowCreated(c, w);
  if (epoch != g.egl.surfaceEpoch) ResetGpu();
}
void Changed(OH_NativeXComponent* c, void* w) {
  std::lock_guard<std::mutex> lock(g.mutex);
  if (c == g.component && !g.released) g.egl.OnWindowChanged(c, w);
}
void Destroyed(OH_NativeXComponent* c, void* w) {
  std::lock_guard<std::mutex> lock(g.mutex);
  if (c != g.component) return;
  const auto epoch = g.egl.surfaceEpoch;
  g.egl.OnWindowDestroyed(c, w);
  if (epoch != g.egl.surfaceEpoch) ResetGpu();
}
OH_NativeXComponent_Callback callbacks{Created, Changed, Destroyed, nullptr};
void Hook(napi_env env, napi_value exports) {
  bool has = false;
  napi_has_named_property(env, exports, OH_NATIVE_XCOMPONENT_OBJ, &has);
  if (!has) return;
  napi_value v = nullptr;
  OH_NativeXComponent* c = nullptr;
  if (napi_get_named_property(env, exports, OH_NATIVE_XCOMPONENT_OBJ, &v) !=
          napi_ok ||
      napi_unwrap(env, v, reinterpret_cast<void**>(&c)) != napi_ok || !c)
    return;
  char id[OH_XCOMPONENT_ID_LEN_MAX + 1]{};
  uint64_t n = sizeof(id);
  if (OH_NativeXComponent_GetXComponentId(c, id, &n) != 0) return;
  {
    std::lock_guard<std::mutex> lock(g.mutex);
    if (g.ownerBound && g.componentId != id) return;
    g.component = c;
    g.componentId = id;
    g.released = false;
  }
  OH_NativeXComponent_RegisterCallback(c, &callbacks);
}
void Cleanup(void*) {
  std::lock_guard<std::mutex> lock(g.mutex);
  if (Owner() == Status::OK) {
    Destroy(ResetReason::RELEASE);
    g.egl.Release();
    g.ownerBound = false;
    g.released = true;
    g.count = 0;
    g.storage.reset();
  }
}
napi_value InitSession(napi_env e, napi_callback_info info) {
  size_t n = 1;
  napi_value a[1]{};
  napi_get_cb_info(e, info, &n, a, nullptr, nullptr);
  std::string id;
  bool valid = n == 1 && String(e, a[0], id) && !id.empty();
  std::lock_guard<std::mutex> lock(g.mutex);
  if (!valid) return Control(e, Status::INVALID_ARGUMENT);
  if (Owner() != Status::OK) return Control(e, Status::WRONG_THREAD);
  if (g.ownerBound && id != g.componentId)
    return Control(e, Status::SESSION_BUSY);
  if (id != g.componentId || g.egl.state == State::WAIT_WINDOW)
    return Control(e, Status::NOT_READY);
  if (!g.ownerBound) {
    if (g.serial >= kSafeInteger) return Control(e, Status::SESSION_BUSY);
    g.storage.reset(new (std::nothrow) FrameStorage());
    if (!g.storage) return Control(e, Status::GL_RESOURCE_FAILED);
    g.count = g.head = 0;
    g.id = ++g.serial;
    g.attempt = g.lastSwap = 0;
    g.owner = std::this_thread::get_id();
    g.ownerBound = true;
    g.released = false;
    g.egl.SetActive(true);
  }
  return Control(e, Status::OK);
}
napi_value GetState(napi_env e, napi_callback_info) {
  std::lock_guard<std::mutex> lock(g.mutex);
  Status code = Owner();
  if (code == Status::OK) g.egl.QuerySize();
  napi_value o = Control(e, code);
  Put(e, o, "widthPx", static_cast<double>(g.egl.width));
  Put(e, o, "heightPx", static_cast<double>(g.egl.height));
  Put(e, o, "sizeSource",
      g.egl.width > 0 ? "EGL_QUERY_SURFACE" : "UNAVAILABLE");
  Put(e, o, "active", g.egl.active);
  Put(e, o, "ownerBound", g.ownerBound);
  Put(e, o, "lastErrorCode", static_cast<double>(g.lastError));
  Put(e, o, "nativeOwnedBytes",
      static_cast<double>(g.storage ? sizeof(FrameStorage) : 0));
  Put(e, o, "textureLiveCount", g.uploader.Texture() ? 1.0 : 0.0);
  Put(e, o, "programLiveCount", g.shader.Program() ? 1.0 : 0.0);
  Put(e, o, "vaoLiveCount", g.shader.Vao() ? 1.0 : 0.0);
  Put(e, o, "telemetryLiveCount", static_cast<double>(g.count));
  Put(e, o, "textureAbandonCount",
      static_cast<double>(g.uploader.textureAbandonCount));
  Put(e, o, "programAbandonCount",
      static_cast<double>(g.shader.programAbandonCount));
  Put(e, o, "vaoAbandonCount", static_cast<double>(g.shader.vaoAbandonCount));
  return o;
}
napi_value SetActive(napi_env e, napi_callback_info info) {
  size_t n = 1;
  napi_value a[1]{};
  napi_get_cb_info(e, info, &n, a, nullptr, nullptr);
  bool active = false;
  napi_valuetype t;
  bool valid = n == 1 && napi_typeof(e, a[0], &t) == napi_ok &&
               t == napi_boolean &&
               napi_get_value_bool(e, a[0], &active) == napi_ok;
  std::lock_guard<std::mutex> lock(g.mutex);
  Status code = Owner();
  if (code == Status::OK) {
    if (!valid)
      code = Status::INVALID_ARGUMENT;
    else if (g.released)
      code = Status::RELEASED;
    else {
      if (!active) Destroy(ResetReason::BACKGROUND);
      code = g.egl.SetActive(active);
      if (!active && !g.egl.current) ResetGpu();
    }
  }
  return Control(e, code);
}
napi_value Release(napi_env e, napi_callback_info) {
  std::lock_guard<std::mutex> lock(g.mutex);
  Status code = Owner();
  if (code != Status::OK) return Control(e, code);
  Destroy(ResetReason::RELEASE);
  g.egl.Release();
  if (g.egl.current) return Control(e, Status::EGL_INIT_FAILED);
  g.ownerBound = false;
  g.released = true;
  g.count = 0;
  g.head = 0;
  g.storage.reset();
  return Control(e, Status::OK);
}
void Save(const Telemetry& t) {
  if (!g.storage) {
    g.dropped++;
    return;
  }
  if (g.count == kTelemetryCapacity) {
    g.head = (g.head + 1) % kTelemetryCapacity;
    g.count--;
    g.dropped++;
  }
  g.storage->ring[(g.head + g.count) % kTelemetryCapacity] = t;
  g.count++;
}
napi_value Result(napi_env e, const Telemetry& t) {
  napi_value o = Control(e, t.code);
  Put(e, o, "sessionId", t.frame.sessionId);
  Put(e, o, "surfaceEpoch", t.frame.surfaceEpoch);
  Put(e, o, "contextEpoch", t.contextEpoch);
  Put(e, o, "state", StateName(t.state));
  Put(e, o, "frameId", t.frame.frameId);
  Put(e, o, "snapshotId", t.frame.snapshotId);
  Put(e, o, "uploadedFrameId", static_cast<double>(t.uploadedFrameId));
  Put(e, o, "submittedFrameId", static_cast<double>(t.submittedFrameId));
  Put(e, o, "lastSwapFrameId", static_cast<double>(t.lastSwapFrameId));
  Put(e, o, "swapSucceeded", t.swapSucceeded);
  Put(e, o, "eglError", static_cast<double>(t.eglError));
  Put(e, o, "glError", static_cast<double>(t.glError));
  Put(e, o, "recoveryRequired", t.recoveryRequired);
  Put(e, o, "errorStage", t.errorStage);
  return o;
}
bool ParseFrame(napi_env e, napi_value o, FrameInput& f, const char*& stage) {
  napi_valuetype type;
  if (napi_typeof(e, o, &type) != napi_ok || type != napi_object) {
    stage = "FRAME_OBJECT";
    return false;
  }
  bool ok = true;
#define READ(x) do { if (!Number(e, o, #x, f.x)) { if (ok) stage = "FRAME_FIELD_" #x; ok = false; } } while (false)
  READ(sessionId);
  READ(frameId);
  READ(snapshotId);
  READ(surfaceEpoch);
  READ(fieldEpoch);
  READ(geometryEpoch);
  READ(continuityEpoch);
  READ(appearanceConfigRevision);
  READ(lightConfigRevision);
  READ(frameTimeNs);
  READ(surfaceWidthPx);
  READ(surfaceHeightPx);
  READ(boxXPx);
  READ(boxYPx);
  READ(boxWidthPx);
  READ(boxHeightPx);
  READ(capacity);
  READ(T_dataLayoutVersion);
  READ(zOriginVersion);
#undef READ
  if (!Boolean(e, o, "visualInverted", f.visualInverted)) {
    if (ok) stage = "FRAME_FIELD_visualInverted";
    ok = false;
  }
  if (!Boolean(e, o, "diagnosticEnabled", f.diagnosticEnabled)) {
    if (ok) stage = "FRAME_FIELD_diagnosticEnabled";
    ok = false;
  }
  return ok;
}
bool ParseLight(napi_env e, napi_value o, LightParams& l, const char*& stage) {
  napi_valuetype type;
  if (napi_typeof(e, o, &type) != napi_ok || type != napi_object) {
    stage = "LIGHT_OBJECT";
    return false;
  }
  bool ok = true;
#define READ(x) do { if (!Number(e, o, #x, l.x)) { if (ok) stage = "LIGHT_FIELD_" #x; ok = false; } } while (false)
  READ(la);
  READ(ld);
  READ(lmin);
  READ(lmax);
  READ(lref);
  READ(ellLightMaterial);
  READ(lightX);
  READ(lightY);
  READ(lightZ);
  READ(sigmaCandidate);
#undef READ
  return ok;
}
napi_value DrawFrame(napi_env e, napi_callback_info info) {
  const double start = NowMs();
  Telemetry t{};
  bool eglAttempted = false, drawAttempted = false;
  size_t n = 3;
  napi_value a[3]{};
  napi_get_cb_info(e, info, &n, a, nullptr, nullptr);
  FrameInput f{};
  LightParams l{};
  const char* parseStage = "FRAME_ARGUMENT_COUNT";
  bool parsed = n == 3 && ParseFrame(e, a[0], f, parseStage) && ParseLight(e, a[2], l, parseStage);
  std::string kernel;
  napi_value kv = nullptr;
  bool version =
      n == 3 && parsed &&
      napi_get_named_property(e, a[0], "slopeKernelVersion", &kv) == napi_ok &&
      String(e, kv, kernel);
  napi_typedarray_type type{};
  size_t length = 0, offset = 0;
  void* data = nullptr;
  napi_value buffer = nullptr;
  bool detached = false;
  const double copyStart = NowMs();
  bool isTyped = false;
  if (n == 3) napi_is_typedarray(e, a[1], &isTyped);
  const char* typedStage = "TDATA_NOT_TYPEDARRAY";
  bool typed = false;
  void* base = nullptr;
  size_t bytes = 0;
  if (n == 3 && isTyped) {
    if (napi_get_typedarray_info(e, a[1], &type, &length, &data, &buffer, &offset) != napi_ok)
      typedStage = "TDATA_INFO";
    else if (type != napi_float32_array) typedStage = "TDATA_TYPE";
    else if (length != kFloats) typedStage = "TDATA_LENGTH";
    else if (!data) typedStage = "TDATA_DATA";
    else if (napi_is_detached_arraybuffer(e, buffer, &detached) != napi_ok)
      typedStage = "TDATA_DETACH_QUERY";
    else if (detached) typedStage = "TDATA_DETACHED";
    else if (napi_get_arraybuffer_info(e, buffer, &base, &bytes) != napi_ok || !base)
      typedStage = "TDATA_BUFFER_INFO";
    else if (offset > bytes || kFloats * sizeof(float) > bytes - offset)
      typedStage = "TDATA_BUFFER_RANGE";
    else if (offset % sizeof(float) != 0) typedStage = "TDATA_OFFSET";
    else if (data != static_cast<unsigned char*>(base) + offset) typedStage = "TDATA_POINTER";
    else typed = true;
  }
  const double lockStart = NowMs();
  std::unique_lock<std::mutex> lock(g.mutex);
  t.lifecycleLockWaitMs = NowMs() - lockStart;
  t.frame = f;
  t.light = l;
  t.inputLight = l;
  t.ownerThreadId = static_cast<uint64_t>(syscall(SYS_gettid));
  Status code = Owner();
  if (code == Status::OK && !g.ownerBound)
    code = g.released ? Status::RELEASED : Status::NOT_READY;
  if (code == Status::OK && f.sessionId != g.id) code = Status::RELEASED;
  if (code == Status::OK) {
    if (!SafeInteger(f.frameId) || f.frameId < 1)
      code = Status::INVALID_ARGUMENT;
    else if (f.frameId <= g.attempt)
      code = Status::STALE_FRAME;
    else {
      t.frameGap = f.frameId - g.attempt - 1;
      g.attempt = static_cast<uint64_t>(f.frameId);
    }
  }
  if (code == Status::OK && (!parsed || !typed || !version)) {
    code = Status::INVALID_ARGUMENT;
    t.errorStage = !parsed ? parseStage : !typed ? typedStage : "SLOPE_KERNEL_FIELD";
  }
  if (code == Status::OK && kernel != "E2_APPROX_P0") {
    code = Status::VERSION_MISMATCH;
    t.errorStage = "SLOPE_KERNEL_VERSION";
  }
  if (code == Status::OK) {
    const double ct = NowMs();
    std::memcpy(g.storage->staging.data(), data, kFloats * sizeof(float));
    t.bridgeCopyMs = NowMs() - ct;
    t.tdataHash = 14695981039346656037ULL;
    const auto* bytes =
        reinterpret_cast<const unsigned char*>(g.storage->staging.data());
    for (size_t i = 0; i < sizeof(g.storage->staging); i++) {
      t.tdataHash ^= bytes[i];
      t.tdataHash *= 1099511628211ULL;
    }
    const double vt = NowMs();
    code = Validate(f, l, g.storage->staging);
    if (code != Status::OK) t.errorStage = "FRAME_VALIDATE";
    t.bridgeValidateMs = NowMs() - vt;
  }
  const double readValidateMs = NowMs() - copyStart - t.lifecycleLockWaitMs;
  if (code == Status::OK && !g.egl.active) code = Status::INACTIVE;
  if (code == Status::OK) {
    const double norm = std::hypot(std::hypot(l.lightX, l.lightY), l.lightZ);
    l.lightX /= norm;
    l.lightY /= norm;
    l.lightZ /= norm;
    t.light = l;
    eglAttempted = true;
    SurfaceInfo si{};
    code = g.egl.EnsureCurrent({static_cast<uint64_t>(f.surfaceEpoch),
                                static_cast<int>(f.surfaceWidthPx),
                                static_cast<int>(f.surfaceHeightPx)},
                               si);
    if (code == Status::OK) {
      if (g.shader.epoch != g.egl.contextEpoch) {
        ResetGpu();
        code =
            g.shader.Create({kP013Vertex, kP013Fragment}, g.egl.contextEpoch);
        if (code == Status::OK) code = g.uploader.Create(g.egl.contextEpoch);
        t.shaderBuildMs = g.shader.buildMs;
      }
      if (code == Status::OK && f.diagnosticEnabled) {
        code = g.uploader.Upload(g.storage->staging.data(),
                                 static_cast<uint64_t>(f.frameId));
        t.uploadCpuMs = g.uploader.uploadMs;
        t.glError = g.uploader.error;
        if (code == Status::OK) t.uploadedFrameId = f.frameId;
      }
      if (code == Status::OK) {
        drawAttempted = true;
        code = g.shader.Draw({f, l}, g.uploader.Texture());
        t.glError = g.shader.error;
        if (code == Status::OK) t.submittedFrameId = f.frameId;
      }
      if (code == Status::OK) {
        code = g.egl.Swap(static_cast<uint64_t>(f.frameId));
        t.swapSucceeded = code == Status::OK;
      }
    }
    const Status unbound = g.egl.Unbind();
    if (unbound != Status::OK) code = unbound;
  }
  if (code == Status::OK && t.swapSucceeded)
    g.lastSwap = static_cast<uint64_t>(f.frameId);
  t.recoveryRequired = static_cast<int>(code) >= 200;
  if (t.recoveryRequired) {
    Destroy(ResetReason::FAILURE);
    g.egl.state = State::FAILED;
  }
  t.state = g.egl.state;
  t.compileLogHash = g.shader.compileLogHash;
  t.contextEpoch = g.egl.contextEpoch;
  t.lastSwapFrameId = g.lastSwap;
  t.code = code;
  g.lastError = code;
  if (eglAttempted) {
    t.eglError = g.egl.error;
    t.errorStage = g.egl.errorStage;
    t.eglInitMs = g.egl.initMs;
    t.makeCurrentMs = g.egl.currentMs;
    t.unbindMs = g.egl.unbindMs;
    t.swapMs = g.egl.swapMs;
  }
  if (drawAttempted) {
    t.uniformCpuMs = g.shader.uniformMs;
    t.drawCpuMs = g.shader.drawMs;
  }
  if (code != Status::OK && std::strcmp(t.errorStage, "NONE") == 0)
    t.errorStage = "FRAME_REJECTED";
  if (code == Status::INVALID_ARGUMENT && parsed && typed && version &&
      (l.lref - l.lmin > 1 || l.lmax - l.lref > 1))
    t.errorStage = "DIAG_RANGE";
  t.uploadPathMs = readValidateMs + t.uploadCpuMs;
  t.uploadCount = g.uploader.uploadCount;
  t.textureAllocCount = g.uploader.textureAllocCount;
  t.textureAbandonCount = g.uploader.textureAbandonCount;
  t.programAbandonCount = g.shader.programAbandonCount;
  t.vaoAbandonCount = g.shader.vaoAbandonCount;
  t.textureLiveCount = g.uploader.Texture() ? 1 : 0;
  t.programLiveCount = g.shader.Program() ? 1 : 0;
  t.vaoLiveCount = g.shader.Vao() ? 1 : 0;
  t.surfaceCreateCount = g.egl.surfaceCreateCount;
  t.surfaceDestroyCount = g.egl.surfaceDestroyCount;
  t.contextCreateCount = g.egl.contextCreateCount;
  t.contextDestroyCount = g.egl.contextDestroyCount;
  t.contextLossCount = g.egl.contextLossCount;
  t.recoveryAttemptCount = g.egl.recoveryAttemptCount;
  t.staleLifecycleCount = g.egl.staleLifecycleCount;
  napi_value result = Result(e, t);
  t.bridgeCpuMs = NowMs() - start;
  Save(t);
  return result;
}
napi_value Drain(napi_env e, napi_callback_info) {
  std::lock_guard<std::mutex> lock(g.mutex);
  if (Owner() != Status::OK) return Control(e, Status::WRONG_THREAD);
  napi_value batch = Control(e, Status::OK), rows = nullptr;
  napi_create_array_with_length(e, g.count, &rows);
  for (size_t i = 0; i < g.count; i++) {
    const auto& t = g.storage->ring[(g.head + i) % kTelemetryCapacity];
    napi_value o = Result(e, t);
#define METRIC(x) Put(e, o, #x, static_cast<double>(t.x))
    METRIC(bridgeValidateMs);
    METRIC(bridgeCopyMs);
    METRIC(bridgeCpuMs);
    METRIC(uploadCpuMs);
    METRIC(uploadPathMs);
    METRIC(eglInitMs);
    METRIC(makeCurrentMs);
    METRIC(unbindMs);
    METRIC(swapMs);
    METRIC(lifecycleLockWaitMs);
    METRIC(shaderBuildMs);
    METRIC(uniformCpuMs);
    METRIC(drawCpuMs);
    METRIC(ownerThreadId);
    METRIC(frameGap);
    METRIC(uploadCount);
    METRIC(textureAllocCount);
    METRIC(textureAbandonCount);
    METRIC(programAbandonCount);
    METRIC(vaoAbandonCount);
    METRIC(textureLiveCount);
    METRIC(programLiveCount);
    METRIC(vaoLiveCount);
    METRIC(surfaceCreateCount);
    METRIC(surfaceDestroyCount);
    METRIC(contextCreateCount);
    METRIC(contextDestroyCount);
    METRIC(contextLossCount);
    METRIC(recoveryAttemptCount);
    METRIC(staleLifecycleCount);
#undef METRIC
#define FRAME(x) Put(e, o, #x, t.frame.x)
    FRAME(sessionId);
    FRAME(frameId);
    FRAME(snapshotId);
    FRAME(surfaceEpoch);
    FRAME(fieldEpoch);
    FRAME(geometryEpoch);
    FRAME(continuityEpoch);
    FRAME(appearanceConfigRevision);
    FRAME(lightConfigRevision);
    FRAME(frameTimeNs);
    FRAME(T_dataLayoutVersion);
    FRAME(zOriginVersion);
    FRAME(surfaceWidthPx);
    FRAME(surfaceHeightPx);
    FRAME(boxXPx);
    FRAME(boxYPx);
    FRAME(boxWidthPx);
    FRAME(boxHeightPx);
    FRAME(capacity);
    FRAME(visualInverted);
    FRAME(diagnosticEnabled);
#undef FRAME
    Put(e, o, "contextEpoch", t.contextEpoch);
    Put(e, o, "schemaVersion", 1.0);
    Put(e, o, "slopeKernelVersion", "E2_APPROX_P0");
    Put(e, o, "mode",
        t.frame.diagnosticEnabled ? "GLES_LMAT_DIAG_ON" : "GLES_LMAT_DIAG_OFF");
    Put(e, o, "diagEncoding", "DIAG_GRAY_V1");
    Put(e, o, "profileKernel", "DISPLAY_4X3_V1");
    Put(e, o, "maskRevision", "P013_CONTINUOUS_SAND_V1");
    Put(e, o, "lightFrame", "CONTAINER_PRE_INVERT_V1");
    Put(e, o, "shaderHash", kP013ShaderHash);
    Put(e, o, "errorStage", t.errorStage);
    char hash[24]{};
    std::snprintf(hash, sizeof(hash), "%016llx",
                  static_cast<unsigned long long>(t.tdataHash));
    Put(e, o, "T_dataHash", hash);
    std::snprintf(hash, sizeof(hash), "%016llx",
                  static_cast<unsigned long long>(t.compileLogHash));
    Put(e, o, "compileLogHash", hash);
    Put(e, o, "uploadBytes", t.uploadedFrameId ? 2048.0 : 0.0);
    Put(e, o, "texturePayloadBytes", t.textureLiveCount ? 2048.0 : 0.0);
    Put(e, o, "nativeOwnedBytes",
        static_cast<double>(g.storage ? sizeof(FrameStorage) : 0));
    Put(e, o, "gpuTimerValid", false);
    Put(e, o, "gpuDisjoint", false);
    napi_value nullValue = nullptr;
    napi_get_null(e, &nullValue);
    napi_set_named_property(e, o, "gpuPassMs", nullValue);
    Put(e, o, "gpuQueryPendingCount", 0.0);
    napi_value light = Object(e);
#define LIGHT(x) Put(e, light, #x, t.inputLight.x)
    LIGHT(la);
    LIGHT(ld);
    LIGHT(lmin);
    LIGHT(lmax);
    LIGHT(lref);
    LIGHT(ellLightMaterial);
    LIGHT(lightX);
    LIGHT(lightY);
    LIGHT(lightZ);
    LIGHT(sigmaCandidate);
#undef LIGHT
    napi_set_named_property(e, o, "lightConfig", light);
    napi_value normalized = Object(e);
    Put(e, normalized, "lightX", t.light.lightX);
    Put(e, normalized, "lightY", t.light.lightY);
    Put(e, normalized, "lightZ", t.light.lightZ);
    napi_set_named_property(e, o, "normalizedLightDirection", normalized);
    Put(e, o, "gpuTimerStatus", "UNMEASURED_USE_SYSTEM_TRACE");
    napi_set_element(e, rows, i, o);
  }
  napi_set_named_property(e, batch, "records", rows);
  Put(e, batch, "droppedTelemetry", static_cast<double>(g.dropped));
  Put(e, batch, "telemetryLiveCount", static_cast<double>(g.count));
  g.head = g.count = 0;
  return batch;
}
}  // namespace
}  // namespace p013
EXTERN_C_START
static napi_value Init(napi_env e, napi_value exports) {
  using namespace p013;
  napi_property_descriptor d[] = {{"init", nullptr, InitSession, nullptr,
                                   nullptr, nullptr, napi_default, nullptr},
                                  {"getState", nullptr, GetState, nullptr,
                                   nullptr, nullptr, napi_default, nullptr},
                                  {"setActive", nullptr, SetActive, nullptr,
                                   nullptr, nullptr, napi_default, nullptr},
                                  {"drawFrame", nullptr, DrawFrame, nullptr,
                                   nullptr, nullptr, napi_default, nullptr},
                                  {"release", nullptr, Release, nullptr,
                                   nullptr, nullptr, napi_default, nullptr},
                                  {"drainTelemetry", nullptr, Drain, nullptr,
                                   nullptr, nullptr, napi_default, nullptr}};
  napi_define_properties(e, exports, sizeof(d) / sizeof(d[0]), d);
  Hook(e, exports);
  napi_add_env_cleanup_hook(e, Cleanup, nullptr);
  return exports;
}
EXTERN_C_END
static napi_module module = {1, 0, nullptr, Init, "p013renderer", nullptr, {0}};
extern "C" __attribute__((constructor)) void RegisterP013Renderer() {
  napi_module_register(&module);
}
