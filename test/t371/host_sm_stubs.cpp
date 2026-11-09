// T371 host_sm link stubs — self-contained in test/t371/, touches no T364/product assets.
// Rationale: shared .codex/local-test/t364-host/gl.cpp only stubs the 58 GL entry points T364
// uses; host_sm.cpp additionally needs glClear/glClearColor/glTexImage2D. EglContext::Swap is
// normally provided by T364 host.cpp (not linked here for independence), so mirror its stub.
#include "single_sand_pass.h"
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <cassert>
using namespace p013;

// Same semantics as T364 host.cpp EglContext::Swap stub: test path never fails swap.
Status EglContext::Swap(uint64_t) { return Status::OK; }

namespace {
template <typename FnT>
inline FnT load(const char* name) {
  auto fn = reinterpret_cast<FnT>(eglGetProcAddress(name));
  assert(fn);
  return fn;
}
} // namespace

extern "C" void GL_APIENTRY glClear(GLenum mask) {
  static auto fn = load<PFNGLCLEARPROC>("glClear");
  fn(mask);
}
extern "C" void GL_APIENTRY glClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
  static auto fn = load<PFNGLCLEARCOLORPROC>("glClearColor");
  fn(red, green, blue, alpha);
}
extern "C" void GL_APIENTRY glTexImage2D(GLenum target, GLint level, GLint internalformat,
                                         GLsizei width, GLsizei height, GLint border,
                                         GLenum format, GLenum type, const void* pixels) {
  static auto fn = load<PFNGLTEXIMAGE2DPROC>("glTexImage2D");
  fn(target, level, internalformat, width, height, border, format, type, pixels);
}
