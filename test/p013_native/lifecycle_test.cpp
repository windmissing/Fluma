// P-013 developer selfcheck: real lifecycle class, deterministic platform
// faults.
#include <cstdio>
#include <cstdlib>
#include <limits>

#include "egl_context.h"
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
using namespace p013;
static int fail = 0, contexts = 0, surfaces = 0, terminated = 0;
static bool colorSupported = true, colorWriteEnabled = true, msaaConfig = false;
static EGLint colorAlpha = 8, surfaceColor = EGL_GL_COLORSPACE_SRGB_KHR;
static GLint attachmentColor = GL_SRGB;
static bool requestedSrgb = false;
static EGLint injectedError = EGL_BAD_ALLOC;
static bool hit(int n) { return fail == n; }
#define CHECK(x)                                       \
  do {                                                 \
    if (!(x)) {                                        \
      std::printf("FAIL line %d: %s\n", __LINE__, #x); \
      return 1;                                        \
    }                                                  \
  } while (0)
extern "C" {
int32_t OH_NativeXComponent_GetXComponentSize(OH_NativeXComponent*, const void*,
                                              uint64_t* w, uint64_t* h) {
  *w = 999;
  *h = 999;
  return 0;
}
EGLDisplay eglGetDisplay(EGLNativeDisplayType) {
  return hit(1) ? EGL_NO_DISPLAY : reinterpret_cast<EGLDisplay>(1);
}
EGLBoolean eglInitialize(EGLDisplay, EGLint*, EGLint*) { return !hit(2); }
const char* eglQueryString(EGLDisplay, EGLint name) {
  return name == EGL_VERSION ? "1.4" : colorSupported ? "EGL_KHR_gl_colorspace" : "";
}
EGLBoolean eglGetConfigAttrib(EGLDisplay, EGLConfig, EGLint name, EGLint* value) {
  *value = name == EGL_SAMPLE_BUFFERS || name == EGL_SAMPLES ? (msaaConfig ? 4 : 0) : name == EGL_ALPHA_SIZE ? colorAlpha : 8; return EGL_TRUE;
}
void glBindFramebuffer(GLenum, GLuint) {}
void glGetIntegerv(GLenum, GLint* value) { *value = 0; }
void glGetFramebufferAttachmentParameteriv(GLenum, GLenum, GLenum, GLint* value) { *value = attachmentColor; }
const GLubyte* glGetString(GLenum) { return reinterpret_cast<const GLubyte*>("GL_EXT_sRGB_write_control"); }
void glEnable(GLenum) {}
GLboolean glIsEnabled(GLenum) { return colorWriteEnabled ? GL_TRUE : GL_FALSE; }
GLenum glGetError() { return GL_NO_ERROR; }
EGLBoolean eglChooseConfig(EGLDisplay, const EGLint*, EGLConfig* c, EGLint,
                           EGLint* n) {
  *c = reinterpret_cast<EGLConfig>(1);
  *n = 1;
  return !hit(3);
}
EGLBoolean eglBindAPI(EGLenum) { return !hit(4); }
EGLContext eglCreateContext(EGLDisplay, EGLConfig, EGLContext, const EGLint*) {
  if (hit(5)) return EGL_NO_CONTEXT;
  contexts++;
  return reinterpret_cast<EGLContext>(1);
}
EGLSurface eglCreateWindowSurface(EGLDisplay, EGLConfig, EGLNativeWindowType,
                                  const EGLint* attrs) {
  requestedSrgb = attrs && attrs[0] == EGL_GL_COLORSPACE_KHR && attrs[1] == EGL_GL_COLORSPACE_SRGB_KHR;
  if (hit(6)) return EGL_NO_SURFACE;
  surfaces++;
  return reinterpret_cast<EGLSurface>(1);
}
EGLBoolean eglMakeCurrent(EGLDisplay, EGLSurface d, EGLSurface, EGLContext) {
  return !(hit(7) || (hit(10) && d == EGL_NO_SURFACE));
}
EGLBoolean eglQuerySurface(EGLDisplay, EGLSurface, EGLint attribute, EGLint* v) {
  *v = attribute == EGL_GL_COLORSPACE_KHR ? surfaceColor : 64;
  return !hit(8);
}
EGLBoolean eglSwapBuffers(EGLDisplay, EGLSurface) { return !hit(9); }
EGLBoolean eglDestroySurface(EGLDisplay, EGLSurface) {
  surfaces--;
  return EGL_TRUE;
}
EGLBoolean eglDestroyContext(EGLDisplay, EGLContext) {
  contexts--;
  return EGL_TRUE;
}
EGLBoolean eglTerminate(EGLDisplay) {
  terminated++;
  return EGL_TRUE;
}
EGLint eglGetError() { return injectedError; }
}
int main() {
  auto* component = reinterpret_cast<OH_NativeXComponent*>(1);
  void* oldWindow = reinterpret_cast<void*>(1);
  void* newWindow = reinterpret_cast<void*>(2);
  // Explicit profile controls, unsupported paths, and original profile isolation.
  for (int fault = 0; fault < 7; fault++) {
    EglContext profile; SurfaceInfo pi{}; profile.OnWindowCreated(component,oldWindow);
    const auto oldEpoch=profile.surfaceEpoch;
    CHECK(profile.ConfigureComposition(true)==Status::OK && profile.surfaceEpoch>oldEpoch);
    msaaConfig=fault==6;
    colorWriteEnabled=fault!=5;
    colorSupported=fault!=1; colorAlpha=fault==2?0:8;
    surfaceColor=fault==3?EGL_GL_COLORSPACE_LINEAR_KHR:EGL_GL_COLORSPACE_SRGB_KHR;
    attachmentColor=fault==4?GL_LINEAR:GL_SRGB;
    auto code=profile.EnsureCurrent({profile.surfaceEpoch,64,64},pi);
    if(fault==0){CHECK(code==Status::OK && requestedSrgb && profile.alphaSize==8);profile.Unbind();}
    else CHECK(code==Status::COLORSPACE_UNSUPPORTED);
    profile.Release();CHECK(contexts==0 && surfaces==0);
  }
  msaaConfig=false;colorWriteEnabled=true;colorSupported=true;colorAlpha=8;surfaceColor=EGL_GL_COLORSPACE_SRGB_KHR;attachmentColor=GL_SRGB;
  for (int stage = 1; stage <= 8; stage++) {
    EglContext e;
    e.OnWindowCreated(component, oldWindow);
    SurfaceInfo info{};
    fail = stage;
    CHECK(e.EnsureCurrent({e.surfaceEpoch, 64, 64}, info) != Status::OK);
    fail = 0;
    e.Release();
    e.Release();
    CHECK(contexts == 0 && surfaces == 0 && !e.current);
  }
  EglContext e;
  SurfaceInfo info{};
  e.OnWindowCreated(component, oldWindow);
  auto epoch = e.surfaceEpoch;
  e.OnWindowCreated(component, oldWindow);
  CHECK(e.surfaceEpoch == epoch);
  CHECK(e.EnsureCurrent({epoch, 999, 999}, info) == Status::STALE_SURFACE);
  CHECK(e.width == 64 && e.height == 64 && !e.current);
  CHECK(e.EnsureCurrent({epoch, 64, 64}, info) == Status::OK);
  CHECK(e.Swap(1) == Status::OK);
  CHECK(e.Unbind() == Status::OK);
  CHECK(e.SetActive(false) == Status::OK && contexts == 0 && surfaces == 0);
  CHECK(e.EnsureCurrent({epoch, 64, 64}, info) == Status::INACTIVE);
  e.SetActive(true);
  CHECK(e.EnsureCurrent({epoch, 64, 64}, info) == Status::OK);
  e.Unbind();
  e.OnWindowCreated(component, newWindow);
  auto newEpoch = e.surfaceEpoch;
  e.OnWindowChanged(component, oldWindow);
  e.OnWindowDestroyed(component, oldWindow);
  CHECK(e.surfaceEpoch == newEpoch && e.staleLifecycleCount == 2);
  CHECK(e.EnsureCurrent({epoch, 64, 64}, info) == Status::STALE_SURFACE);
  CHECK(e.EnsureCurrent({newEpoch, 64, 64}, info) == Status::OK);
  e.OnWindowDestroyed(component, oldWindow);
  CHECK(e.current && e.state == State::READY && e.staleLifecycleCount == 3);
  fail = 9;
  injectedError = EGL_CONTEXT_LOST;
  CHECK(e.Swap(2) == Status::CONTEXT_LOST);
  fail = 0;
  CHECK(contexts == 0 && surfaces == 0 && !e.current);
  CHECK(e.EnsureCurrent({newEpoch, 64, 64}, info) == Status::OK);
  fail = 9;
  CHECK(e.Swap(3) == Status::CONTEXT_LOST);
  fail = 0;
  CHECK(e.EnsureCurrent({newEpoch, 64, 64}, info) == Status::EGL_INIT_FAILED);
  e.OnWindowChanged(component, newWindow);
  CHECK(e.EnsureCurrent({e.surfaceEpoch, 64, 64}, info) == Status::OK);
  fail = 9;
  injectedError = EGL_BAD_SURFACE;
  CHECK(e.Swap(4) == Status::BAD_SURFACE);
  fail = 0;
  CHECK(e.EnsureCurrent({e.surfaceEpoch, 64, 64}, info) == Status::NOT_READY);
  e.Release();
  CHECK(contexts == 0 && surfaces == 0);
  EglContext broken;
  broken.OnWindowCreated(component, oldWindow);
  CHECK(broken.EnsureCurrent({broken.surfaceEpoch, 64, 64}, info) ==
        Status::OK);
  fail = 10;
  CHECK(broken.Unbind() == Status::EGL_INIT_FAILED);
  CHECK(broken.current);
  CHECK(broken.EnsureCurrent({broken.surfaceEpoch, 64, 64}, info) ==
        Status::EGL_INIT_FAILED);
  bool otherThreadRefused = false;
  std::thread other(
      [&] { otherThreadRefused = broken.Unbind() == Status::WRONG_THREAD; });
  other.join();
  CHECK(otherThreadRefused);
  broken.OnWindowDestroyed(component, oldWindow);
  fail = 0;
  CHECK(broken.Unbind() == Status::OK);
  broken.SetActive(true);
  CHECK(broken.EnsureCurrent({broken.surfaceEpoch, 64, 64}, info) ==
        Status::NOT_READY);
  broken.Release();
  CHECK(contexts == 0 && surfaces == 0);
  // T62 real EGL preparation: no caller dimensions required, no swap.
  EglContext prepared;
  prepared.OnWindowCreated(component, oldWindow);
  const auto pe = prepared.surfaceEpoch;
  CHECK(prepared.width == 0 && prepared.height == 0);
  CHECK(prepared.PrepareCurrent(pe, info) == Status::OK);
  CHECK(info.width == 64 && info.height == 64 && prepared.current);
  CHECK(prepared.Unbind() == Status::OK);
  CHECK(prepared.EnsureCurrent({pe,63,64},info) == Status::STALE_SURFACE);
  CHECK(!prepared.current);
  CHECK(prepared.PrepareCurrent(pe+1,info) == Status::STALE_SURFACE);
  prepared.SetActive(false);
  CHECK(prepared.PrepareCurrent(pe,info) == Status::INACTIVE);
  prepared.SetActive(true);
  CHECK(prepared.PrepareCurrent(pe,info) == Status::OK);
  CHECK(prepared.Unbind() == Status::OK);
  prepared.Release();
  FrameInput f{};
  f.frameId = 1;
  f.surfaceWidthPx = 64;
  f.surfaceHeightPx = 64;
  f.boxWidthPx = 64;
  f.boxHeightPx = 64;
  f.capacity = 192;
  f.T_dataLayoutVersion = 1;
  f.zOriginVersion = 1;
  LightParams l{};
  l.la = .85;
  l.ld = .15;
  l.lmin = .8;
  l.lmax = 1.12;
  l.lref = 1;
  l.ellLightMaterial = 24;
  l.lightZ = 1;
  std::array<float, kFloats> data{};
  for (size_t i = 0; i < 128; i++) {
    data[4 * i] = 80;
    data[4 * i + 1] = 160;
    data[4 * i + 2] = 20;
    data[4 * i + 3] = 40;
  }
  CHECK(Validate(f, l, data) == Status::OK);
  data[100] = std::numeric_limits<float>::quiet_NaN();
  CHECK(Validate(f, l, data) == Status::INVALID_ARGUMENT);
  data[100] = 80;
  f.T_dataLayoutVersion = 2;
  CHECK(Validate(f, l, data) == Status::VERSION_MISMATCH);
  f.T_dataLayoutVersion = 1;
  f.frameId = .5;
  CHECK(Validate(f, l, data) == Status::INVALID_ARGUMENT);
  f.frameId = 1;
  data[102] = 90;
  CHECK(Validate(f, l, data) == Status::INVALID_ARGUMENT);
  std::puts(
      "PASS P013 lifecycle: eight partial-init faults, authoritative "
      "dimensions, background, stale callbacks, swap/context loss, bounded "
      "recovery, repeated release, frame contract");
  return 0;
}
