// P-013 P0 v1/P0a compatibility shell. EGL chain is shared with the renderer.
#include <GLES3/gl3.h>
#include <hilog/log.h>
#include <napi/native_api.h>

#include <mutex>
#include <thread>

#include "../p013renderer/egl_context.h"
namespace {
p013::EglContext egl;
std::mutex mutex;
std::thread::id owner;
bool bound = false;
int64_t frames = 0;
uint64_t loggedEpoch = 0;
void Created(OH_NativeXComponent* c, void* w) {
  std::lock_guard<std::mutex> l(mutex);
  egl.OnWindowCreated(c, w);
  OH_LOG_Print(
      LOG_APP, LOG_INFO, 0x013, "P013Hello",
      "OnSurfaceCreated: window available; EGL creation deferred to draw");
}
void Changed(OH_NativeXComponent* c, void* w) {
  std::lock_guard<std::mutex> l(mutex);
  egl.OnWindowChanged(c, w);
  OH_LOG_Print(LOG_APP, LOG_INFO, 0x013, "P013Hello", "OnSurfaceChanged");
}
void Destroyed(OH_NativeXComponent* c, void* w) {
  std::lock_guard<std::mutex> l(mutex);
  egl.OnWindowDestroyed(c, w);
  OH_LOG_Print(LOG_APP, LOG_INFO, 0x013, "P013Hello", "OnSurfaceDestroyed");
  if (!egl.current)
    OH_LOG_Print(LOG_APP, LOG_INFO, 0x013, "P013Hello", "EGL released");
}
OH_NativeXComponent_Callback callbacks{Created, Changed, Destroyed, nullptr};
napi_value Bool(napi_env e, bool b) {
  napi_value v = nullptr;
  napi_get_boolean(e, b, &v);
  return v;
}
napi_value Draw(napi_env e, napi_callback_info) {
  std::lock_guard<std::mutex> l(mutex);
  if (bound && owner != std::this_thread::get_id()) return Bool(e, false);
  if (!bound) {
    owner = std::this_thread::get_id();
    bound = true;
  }
  p013::SurfaceInfo si{};
  auto code = egl.EnsureCurrent({egl.surfaceEpoch, egl.width, egl.height}, si);
  if (code == p013::Status::STALE_SURFACE && egl.width > 0)
    code = egl.EnsureCurrent({egl.surfaceEpoch, egl.width, egl.height}, si);
  bool ok = false;
  if (code == p013::Status::OK) {
    if (loggedEpoch != egl.contextEpoch) {
      loggedEpoch = egl.contextEpoch;
      const auto* vendor = glGetString(GL_VENDOR);
      OH_LOG_Print(LOG_APP, LOG_INFO, 0x013, "P013Hello",
                   "EGL init ok (GLES3); vendor=%{public}s",
                   vendor ? reinterpret_cast<const char*>(vendor) : "?");
    }
    glViewport(0, 0, si.width, si.height);
    glClearColor(0.18f, 0.20f, 0.22f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    code = egl.Swap(frames + 1);
    ok = code == p013::Status::OK;
    if (ok) {
      frames++;
      if (frames % 300 == 1)
        OH_LOG_Print(
            LOG_APP, LOG_INFO, 0x013, "P013Hello",
            "hello frame #%{public}lld swap ok (%{public}dx%{public}d)",
            static_cast<long long>(frames), si.width, si.height);
    }
  }
  if (egl.Unbind() != p013::Status::OK) ok = false;
  return Bool(e, ok);
}
napi_value Ready(napi_env e, napi_callback_info) {
  std::lock_guard<std::mutex> l(mutex);
  return Bool(e, egl.active && egl.state == p013::State::READY);
}
napi_value Count(napi_env e, napi_callback_info) {
  std::lock_guard<std::mutex> l(mutex);
  napi_value v = nullptr;
  napi_create_int64(e, frames, &v);
  return v;
}
napi_value Active(napi_env e, napi_callback_info info) {
  size_t n = 1;
  napi_value a[1]{};
  bool active = false;
  napi_get_cb_info(e, info, &n, a, nullptr, nullptr);
  if (n != 1 || napi_get_value_bool(e, a[0], &active) != napi_ok)
    return Bool(e, false);
  std::lock_guard<std::mutex> l(mutex);
  if (bound && owner != std::this_thread::get_id()) return Bool(e, false);
  return Bool(e, egl.SetActive(active) == p013::Status::OK);
}
void Cleanup(void*) {
  std::lock_guard<std::mutex> l(mutex);
  egl.Release();
}
void Hook(napi_env e, napi_value exports) {
  bool has = false;
  napi_has_named_property(e, exports, OH_NATIVE_XCOMPONENT_OBJ, &has);
  if (!has) return;
  napi_value v = nullptr;
  OH_NativeXComponent* c = nullptr;
  if (napi_get_named_property(e, exports, OH_NATIVE_XCOMPONENT_OBJ, &v) ==
          napi_ok &&
      napi_unwrap(e, v, reinterpret_cast<void**>(&c)) == napi_ok && c)
    OH_NativeXComponent_RegisterCallback(c, &callbacks);
}
}  // namespace
EXTERN_C_START
static napi_value Init(napi_env e, napi_value exports) {
  napi_property_descriptor d[] = {{"drawFrame", nullptr, Draw, nullptr, nullptr,
                                   nullptr, napi_default, nullptr},
                                  {"getFrameCount", nullptr, Count, nullptr,
                                   nullptr, nullptr, napi_default, nullptr},
                                  {"isReady", nullptr, Ready, nullptr, nullptr,
                                   nullptr, napi_default, nullptr},
                                  {"setActive", nullptr, Active, nullptr,
                                   nullptr, nullptr, napi_default, nullptr}};
  napi_define_properties(e, exports, sizeof(d) / sizeof(d[0]), d);
  Hook(e, exports);
  napi_add_env_cleanup_hook(e, Cleanup, nullptr);
  return exports;
}
EXTERN_C_END
static napi_module module = {1, 0, nullptr, Init, "p013hello", nullptr, {0}};
extern "C" __attribute__((constructor)) void RegisterP013HelloModule() {
  napi_module_register(&module);
}
