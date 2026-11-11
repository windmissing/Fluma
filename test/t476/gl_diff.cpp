// T476 GL 侧独立差分：真实 ANGLE ES3.1 上核 compile→PERSISTED→跨进程 BINARY_LOAD compile0、
// 四类失效（代码/layout/link/环境）、驱动拒绝坏 binary、未知 format、
// 坏 hash/截断后一次源码回退、原 GL 错误传播。断言独立从卡面合同推导（不复用 t475 gl_test 断言）。
#include <GLES3/gl31.h>
#include <string>
#include <filesystem>
#include <iostream>
// 生产 ProgramFactory 直接挂真实 GL——不注入查询桩。
#include "../../entry/src/main/cpp/gpu_resident/program_cache/program_factory.h"
#include "../../entry/src/main/cpp/gpu_resident/uniform_locations.h"
#include <cassert>
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
static int checks = 0, passed = 0;
static void check(const char *id, bool ok, const std::string &detail) {
  ++checks;
  if (ok)
    ++passed;
  std::cout << (ok ? "PASS " : "FAIL ") << id << " | " << detail << std::endl;
}
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
  packageSource = std::string("T476_HOST_PACKAGE_") + argv[2]; // 来源独立声明（key 分离验证）
  assert(Store().Configure(argv[1])); // 文件层配置先于任何 Build/Receipt
  const std::string phase = argv[2];
  const auto env = Environment();
  GLint formats = 0;
  glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS, &formats);
  check("G0_REAL_FORMATS", formats > 0, "真实 ANGLE binary formats=" + std::to_string(formats));

  // ---- G1 跨进程 cold→warm：同 key 冷编译 PERSISTED，暖进程 BINARY_LOAD compile0 ----
  const auto key = Key(src, "T476_ABI", env);
  if (phase == "disk-corrupt" || phase == "disk-truncate") {
    const auto file = std::filesystem::path(argv[1]) / "gpu-program-cache/v1" /
                      (key + ".bin");
    std::cout << "DISK_KEY=" << key << std::endl;
    // 破坏由外部 python 在本进程启动前完成（跨进程时序）；本进程只校验拒绝路由。
    assert(std::filesystem::exists(file));
  }
  GLuint program = 0;
  GLenum error = 0;
  assert(Build(0, src, Hash(src).c_str(), "T476_ABI", program, error));
  const auto route = pending.at(program).route;
  const bool diskMode = phase.rfind("disk-", 0) == 0;
  check("G1_COLD_ROUTE", (phase == "cold" || diskMode) ? route == "SOURCE_COMPILE" ||
                                                             route == "REJECT_THEN_SOURCE_ONCE"
                                                       : route == "BINARY_LOAD",
        "phase=" + phase + " route=" + route);
  if (diskMode) {
    const auto &p = pending.at(program);
    check(("G8_" + phase.substr(5)).c_str(),
          p.route == "REJECT_THEN_SOURCE_ONCE" && p.cacheLog.rfind("DISK_", 0) == 0,
          phase + " → DISK_* 拒绝 + 一次源码编译");
    Complete(program);
    glDeleteProgram(program);
    std::cout << "T476 GL differential: " << passed << "/" << checks
              << " PASS (phase=" << phase << ")\n";
    return passed == checks ? 0 : 1;
  }
  if (phase == "warm") {
    const auto &p = pending.at(program);
    check("G2_COMPILE0_ON_WARM", p.compileBegin == 0 && p.compileEnd == 0,
          "暖阶段零源码编译（compile 时间戳未记录）");
    check("G3_ENV_SAME_KEY", p.environment == env && p.key == key,
          "环境指纹一致→同 key 命中");
  }
  Complete(program);
  if (formats) {
    auto a = Store().Get(key);
    check("G4_PERSISTED_ON_DISK",
          a && a->state.load() == PERSISTED,
          "state=" + std::to_string(a ? a->state.load() : -1) + " bytes=" +
              std::to_string(a ? a->bytes.size() : 0));
  }
  glDeleteProgram(program);

  if (phase == "warm" && formats) {
    // ---- G5 四类失效 key 分离 ----
    check("G5_KEY_SEPARATION",
          Key(std::string(src) + "\n#define CHANGED 1", "T476_ABI", env) != key &&
              Key(src, "OTHER_ABI", env) != key &&
              Key(src, "T476_ABI", env + "x") != key &&
              Key(src, "T476_ABI", env, "different-link") != key,
          "代码/layout/环境/link 任一变更 → 不同 key");

    // ---- G6 驱动拒绝坏 binary → 一次源码回退 ----
    Store().Reject(key);
    Store().Drain();
    {
      GLint realFormat = 0;
      glGetIntegerv(GL_PROGRAM_BINARY_FORMATS, &realFormat);
      auto bad = Store().Put(key, uint32_t(realFormat), {1, 2, 3});
      assert(bad);
      Store().Drain();
    }
    program = 0;
    error = 0;
    assert(Build(0, src, Hash(src).c_str(), "T476_ABI", program, error));
    {
      const auto &p = pending.at(program);
      check("G6_DRIVER_REJECT_FALLBACK", p.route == "REJECT_THEN_SOURCE_ONCE" &&
                                            p.compileBegin > 0,
            "坏 binary 驱动拒绝 → REJECT_THEN_SOURCE_ONCE + 真实源码编译一次");
    }
    Complete(program);
    glDeleteProgram(program);

    // ---- G7 未知 format 拒绝 ----
    Store().Reject(key);
    Store().Drain();
    {
      auto unknown = Store().Put(key, 0xdeadbeefu, {1, 2, 3});
      assert(unknown);
      Store().Drain();
    }
    program = 0;
    error = 0;
    assert(Build(0, src, Hash(src).c_str(), "T476_ABI", program, error));
    check("G7_UNKNOWN_FORMAT_REJECTED",
          pending.at(program).route == "REJECT_THEN_SOURCE_ONCE",
          "未知 format 0xdeadbeef → 拒绝 + 源码回退");
    Complete(program);
    glDeleteProgram(program);

    // ---- G9 无效 shader：诊断收据，无可执行产物 ----
    const char *invalid = "#version 310 es\nthis is invalid shader code";
    program = 0;
    error = 0;
    assert(!Build(0, invalid, Hash(invalid).c_str(), "INVALID", program, error));
    Store().Drain();
    check("G9_INVALID_SHADER_NO_ARTIFACT",
          !Store().Get(Key(invalid, "INVALID", env)),
          "编译失败不产生缓存产物");
    if (program)
      glDeleteProgram(program);

    // ---- G10 原产品 GL 错误传播 ----
    glBindBuffer(0xdeadbeef, 0);
    program = 0;
    error = 0;
    assert(!Build(0, src, Hash(src).c_str(), "PRIOR_ERROR", program, error));
    check("G10_PRIOR_GL_ERROR_PROPAGATED", error == GL_INVALID_ENUM,
          "既有 GL 错误以原码失败（不当 cache miss 吞掉）");
    glGetError(); // 清错
    Store().Drain();

    // ---- G11 跨存活 context 拒绝导出 ----
    program = 0;
    error = 0;
    assert(Build(0, src, Hash(src).c_str(), "CTX_PIN", program, error));
    auto second = eglCreateContext(display, config, EGL_NO_CONTEXT, ca);
    assert(second != ctx && second != EGL_NO_CONTEXT);
    assert(eglMakeCurrent(display, surface, surface, second));
    assert(Prepared(program) == GL_NO_ERROR);
    Store().Drain();
    check("G11_CROSS_CONTEXT_REFUSED",
          !Store().Get(Key(src, "CTX_PIN", env)),
          "原 context 存活时跨 context 导出拒绝");
    assert(eglMakeCurrent(display, surface, surface, ctx));
    glDeleteProgram(program);
    eglDestroyContext(display, second);
  }

  std::cout << "T476 GL differential: " << passed << "/" << checks
            << " PASS (phase=" << phase << ")\n";
  return passed == checks ? 0 : 1;
}
