/*
 * P-013 P0a hello-surface：XComponent + EGL 初始化 + 最小渲染帧
 * 问题 ID：P-013；方案版本：proposal-P013-incorporation-v1 §2.0/§2.1/§2.2
 *
 * 独立诊断页（§2.3-c）：不接 SandCanvas 主页面，零 Canvas 链改动。
 * 验收（§2.0）：XComponent 区域显示固定清屏色（深灰）；eglSwapBuffers
 * 返回 EGL_TRUE（hilog 埋点）；前后台切换/旋转不崩溃（surface 重建路径）。
 */
#include <napi/native_api.h>
#include <ace/xcomponent/native_interface_xcomponent.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <hilog/log.h>

#include <string>

#define LOG_DOMAIN_P013 0x013
#define LOG_TAG_P013 "P013Hello"

#define HILOG_INFO(fmt, ...) \
  OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN_P013, LOG_TAG_P013, fmt, ##__VA_ARGS__)
#define HILOG_ERROR(fmt, ...) \
  OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_DOMAIN_P013, LOG_TAG_P013, fmt, ##__VA_ARGS__)

namespace {

// ── EGL 状态（§2.2 生命周期：display/config/context/surface + 丢失重建） ──
EGLDisplay g_display = EGL_NO_DISPLAY;
EGLConfig g_config = nullptr;
EGLContext g_context = EGL_NO_CONTEXT;
EGLSurface g_surface = EGL_NO_SURFACE;
NativeWindow *g_nativeWindow = nullptr;
uint64_t g_width = 0;
uint64_t g_height = 0;
int64_t g_frameCount = 0;
bool g_ready = false;

// 清屏测试色（深灰——立项书 §2.0「指定清屏色」）
const float HELLO_COLOR_R = 0.18f;
const float HELLO_COLOR_G = 0.20f;
const float HELLO_COLOR_B = 0.22f;
const float HELLO_COLOR_A = 1.0f;

const char *EglErrorString(EGLint err) {
  switch (err) {
    case EGL_SUCCESS: return "EGL_SUCCESS";
    case EGL_NOT_INITIALIZED: return "EGL_NOT_INITIALIZED";
    case EGL_BAD_ACCESS: return "EGL_BAD_ACCESS";
    case EGL_BAD_ALLOC: return "EGL_BAD_ALLOC";
    case EGL_BAD_ATTRIBUTE: return "EGL_BAD_ATTRIBUTE";
    case EGL_BAD_CONFIG: return "EGL_BAD_CONFIG";
    case EGL_BAD_CONTEXT: return "EGL_BAD_CONTEXT";
    case EGL_BAD_CURRENT_SURFACE: return "EGL_BAD_CURRENT_SURFACE";
    case EGL_BAD_DISPLAY: return "EGL_BAD_DISPLAY";
    case EGL_BAD_MATCH: return "EGL_BAD_MATCH";
    case EGL_BAD_NATIVE_PIXMAP: return "EGL_BAD_NATIVE_PIXMAP";
    case EGL_BAD_NATIVE_WINDOW: return "EGL_BAD_NATIVE_WINDOW";
    case EGL_BAD_PARAMETER: return "EGL_BAD_PARAMETER";
    case EGL_BAD_SURFACE: return "EGL_BAD_SURFACE";
    case EGL_CONTEXT_LOST: return "EGL_CONTEXT_LOST";
    default: return "EGL_UNKNOWN";
  }
}

bool CheckEgl(const char *step) {
  EGLint err = eglGetError();
  if (err != EGL_SUCCESS) {
    HILOG_ERROR("EGL step %{public}s failed: %{public}s (0x%{public}x)",
                step, EglErrorString(err), err);
    return false;
  }
  return true;
}

// ── EGL 初始化链（§2.2：eglGetDisplay/Initialize/ChooseConfig/Context/Surface） ──
bool InitEgl(NativeWindow *window) {
  g_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  if (g_display == EGL_NO_DISPLAY) {
    HILOG_ERROR("eglGetDisplay failed");
    return false;
  }
  if (!eglInitialize(g_display, nullptr, nullptr)) {
    CheckEgl("eglInitialize");
    return false;
  }
  // RGBA8888 + EGL_WINDOW_BIT（§2.2 配置要求）
  const EGLint attribs[] = {
    EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
    EGL_RED_SIZE, 8,
    EGL_GREEN_SIZE, 8,
    EGL_BLUE_SIZE, 8,
    EGL_ALPHA_SIZE, 8,
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
    EGL_NONE
  };
  EGLint numConfigs = 0;
  if (!eglChooseConfig(g_display, attribs, &g_config, 1, &numConfigs) || numConfigs < 1) {
    CheckEgl("eglChooseConfig");
    return false;
  }
  const EGLint ctxAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
  g_context = eglCreateContext(g_display, g_config, EGL_NO_CONTEXT, ctxAttribs);
  if (g_context == EGL_NO_CONTEXT) {
    CheckEgl("eglCreateContext");
    return false;
  }
  g_surface = eglCreateWindowSurface(g_display, g_config,
      reinterpret_cast<EGLNativeWindowType>(window), nullptr);
  if (g_surface == EGL_NO_SURFACE) {
    CheckEgl("eglCreateWindowSurface");
    return false;
  }
  if (!eglMakeCurrent(g_display, g_surface, g_surface, g_context)) {
    CheckEgl("eglMakeCurrent");
    return false;
  }
  HILOG_INFO("EGL init ok (GLES3); vendor=%{public}s",
             glGetString(GL_VENDOR) != nullptr
               ? reinterpret_cast<const char *>(glGetString(GL_VENDOR)) : "?");
  return true;
}

// 全 GPU 资源释放（§2.2 SurfaceDestroyed → 重建路径；hello 帧无纹理，仅上下文链）
void ReleaseEgl() {
  g_ready = false;
  if (g_display != EGL_NO_DISPLAY) {
    eglMakeCurrent(g_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (g_surface != EGL_NO_SURFACE) {
      eglDestroySurface(g_display, g_surface);
      g_surface = EGL_NO_SURFACE;
    }
    if (g_context != EGL_NO_CONTEXT) {
      eglDestroyContext(g_display, g_context);
      g_context = EGL_NO_CONTEXT;
    }
    eglTerminate(g_display);
    g_display = EGL_NO_DISPLAY;
  }
  HILOG_INFO("EGL released (frames=%{public}lld)", static_cast<long long>(g_frameCount));
}

// 最小 hello 帧（§2.0：清屏为固定测试色 + eglSwapBuffers）
void DrawHelloFrame() {
  glViewport(0, 0, static_cast<GLsizei>(g_width), static_cast<GLsizei>(g_height));
  glClearColor(HELLO_COLOR_R, HELLO_COLOR_G, HELLO_COLOR_B, HELLO_COLOR_A);
  glClear(GL_COLOR_BUFFER_BIT);
  EGLBoolean ok = eglSwapBuffers(g_display, g_surface);
  g_frameCount++;
  if (ok != EGL_TRUE) {
    CheckEgl("eglSwapBuffers");
    return;
  }
  // 验收②埋点：每 300 帧一次健康心跳（避免逐帧刷日志）
  if (g_frameCount % 300 == 1) {
    HILOG_INFO("hello frame #%{public}lld swap ok (%{public}llux%{public}llu)",
               static_cast<long long>(g_frameCount),
               static_cast<unsigned long long>(g_width),
               static_cast<unsigned long long>(g_height));
  }
}

// ── OH_NativeXComponent 回调（§2.1/§2.2） ──
void OnSurfaceCreated(OH_NativeXComponent *component, void *window) {
  (void)component;
  g_nativeWindow = static_cast<NativeWindow *>(window);
  if (!InitEgl(g_nativeWindow)) {
    HILOG_ERROR("OnSurfaceCreated: EGL init failed");
    ReleaseEgl();
    return;
  }
  OH_NativeXComponent_GetXComponentSize(component, window, &g_width, &g_height);
  g_ready = true;
  HILOG_INFO("OnSurfaceCreated: %{public}llux%{public}llu",
             static_cast<unsigned long long>(g_width),
             static_cast<unsigned long long>(g_height));
}

void OnSurfaceChanged(OH_NativeXComponent *component, void *window) {
  (void)window;
  if (!g_ready) {
    return;
  }
  // §2.2 尺寸变化：重查 + viewport 更新（下一帧 DrawHelloFrame 生效）
  OH_NativeXComponent_GetXComponentSize(component, g_nativeWindow, &g_width, &g_height);
  HILOG_INFO("OnSurfaceChanged: %{public}llux%{public}llu",
             static_cast<unsigned long long>(g_width),
             static_cast<unsigned long long>(g_height));
}

void OnSurfaceDestroyed(OH_NativeXComponent *component, void *window) {
  (void)component;
  (void)window;
  HILOG_INFO("OnSurfaceDestroyed");
  ReleaseEgl();
  g_nativeWindow = nullptr;
}

// ── napi 桥（§2.1 骨架最小集：XComponent 回调挂接 + drawFrame 驱动 + 诊断只读钩子） ──

// XComponent 回调挂接（libace_napi_arkui 约定：Init 第二参 exports 上带
// __NATIVE_XCOMPONENT_OBJ__ 属性 = OH_NativeXComponent*，由 ArkTS 侧
// XComponent(libbackground: p013hello) 的 libname 触发）
void HookXComponent(napi_env env, napi_value exports) {
  napi_value exportInstance = nullptr;
  if (napi_get_named_property(env, exports, OH_NATIVE_XCOMPONENT_OBJ, &exportInstance) != napi_ok ||
      exportInstance == nullptr) {
    HILOG_INFO("no __NATIVE_XCOMPONENT_OBJ__ on exports（非 XComponent 加载路径）");
    return;
  }
  OH_NativeXComponent *nativeXComponent = nullptr;
  if (napi_unwrap(env, exportInstance,
                  reinterpret_cast<void **>(&nativeXComponent)) != napi_ok ||
      nativeXComponent == nullptr) {
    HILOG_ERROR("napi_unwrap OH_NativeXComponent failed");
    return;
  }
  static OH_NativeXComponent_Callback cb;
  cb.OnSurfaceCreated = OnSurfaceCreated;
  cb.OnSurfaceChanged = OnSurfaceChanged;
  cb.OnSurfaceDestroyed = OnSurfaceDestroyed;
  char id[OH_XCOMPONENT_ID_LEN_MAX + 1] = {};
  uint64_t idLen = OH_XCOMPONENT_ID_LEN_MAX + 1;
  OH_NativeXComponent_GetXComponentId(nativeXComponent, id, &idLen);
  if (OH_NativeXComponent_RegisterCallback(nativeXComponent, &cb) != 0) {
    HILOG_ERROR("RegisterCallback failed (id=%{public}s)", id);
    return;
  }
  HILOG_INFO("XComponent callbacks hooked (id=%{public}s)", id);
}

napi_value DrawFrame(napi_env env, napi_callback_info info) {
  (void)info;
  napi_value result = nullptr;
  bool ok = false;
  if (g_ready) {
    DrawHelloFrame();
    ok = true;
  }
  napi_get_boolean(env, ok, &result);
  return result;
}

napi_value GetFrameCount(napi_env env, napi_callback_info info) {
  (void)info;
  napi_value result = nullptr;
  napi_create_int64(env, g_frameCount, &result);
  return result;
}

napi_value IsReady(napi_env env, napi_callback_info info) {
  (void)info;
  napi_value result = nullptr;
  napi_get_boolean(env, g_ready, &result);
  return result;
}

} // namespace

// 模块注册：DevEco 标准 napi 流程（§2.1）
EXTERN_C_START
static napi_value Init(napi_env env, napi_value exports) {
  napi_property_descriptor desc[] = {
    { "drawFrame", nullptr, DrawFrame, nullptr, nullptr, nullptr, napi_default, nullptr },
    { "getFrameCount", nullptr, GetFrameCount, nullptr, nullptr, nullptr, napi_default, nullptr },
    { "isReady", nullptr, IsReady, nullptr, nullptr, nullptr, napi_default, nullptr },
  };
  napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
  HookXComponent(env, exports);
  return exports;
}
EXTERN_C_END

static napi_module g_p013helloModule = {
  .nm_version = 1,
  .nm_flags = 0,
  .nm_filename = nullptr,
  .nm_register_func = Init,
  .nm_modname = "p013hello",
  .nm_priv = nullptr,
  .reserved = { 0 },
};

extern "C" __attribute__((constructor)) void RegisterP013HelloModule(void) {
  napi_module_register(&g_p013helloModule);
}
// XComponent 回调挂接见 HookXComponent（libace_napi_arkui 的
// __NATIVE_XCOMPONENT_OBJ__ 约定——ArkTS 侧 XComponent libname=p013hello
// 触发本模块 Init，exports 上带 OH_NativeXComponent 指针）
