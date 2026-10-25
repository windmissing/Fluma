#include "egl_context.h"
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <cstring>
#include <cstdlib>
namespace p013 {
namespace {
bool HasExtension(const char* list, const char* name) {
 if(!list)return false;const size_t length=std::strlen(name);
 for(const char* p=list;(p=std::strstr(p,name));p+=length)
  if((p==list || p[-1]==' ') && (p[length]==' ' || p[length]=='\0'))return true;
 return false;
}
}

Status EglContext::ConfigureComposition(bool enabled) {
  if (current) return Status::SESSION_BUSY;
  if (composition == enabled) return Status::OK;
  DestroyUnbound(ResetReason::RELEASE);
  composition = enabled; alphaSize = -1; actualColorSpace = attachmentEncoding = 0;
  writeControlSupported = srgbWriteEnabled = false;
  surfaceEpoch++; attemptedEpoch = UINT64_MAX; badWindow = recovered = false;
  state = window ? State::WINDOW_AVAILABLE : State::WAIT_WINDOW;
  return Status::OK;
}
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
    if (composition) {
      const char* extensions = eglQueryString(display, EGL_EXTENSIONS);
      const char* version = eglQueryString(display, EGL_VERSION);
      eglVersion = version ? version : "UNPROVEN";
      eglExtensions = extensions ? extensions : "UNPROVEN";
      if ((!HasExtension(extensions, "EGL_KHR_gl_colorspace")) &&
          (!version || std::strtod(version, nullptr) < 1.5))
        return Failure("COLORSPACE_EXTENSION", Status::COLORSPACE_UNSUPPORTED);
    }
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
    const EGLint textureAttrs[] = {EGL_SURFACE_TYPE,EGL_WINDOW_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,
      EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,
      EGL_SAMPLE_BUFFERS,0,EGL_SAMPLES,0,EGL_NONE};
    if (!eglChooseConfig(display, composition ? textureAttrs : attrs, &config, 1, &count) || count < 1)
      return Failure("chooseConfig", Status::EGL_INIT_FAILED);
    if (!eglBindAPI(EGL_OPENGL_ES_API))
      return Failure("bindAPI", Status::EGL_INIT_FAILED);
    const EGLint ca[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    context = eglCreateContext(display, config, EGL_NO_CONTEXT, ca);
    if (context == EGL_NO_CONTEXT)
      return Failure("createContext", Status::EGL_INIT_FAILED);
    contextCreateCount++;
    if (composition) {
      EGLint bits = 0;
      for (EGLint channel : {EGL_RED_SIZE, EGL_GREEN_SIZE, EGL_BLUE_SIZE, EGL_ALPHA_SIZE}) {
        if (!eglGetConfigAttrib(display, config, channel, &bits) || bits < 8)
          return Failure("COLORSPACE_RGBA8", Status::COLORSPACE_UNSUPPORTED);
        if (channel == EGL_ALPHA_SIZE) alphaSize = bits;
      }
    }
    if (composition) {
      EGLint buffers = -1, samples = -1;
      if (!eglGetConfigAttrib(display, config, EGL_SAMPLE_BUFFERS, &buffers) ||
          !eglGetConfigAttrib(display, config, EGL_SAMPLES, &samples) || buffers != 0 || samples != 0)
        return Failure("COLORSPACE_MSAA", Status::COLORSPACE_UNSUPPORTED);
    }
    const EGLint colorAttrs[] = {EGL_GL_COLORSPACE_KHR, EGL_GL_COLORSPACE_SRGB_KHR, EGL_NONE};
    surface = eglCreateWindowSurface(
        display, config, reinterpret_cast<EGLNativeWindowType>(window),
        composition ? colorAttrs : nullptr);
    if (surface == EGL_NO_SURFACE)
      return Failure("createSurface", Status::EGL_INIT_FAILED);
    surfaceCreateCount++;
    if (composition) {
      EGLint space = 0;
      if (!eglQuerySurface(display, surface, EGL_GL_COLORSPACE_KHR, &space) || space != EGL_GL_COLORSPACE_SRGB_KHR)
        return Failure("COLORSPACE_SURFACE", Status::COLORSPACE_UNSUPPORTED);
      actualColorSpace = space;
    }
    contextEpoch++;
    initMs = NowMs() - t;
  }
  const double mt = NowMs();
  if (!eglMakeCurrent(display, surface, surface, context))
    return Failure("makeCurrent", Status::EGL_INIT_FAILED);
  current = true;
  currentOwner = std::this_thread::get_id();
  currentMs = NowMs() - mt;
  if (composition) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    GLint draw = -1, read = -1, encoding = 0;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &draw);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &read);
    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_BACK, GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING, &encoding);
    attachmentEncoding = encoding; drawFramebuffer = draw; readFramebuffer = read;
    if (draw != 0 || read != 0 || encoding != GL_SRGB || glGetError() != GL_NO_ERROR) {
      Unbind(); return Failure("COLORSPACE_ATTACHMENT", Status::COLORSPACE_UNSUPPORTED);
    }
    const char* extensions = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
    const char* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    glVersion = version ? version : "UNPROVEN";
    glExtensions = extensions ? extensions : "UNPROVEN";
    writeControlSupported = HasExtension(extensions, "GL_EXT_sRGB_write_control");
    if (writeControlSupported) glEnable(0x8DB9); // GL_FRAMEBUFFER_SRGB_EXT, only with extension.
    srgbWriteEnabled = !writeControlSupported || glIsEnabled(0x8DB9) == GL_TRUE;
    if (!srgbWriteEnabled || glGetError() != GL_NO_ERROR) {
      Unbind(); return Failure("COLORSPACE_WRITE_CONTROL", Status::COLORSPACE_UNSUPPORTED);
    }
  }
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
  alphaSize = -1; actualColorSpace = attachmentEncoding = 0;
  drawFramebuffer = readFramebuffer = -1;
  writeControlSupported = srgbWriteEnabled = false;
  eglVersion.clear(); eglExtensions.clear(); glVersion.clear(); glExtensions.clear();
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
  composition = false;
  active = false;
}
}  // namespace p013
