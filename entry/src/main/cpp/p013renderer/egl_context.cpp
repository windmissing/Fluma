#include "egl_context.h"
namespace p013 {
void EglContext::Hint(OH_NativeXComponent* c, void* w) {
  // Hints never become authoritative frame dimensions.
  uint64_t x = 0, y = 0;
  OH_NativeXComponent_GetXComponentSize(c, w, &x, &y);
}
Status EglContext::OnWindowCreated(OH_NativeXComponent* c, void* w) {
  if (current) return Status::EGL_INIT_FAILED;
  if (!w) return Status::NOT_READY;
  if (window == w) return Status::OK;
  DestroyUnbound(ResetReason::DESTROYED);
  window = w;
  surfaceEpoch++;
  attemptedEpoch = UINT64_MAX;
  badWindow = false;
  recovered = false;
  Hint(c, w);
  state = active ? State::WINDOW_AVAILABLE : State::SUSPENDED;
  return Status::OK;
}
Status EglContext::OnWindowChanged(OH_NativeXComponent* c, void* w) {
  if (current) return Status::EGL_INIT_FAILED;
  if (w != window || !window) {
    staleLifecycleCount++;
    return Status::STALE_SURFACE;
  }
  Hint(c, w);
  surfaceEpoch++;
  badWindow = false;
  attemptedEpoch = UINT64_MAX;
  recovered = false;
  QuerySize();
  state = active ? State::WINDOW_AVAILABLE : State::SUSPENDED;
  return Status::OK;
}
void EglContext::OnWindowDestroyed(OH_NativeXComponent*, void* w) {
  if (w != window || !window) {
    staleLifecycleCount++;
    return;
  }
  if (current) {
    // An unbind failure cannot extend the native window's borrowed lifetime.
    window = nullptr;
    width = height = 0;
    surfaceEpoch++;
    badWindow = true;
    state = State::FAILED;
    return;
  }
  DestroyUnbound(ResetReason::DESTROYED);
  window = nullptr;
  width = height = 0;
  surfaceEpoch++;
  state = State::WAIT_WINDOW;
}
void EglContext::QuerySize() {
  if (surface == EGL_NO_SURFACE) return;
  EGLint x = 0, y = 0;
  if (eglQuerySurface(display, surface, EGL_WIDTH, &x) &&
      eglQuerySurface(display, surface, EGL_HEIGHT, &y)) {
    width = x;
    height = y;
  } else {
    error = eglGetError();
    errorStage = "querySurface";
    width = height = 0;
  }
}
Status EglContext::Failure(const char* stage, Status fallback) {
  error = eglGetError();
  errorStage = stage;
  if (error == EGL_CONTEXT_LOST) {
    contextLossCount++;
    fallback = Status::CONTEXT_LOST;
  }
  if (error == EGL_BAD_SURFACE || error == EGL_BAD_NATIVE_WINDOW) {
    badWindow = true;
    fallback = Status::BAD_SURFACE;
  }
  DestroyUnbound(ResetReason::FAILURE);
  state = State::FAILED;
  // Only one automatic context-loss recovery in this surface generation.
  if (fallback == Status::CONTEXT_LOST && !recovered) {
    recovered = true;
    attemptedEpoch = UINT64_MAX;
    recoveryAttemptCount++;
  }
  return fallback;
}
Status EglContext::EnsureCurrent(const SurfaceKey& key, SurfaceInfo& info) {
  if (current) return Status::EGL_INIT_FAILED;
  initMs = currentMs = unbindMs = swapMs = 0;
  error = EGL_SUCCESS;
  errorStage = "NONE";
  if (!active) return Status::INACTIVE;
  if (!window || badWindow) return Status::NOT_READY;
  if (key.epoch != surfaceEpoch) return Status::STALE_SURFACE;
  const double t = NowMs();
  if (context == EGL_NO_CONTEXT) {
    if (attemptedEpoch == surfaceEpoch) return Status::EGL_INIT_FAILED;
    attemptedEpoch = surfaceEpoch;
    display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display == EGL_NO_DISPLAY)
      return Failure("getDisplay", Status::EGL_INIT_FAILED);
    if (!eglInitialize(display, nullptr, nullptr))
      return Failure("initialize", Status::EGL_INIT_FAILED);
    const EGLint attrs[] = {EGL_SURFACE_TYPE,
                            EGL_WINDOW_BIT,
                            EGL_RED_SIZE,
                            8,
                            EGL_GREEN_SIZE,
                            8,
                            EGL_BLUE_SIZE,
                            8,
                            EGL_ALPHA_SIZE,
                            8,
                            EGL_RENDERABLE_TYPE,
                            EGL_OPENGL_ES3_BIT,
                            EGL_NONE};
    EGLint count = 0;
    if (!eglChooseConfig(display, attrs, &config, 1, &count) || count < 1)
      return Failure("chooseConfig", Status::EGL_INIT_FAILED);
    if (!eglBindAPI(EGL_OPENGL_ES_API))
      return Failure("bindAPI", Status::EGL_INIT_FAILED);
    const EGLint ca[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    context = eglCreateContext(display, config, EGL_NO_CONTEXT, ca);
    if (context == EGL_NO_CONTEXT)
      return Failure("createContext", Status::EGL_INIT_FAILED);
    contextCreateCount++;
    surface = eglCreateWindowSurface(
        display, config, reinterpret_cast<EGLNativeWindowType>(window),
        nullptr);
    if (surface == EGL_NO_SURFACE)
      return Failure("createSurface", Status::EGL_INIT_FAILED);
    surfaceCreateCount++;
    contextEpoch++;
    initMs = NowMs() - t;
  }
  const double mt = NowMs();
  if (!eglMakeCurrent(display, surface, surface, context))
    return Failure("makeCurrent", Status::EGL_INIT_FAILED);
  current = true;
  currentOwner = std::this_thread::get_id();
  currentMs = NowMs() - mt;
  QuerySize();
  if (width <= 0 || height <= 0) {
    Unbind();
    state = State::SUSPENDED;
    return Status::NOT_READY;
  }
  info = {width, height, surfaceEpoch};
  if (key.width != width || key.height != height) {
    Unbind();
    return Status::STALE_SURFACE;
  }
  state = State::READY;
  return Status::OK;
}
Status EglContext::Swap(uint64_t) {
  if (!current) return Status::NOT_READY;
  if (currentOwner != std::this_thread::get_id()) return Status::WRONG_THREAD;
  const double t = NowMs();
  const bool ok = eglSwapBuffers(display, surface) == EGL_TRUE;
  swapMs = NowMs() - t;
  return ok ? Status::OK : Failure("swap", Status::SWAP_FAILED);
}
Status EglContext::Unbind() {
  if (!current) return Status::OK;
  if (currentOwner != std::this_thread::get_id()) return Status::WRONG_THREAD;
  const double t = NowMs();
  if (!eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE,
                      EGL_NO_CONTEXT)) {
    error = eglGetError();
    errorStage = "unbind";
    state = State::FAILED;
    unbindMs = NowMs() - t;
    return Status::EGL_INIT_FAILED;
  }
  current = false;
  unbindMs = NowMs() - t;
  return Status::OK;
}
void EglContext::DestroyUnbound(ResetReason) {
  // If unbind failed, keep the owner/current marker; never migrate it.
  if (current && Unbind() != Status::OK) return;
  if (display != EGL_NO_DISPLAY) {
    if (surface != EGL_NO_SURFACE) {
      eglDestroySurface(display, surface);
      surfaceDestroyCount++;
    }
    if (context != EGL_NO_CONTEXT) {
      eglDestroyContext(display, context);
      contextDestroyCount++;
    }
    eglTerminate(display);
  }
  display = EGL_NO_DISPLAY;
  surface = EGL_NO_SURFACE;
  context = EGL_NO_CONTEXT;
  config = nullptr;
  width = height = 0;
}
Status EglContext::SetActive(bool value) {
  active = value;
  if (!value) {
    DestroyUnbound(ResetReason::BACKGROUND);
    state = State::SUSPENDED;
  } else {
    attemptedEpoch = UINT64_MAX;
    recovered = false;
    state = window ? State::WINDOW_AVAILABLE : State::WAIT_WINDOW;
  }
  return current ? Status::EGL_INIT_FAILED : Status::OK;
}
void EglContext::Release() {
  DestroyUnbound(ResetReason::RELEASE);
  state = State::RELEASED;
  active = false;
}
}  // namespace p013
