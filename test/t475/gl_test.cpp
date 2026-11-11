#include <GLES3/gl31.h>
#include <string>
static std::string fault;
static void TestInteger(GLenum token, GLint *value) {
  if (fault == "no_formats" && token == GL_NUM_PROGRAM_BINARY_FORMATS) {
    *value = 0;
    return;
  }
  glGetIntegerv(token, value);
}
static void TestProgramiv(GLuint object, GLenum token, GLint *value) {
  if (token == GL_PROGRAM_BINARY_LENGTH && fault == "empty_export") {
    *value = 0;
    return;
  }
  if (token == GL_PROGRAM_BINARY_LENGTH && fault == "oversize_export") {
    *value = 32 * 1024 * 1024 + 1;
    return;
  }
  glGetProgramiv(object, token, value);
}
#define glGetIntegerv TestInteger
#define glGetProgramiv TestProgramiv
#include "program_cache/program_factory.h"
#undef glGetIntegerv
#undef glGetProgramiv
#include "uniform_locations.h"
#include <cassert>
#include <iostream>
#define PROC(type, name)                                                       \
  auto f = reinterpret_cast<type>(eglGetProcAddress(#name));                   \
  assert(f)
extern "C" void GL_APIENTRY glProgramParameteri(GLuint p, GLenum k, GLint v) {
  PROC(PFNGLPROGRAMPARAMETERIPROC, glProgramParameteri);
  f(p, k, v);
}
extern "C" void GL_APIENTRY glProgramBinary(GLuint p, GLenum format,
                                            const void *bytes, GLsizei n) {
  PROC(PFNGLPROGRAMBINARYPROC, glProgramBinary);
  f(p, format, bytes, n);
}
extern "C" void GL_APIENTRY glGetProgramBinary(GLuint p, GLsizei n,
                                               GLsizei *len, GLenum *format,
                                               void *bytes) {
  PROC(PFNGLGETPROGRAMBINARYPROC, glGetProgramBinary);
  f(p, n, len, format, bytes);
}
extern "C" const GLubyte *GL_APIENTRY glGetStringi(GLenum p, GLuint i) {
  PROC(PFNGLGETSTRINGIPROC, glGetStringi);
  return f(p, i);
}
extern "C" void GL_APIENTRY glGetShaderInfoLog(GLuint p, GLsizei n,
                                               GLsizei *len, GLchar *bytes) {
  PROC(PFNGLGETSHADERINFOLOGPROC, glGetShaderInfoLog);
  f(p, n, len, bytes);
}
extern "C" void GL_APIENTRY glGetProgramInfoLog(GLuint p, GLsizei n,
                                                GLsizei *len, GLchar *bytes) {
  PROC(PFNGLGETPROGRAMINFOLOGPROC, glGetProgramInfoLog);
  f(p, n, len, bytes);
}
extern "C" EGLBoolean EGLAPIENTRY eglQueryContext(EGLDisplay d, EGLContext c,
                                                  EGLint k, EGLint *v) {
  using F = EGLBoolean(EGLAPIENTRYP)(EGLDisplay, EGLContext, EGLint, EGLint *);
  PROC(F, eglQueryContext);
  return f(d, c, k, v);
}
using namespace grs::programcache;
static void Complete(GLuint program) {
  grs::UniformLocations uniforms;
  assert(uniforms.Prepare(program) && uniforms.Find("value") >= 0);
  assert(glGetError() == GL_NO_ERROR);
  assert(Prepared(program) == GL_NO_ERROR);
  Store().Drain();
}
int main(int argc, char **argv) {
  assert(argc == 3);

  using F = EGLDisplay(EGLAPIENTRYP)(EGLenum, void *, const EGLint *);
  PROC(F, eglGetPlatformDisplayEXT);
  EGLint displayAttrs[] = {0x3203, 0x3450, 0x3209, 0x3487, EGL_NONE};
  auto display = f(0x3202, nullptr, displayAttrs);
  assert(eglInitialize(display, nullptr, nullptr));
  EGLint attrs[] = {EGL_SURFACE_TYPE, EGL_PBUFFER_BIT, EGL_RENDERABLE_TYPE,
                    EGL_OPENGL_ES3_BIT, EGL_NONE};
  EGLConfig config;
  EGLint count = 0;
  assert(eglChooseConfig(display, attrs, &config, 1, &count) && count);
  EGLint ca[] = {EGL_CONTEXT_CLIENT_VERSION, 3, 0x30FB, 1, EGL_NONE},
         sa[] = {EGL_WIDTH, 8, EGL_HEIGHT, 8, EGL_NONE};
  auto ctx = eglCreateContext(display, config, EGL_NO_CONTEXT, ca);
  auto surface = eglCreatePbufferSurface(display, config, sa);
  assert(eglMakeCurrent(display, surface, surface, ctx));
  const char *src = "#version 310 es\nlayout(local_size_x=1) "
                    "in;layout(std430,binding=0)buffer B{uint n;};uniform uint "
                    "value;void main(){n=value;}";
  packageSource = std::string("HOST_PACKAGE_") + argv[2];
  auto key = Key(src, "TEST_ABI", Environment());
  assert(Key(std::string(src) + "\n#define CODE_CHANGE 1", "TEST_ABI",
             Environment()) != key);
  assert(Key(src, "OTHER_ABI", Environment()) != key);
  assert(Key(src, "TEST_ABI", "different") != key);
  assert(Key(src, "TEST_ABI", Environment(), "different") != key);
  const std::string phase = argv[2];
  if (phase == "corrupt_hash" || phase == "truncate") {
    const auto file = std::filesystem::path(argv[1]) / "gpu-program-cache/v1" /
                      (key + ".bin");
    assert(std::filesystem::exists(file));
    if (phase == "truncate")
      std::filesystem::resize_file(file, 50);
    else {
      std::fstream bytes(file, std::ios::binary | std::ios::in | std::ios::out);
      bytes.seekp(144);
      bytes.put('x');
    }
  }
  assert(Store().Configure(argv[1]));
  GLuint program = 0;
  GLenum error = 0;
  assert(Build(0, src, Hash(src).c_str(), "TEST_ABI", program, error));
  const auto route = pending.at(program).route;
  if (phase == "corrupt_hash" || phase == "truncate") {
    assert(route == "REJECT_THEN_SOURCE_ONCE");
    assert(pending.at(program).cacheLog.find("DISK_") == 0);
  }
  Complete(program);
  GLint formats = 0;
  glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS, &formats);
  if (formats) {
    assert(Store().Get(key) && Store().Get(key)->state == 2);
    assert((std::string(argv[2]) == "warm") == (route == "BINARY_LOAD"));
  }
  std::cout << "real ANGLE ES31 formats=" << formats << " route=" << route
            << " key=" << key << " reflection=PASS\n";
  glDeleteProgram(program);
  if (std::string(argv[2]) == "warm" && formats) {
    // Real driver rejects corrupt executable bytes. Exactly one source
    // compilation follows.
    for (bool unknown : {true, false}) {
      Store().Reject(key);
      Store().Drain();
      auto a = Store().Put(
          key,
          unknown ? 0xdeadbeefu
                  : GLenum(Store().Get(key) ? Store().Get(key)->format : 0),
          {1, 2, 3});
      // The real format is queried explicitly, independently of corrupted
      // storage.
      if (!unknown) {
        a.reset();
        Store().Reject(key);
        Store().Drain();
        GLint format = 0;
        glGetIntegerv(GL_PROGRAM_BINARY_FORMATS, &format);
        a = Store().Put(key, uint32_t(format), {1, 2, 3});
      }
      Store().Drain();
      a.reset();
      program = 0;
      error = 0;
      assert(Build(0, src, Hash(src).c_str(), "TEST_ABI", program, error));
      assert(pending.at(program).route == "REJECT_THEN_SOURCE_ONCE");
      assert(pending.at(program).compileBegin &&
             pending.at(program).compileEnd);
      Complete(program);
      glDeleteProgram(program);
    }
    for (const char *mode : {"no_formats", "empty_export", "oversize_export"}) {
      fault = mode;
      auto layout = std::string("FAULT_") + mode;
      auto faultKey = Key(src, layout, Environment());
      program = 0;
      error = 0;
      assert(Build(0, src, Hash(src).c_str(), layout.c_str(), program, error));
      Complete(program);
      assert(!Store().Get(faultKey));
      glDeleteProgram(program);
      std::cout << "injected GL query " << mode
                << " source program retained PASS\n";
    }
    fault.clear();
    // Actual compile failure creates a diagnostic receipt and no executable
    // artifact.
    const char *invalid = "#version 310 es\nthis is invalid shader code";
    program = 0;
    error = 0;
    assert(
        !Build(0, invalid, Hash(invalid).c_str(), "INVALID", program, error));
    Store().Drain();
    assert(!Store().Get(Key(invalid, "INVALID", Environment())));
    if (program)
      glDeleteProgram(program);
    // A preexisting product GL error must fail with the original code.
    glBindBuffer(0xdeadbeef, 0);
    program = 0;
    error = 0;
    assert(!Build(0, src, Hash(src).c_str(), "PRIOR_ERROR", program, error));
    assert(error == GL_INVALID_ENUM);
    Store().Drain();
    // The old context remains alive so this is a genuinely distinct context
    // handle.
    program = 0;
    error = 0;
    assert(Build(0, src, Hash(src).c_str(), "CONTEXT_PIN", program, error));
    auto second = eglCreateContext(display, config, EGL_NO_CONTEXT, ca);
    assert(second != ctx && second != EGL_NO_CONTEXT);
    assert(eglMakeCurrent(display, surface, surface, second));
    assert(Prepared(program) == GL_NO_ERROR);
    assert(!pending.count(program));
    Store().Drain();
    assert(!Store().Get(Key(src, "CONTEXT_PIN", Environment())));
    assert(eglMakeCurrent(display, surface, surface, ctx));
    glDeleteProgram(program);
    eglDestroyContext(display, second);
    std::cout
        << "real driver corrupt binary fallback, unknown format, failed "
           "compile, original GL error, cross-context export refusal PASS\n";
  }
  eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
  eglDestroyContext(display, ctx);
  eglDestroySurface(display, surface);
  eglTerminate(display);
}
