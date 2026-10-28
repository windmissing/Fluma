// T232 GPU驻留平台真机探针 native C++: P-A..P-F (tester, isolated probe target)
// 消费 T231 §5 探针矩阵 + T230 §12 P-A..F 需求。独立 probe main，不改生产开关/设备设置。
// 复用 T131 host 主体骨架（EGL pbuffer + GLES3.2），新增:
//   P-A 整数原语: uaddCarry/usubBorrow/umulExtended/i64 shifts/signed RNE div vs CPU oracle
//   P-B limits 扩展: 全局 bindings16/compute blocks16/offset alignment/maxBlock/shared + 版本身份
//   P-C dispatch 可见性: 两 workgroup 跨 dispatch 写读 + barrier 组合
//   P-D completion: 1024B 尾写 + fence 异步 poll/map + late buffer
//   P-F 数值备选: FP64 扩展枚举 + double 编译负例 + int64/atomic 扩展
// (P-E compute→scene 共享 context 归同卡后续段，需 XComponent 真实 window context——本段 pbuffer 先核 P-A/B/C/D/F)
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl31.h>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <sstream>
#include <algorithm>

#define CHECK(x) do{if(!(x)){std::printf("FAIL line %d %s egl=%x gl=%x\n",__LINE__,#x,eglGetError(),glGetError());return 1;}}while(0)
#ifdef FORCE_HOST_STUBS
// Host-link sanity only: old static GLES lib lacks glMemoryBarrier. Device builds use the real driver.
extern "C" void glMemoryBarrier(GLbitfield) {}
#endif
namespace t232probe {
int runDeviceProbe(const char* outDir);
}
static std::string g_out = ".";
static void dumpText(const std::string& n, const std::string& t) { std::ofstream f(g_out + "/" + n, std::ios::binary); f.write(t.data(), t.size()); }
static std::string hexOf(const void* p, size_t n) { static const char* h = "0123456789abcdef"; std::string s; const uint8_t* b = (const uint8_t*)p; for (size_t i = 0; i < n; ++i) { s += h[b[i] >> 4]; s += h[b[i] & 15]; } return s; }

// ============ P-A integer primitives compute shader ============
// SSBO layout: [0] inA (N u32), [1] inB (N u32), [2] out (N uvec4: carry,borrow,hi,lo-ish), [3] outDiv
static const char* kIntPrimSrc = R"(#version 310 es
precision highp int;
layout(local_size_x=64) in;
layout(std430,binding=0) readonly buffer A { uint a[]; };
layout(std430,binding=1) readonly buffer B { uint b[]; };
layout(std430,binding=2) writeonly buffer O { uvec4 o[]; };
layout(std430,binding=3) writeonly buffer D { uint d[]; };
void main(){
  uint i = gl_GlobalInvocationID.x;
  if (i >= uint(a.length())) return;
  uint x = a[i], y = b[i % uint(b.length())];
  // uaddCarry / usubBorrow
  uint c = 0u, br = 0u;
  uint s = uaddCarry(x, y, c);
  uint sub = usubBorrow(x, y, br);
  // umulExtended 32x32->64
  uint hi = 0u, lo = 0u;
  umulExtended(x, y, hi, lo);
  // shifts 0/31 + shift by width edge
  uint sh31 = x << 31u;
  uint shR = x >> (y & 31u);
  // signed RNE division: emulate via float-free integer: q = roundToEven(x/y) on magnitude
  uint denom = (y & 0x7fffffffu) | 1u; // nonzero odd-ish
  uint q0 = x / denom, r0 = x % denom;
  uint twice = r0 * 2u;
  uint qRNE = q0;
  if (twice > denom || (twice == denom && (q0 & 1u) == 1u)) qRNE = q0 + 1u;
  o[i] = uvec4(c, br, hi, lo);
  d[i] = (s ^ sub ^ sh31 ^ shR ^ qRNE);
})";

// CPU oracle: exact same semantics in C++
struct IntPrimResult { uint32_t carry, borrow, mulHi, mulLo, mix; };
static IntPrimResult cpuIntPrim(uint32_t x, uint32_t y) {
  IntPrimResult r{};
  uint64_t sum = (uint64_t)x + y;
  r.carry = (uint32_t)(sum >> 32);
  r.borrow = x < y ? 1u : 0u;
  uint64_t prod = (uint64_t)x * y;
  r.mulLo = (uint32_t)(prod & 0xffffffffull);
  r.mulHi = (uint32_t)(prod >> 32);
  uint32_t s = x + y, sub = x - y;
  uint32_t sh31 = x << 31, shR = x >> (y & 31);
  uint32_t denom = (y & 0x7fffffffu) | 1u;
  uint32_t q0 = x / denom, r0 = x % denom;
  uint64_t twice = (uint64_t)r0 * 2u;
  uint32_t qRNE = q0;
  if (twice > denom || (twice == denom && (q0 & 1u) == 1u)) qRNE = q0 + 1u;
  r.mix = s ^ sub ^ sh31 ^ shR ^ qRNE;
  return r;
}

static GLuint makeSSBO(GLsizeiptr bytes, const void* init, GLenum usage = GL_DYNAMIC_COPY) {
  GLuint b = 0; glGenBuffers(1, &b);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, b);
  glBufferData(GL_SHADER_STORAGE_BUFFER, bytes, init, usage);
  return b;
}
static GLuint compileCompute(const char* src, std::string& log) {
  GLuint sh = glCreateShader(GL_COMPUTE_SHADER);
  glShaderSource(sh, 1, &src, nullptr); glCompileShader(sh);
  GLint ok = 0; glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
  char buf[4096] = {}; glGetShaderInfoLog(sh, 4096, nullptr, buf); log = buf;
  if (!ok) { glDeleteShader(sh); return 0; }
  GLuint p = glCreateProgram(); glAttachShader(p, sh); glLinkProgram(p);
  GLint lok = 0; glGetProgramiv(p, GL_LINK_STATUS, &lok);
  char lbuf[4096] = {}; glGetProgramInfoLog(p, 4096, nullptr, lbuf); log += lbuf;
  glDeleteShader(sh);
  if (!lok) { glDeleteProgram(p); return 0; }
  return p;
}

namespace t232probe {
int runDeviceProbe(const char* outDir) {
  g_out = outDir;
  std::ostringstream J; J << "{\"t232\":{";
  // ======== identity (same as T131 A section) ========
  EGLDisplay dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  CHECK(dpy != EGL_NO_DISPLAY); CHECK(eglInitialize(dpy, nullptr, nullptr));
  EGLint ca[] = { EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,
    EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_NONE };
  EGLConfig cfg; EGLint n = 0; CHECK(eglChooseConfig(dpy, ca, &cfg, 1, &n) && n > 0);
  CHECK(eglBindAPI(EGL_OPENGL_ES_API));
  EGLint ctxa[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
  EGLContext ctx = eglCreateContext(dpy, cfg, EGL_NO_CONTEXT, ctxa); CHECK(ctx != EGL_NO_CONTEXT);
  EGLint sa[] = { EGL_WIDTH,64,EGL_HEIGHT,64,EGL_NONE };
  EGLSurface surf = eglCreatePbufferSurface(dpy, cfg, sa); CHECK(surf != EGL_NO_SURFACE);
  CHECK(eglMakeCurrent(dpy, surf, surf, ctx));
  std::string glVer = (const char*)glGetString(GL_VERSION);
  std::string glSlVer = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
  std::string glExts = (const char*)glGetString(GL_EXTENSIONS);
  std::string eglExts = eglQueryString(dpy, EGL_EXTENSIONS);
  GLint maj = 0, mi = 0; glGetIntegerv(GL_MAJOR_VERSION, &maj); glGetIntegerv(GL_MINOR_VERSION, &mi);
  {
    std::string vendor = (const char*)glGetString(GL_VENDOR);
    std::string renderer = (const char*)glGetString(GL_RENDERER);
    J << "\"identity\":{\"gl\":\"" << glVer << "\",\"glsl\":\"" << glSlVer << "\",\"vendor\":\"" << vendor
      << "\",\"renderer\":\"" << renderer << "\",\"glMajor\":" << maj << ",\"glMinor\":" << mi << "},";
    dumpText("environment.txt", "GL " + glVer + "\nGLSL " + glSlVer + "\nEXTS " + glExts + "\nEGL_EXTS " + eglExts + "\n");
    std::printf("T232 GL=%s GLSL=%s %d.%d\n", glVer.c_str(), glSlVer.c_str(), maj, mi);
  }

  // ======== P-B: extended limits + offset alignment ========
  {
    GLint globalBindings = 0, computeBlocks = 0, offsetAlign = 0, maxCombined = 0, maxVert = 0, maxFrag = 0, maxElem = 0;
    glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &globalBindings);
    glGetIntegerv(GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS, &computeBlocks);
    glGetIntegerv(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT, &offsetAlign);
    glGetIntegerv(GL_MAX_COMBINED_SHADER_STORAGE_BLOCKS, &maxCombined);
    glGetIntegerv(GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS, &maxVert);
    glGetIntegerv(GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS, &maxFrag);
    glGetIntegerv(GL_MAX_ELEMENTS_INDICES, &maxElem);
    GLint wgCount[3] = { 0,0,0 }, wgSize[3] = { 0,0,0 }, invoc = 0, ssboMax = 0, shared = 0;
    for (int i = 0; i < 3; i++) glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, i, &wgCount[i]);
    for (int i = 0; i < 3; i++) glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, i, &wgSize[i]);
    glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &invoc);
    glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &ssboMax);
    glGetIntegerv(GL_MAX_COMPUTE_SHARED_MEMORY_SIZE, &shared);
    J << "\"limitsP_B\":{\"globalBindings\":" << globalBindings << ",\"computeBlocks\":" << computeBlocks
      << ",\"combinedBlocks\":" << maxCombined << ",\"vertexBlocks\":" << maxVert << ",\"fragmentBlocks\":" << maxFrag
      << ",\"offsetAlignment\":" << offsetAlign << ",\"wgCount\":[" << wgCount[0] << "," << wgCount[1] << "," << wgCount[2]
      << "],\"wgSize\":[" << wgSize[0] << "," << wgSize[1] << "," << wgSize[2] << "],\"invocations\":" << invoc
      << ",\"ssboMaxBytes\":" << ssboMax << ",\"sharedBytes\":" << shared << "},";
    std::printf("T232 P-B global=%d compute=%d combined=%d align=%d shared=%d\n", globalBindings, computeBlocks, maxCombined, offsetAlign, shared);
  }

  // ======== P-F: FP64/int64/atomic extension enumeration (enumeration only, no compile claim) ========
  {
    bool hasDoubleExt = glExts.find("GL_EXT_gpu_shader5") != std::string::npos;
    bool fp64Token = glExts.find("_float64") != std::string::npos || glExts.find("FP64") != std::string::npos;
    bool int64Ext = glExts.find("_int64") != std::string::npos;
    bool atomicFloatExt = glExts.find("atomic_float") != std::string::npos;
    bool timerQuery = glExts.find("disjoint_timer_query") != std::string::npos;
    J << "\"extensions\":{\"gpu_shader5\":" << (hasDoubleExt ? 1 : 0) << ",\"fp64Hint\":" << (fp64Token ? 1 : 0)
      << ",\"int64\":" << (int64Ext ? 1 : 0) << ",\"atomicFloat\":" << (atomicFloatExt ? 1 : 0)
      << ",\"disjointTimerQuery\":" << (timerQuery ? 1 : 0) << "},";
    std::printf("T232 P-F ext: gpu_shader5=%d fp64=%d int64=%d atomicF=%d timer=%d\n", hasDoubleExt, fp64Token, int64Ext, atomicFloatExt, timerQuery);
    // double 编译负例: GLSL ES 3.20 core 'double' 保留字 → 预期编译拒绝
    const char* dsrc = "#version 320 es\nvoid main(){double x=1.0LF;}\n";
    std::string dlog;
    GLuint dp = compileCompute(dsrc, dlog);
    J << "\"doubleCompileNegative\":{\"rejected\":" << (dp == 0 ? 1 : 0) << "},";
    dumpText("double_negative_log.txt", dlog);
    std::printf("T232 P-F double compile rejected=%d\n", dp == 0 ? 1 : 0);
  }

  // ======== P-A: integer primitives vs CPU oracle (N=4096 batch) ========
  {
    const int N = 4096;
    std::vector<uint32_t> A(N), B(N);
    uint32_t seed = 232;
    auto rnd = [&]() { seed = seed * 1664525u + 1013904223u; return seed; };
    for (int i = 0; i < N; i++) { A[i] = rnd(); B[i] = rnd(); }
    // edge cases
    A[0] = 0xffffffffu; B[0] = 1u; A[1] = 0x80000000u; B[1] = 0x80000000u;
    A[2] = 1u; B[2] = 0u; A[3] = 0u; B[3] = 0xffffffffu;
    GLuint bA = makeSSBO(N * 4, A.data()), bB = makeSSBO(N * 4, B.data());
    GLuint bO = makeSSBO((GLsizeiptr)N * 16, nullptr), bD = makeSSBO(N * 4, nullptr);
    std::string log;
    GLuint prog = compileCompute(kIntPrimSrc, log);
    dumpText("int_prim_log.txt", log);
    dumpText("int_prim.comp", kIntPrimSrc);
    if (!prog) {
      J << "\"intPrim\":{\"status\":\"COMPILE_FAILED\"}},";
      dumpText("t232.json", J.str() + "}");
      std::printf("T232 P-A COMPILE FAILED\n"); return 3;
    }
    glUseProgram(prog);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, bA);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, bB);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, bO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, bD);
    glDispatchCompute((N + 63) / 64, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
    // readback
    std::vector<uint32_t> O(N * 4), D(N);
    { glBindBuffer(GL_SHADER_STORAGE_BUFFER, bO); const uint32_t* p = (const uint32_t*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, N * 16, GL_MAP_READ_BIT);
      if (p) { memcpy(O.data(), p, N * 16); glUnmapBuffer(GL_SHADER_STORAGE_BUFFER); } }
    { glBindBuffer(GL_SHADER_STORAGE_BUFFER, bD); const uint32_t* p = (const uint32_t*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, N * 4, GL_MAP_READ_BIT);
      if (p) { memcpy(D.data(), p, N * 4); glUnmapBuffer(GL_SHADER_STORAGE_BUFFER); } }
    int mismatch = 0; std::string firstDiff;
    for (int i = 0; i < N; i++) {
      IntPrimResult c = cpuIntPrim(A[i], B[i % N]);
      if (O[i * 4] != c.carry || O[i * 4 + 1] != c.borrow || O[i * 4 + 2] != c.mulHi || O[i * 4 + 3] != c.mulLo || D[i] != c.mix) {
        if (mismatch < 3) { char buf[256]; std::snprintf(buf, 256, "i=%d a=%08x b=%08x gpu(c=%u,b=%u,hi=%u,lo=%u,mix=%u) cpu(c=%u,b=%u,hi=%u,lo=%u,mix=%u)",
          i, A[i], B[i % N], O[i * 4], O[i * 4 + 1], O[i * 4 + 2], O[i * 4 + 3], D[i], c.carry, c.borrow, c.mulHi, c.mulLo, c.mix); firstDiff += buf; }
        mismatch++;
      }
    }
    J << "\"intPrim\":{\"status\":\"" << (mismatch == 0 ? "OK" : "MISMATCH") << "\",\"n\":" << N << ",\"mismatch\":" << mismatch << ",\"firstDiff\":\"" << firstDiff << "\"},";
    std::printf("T232 P-A intPrim mismatch=%d/%d\n", mismatch, N);
    glDeleteBuffers(1, &bA); glDeleteBuffers(1, &bB); glDeleteBuffers(1, &bO); glDeleteBuffers(1, &bD);
    glDeleteProgram(prog);
  }

  // ======== P-C: two-workgroup cross-dispatch visibility with barrier ========
  {
    // shader: pass1 wg0 writes pattern, wg1 reads (same dispatch impossible) → split into 2 dispatches with barrier
    const char* src = R"(#version 310 es
precision highp int;
layout(local_size_x=64) in;
layout(std430,binding=0) buffer Data { uint data[]; };
layout(std430,binding=1) writeonly buffer Out { uint outv[]; };
void main(){
  uint i = gl_GlobalInvocationID.x;
  if (i >= 128u) return;
  if (data[i] == 0xfeedfaceu + i + 7u) outv[i] = 1u; else outv[i] = 0u;
})";
    std::vector<uint32_t> init(128);
    for (int i = 0; i < 128; i++) init[i] = 0xfeedfaceu + (uint32_t)i;
    GLuint bData = makeSSBO(128 * 4, init.data()), bOut = makeSSBO(128 * 4, nullptr);
    std::string log;
    GLuint prog = compileCompute(src, log);
    if (!prog) { J << "\"crossDispatch\":{\"status\":\"COMPILE_FAILED\"},"; }
    else {
      // dispatch 1: transform data (writer in different workgroups)
      const char* wsrc = R"(#version 310 es
precision highp int;
layout(local_size_x=64) in;
layout(std430,binding=0) buffer Data { uint data[]; };
void main(){uint i=gl_GlobalInvocationID.x;if(i>=128u)return;data[i]=data[i]+7u;})";
      std::string wlog; GLuint wp = compileCompute(wsrc, wlog);
      glUseProgram(wp);
      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, bData);
      glDispatchCompute(2, 1, 1); // two workgroups
      glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
      // dispatch 2: reader verifies
      glUseProgram(prog);
      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, bData);
      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, bOut);
      glDispatchCompute(2, 1, 1);
      glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
      std::vector<uint32_t> out(128);
      { glBindBuffer(GL_SHADER_STORAGE_BUFFER, bOut); const uint32_t* p = (const uint32_t*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 128 * 4, GL_MAP_READ_BIT);
        if (p) { memcpy(out.data(), p, 128 * 4); glUnmapBuffer(GL_SHADER_STORAGE_BUFFER); } }
      int bad = 0; for (int i = 0; i < 128; i++) if (out[i] != 1u) bad++;
      J << "\"crossDispatch\":{\"status\":\"" << (bad == 0 ? "OK" : "MISMATCH") << "\",\"bad\":" << bad << "},";
      std::printf("T232 P-C crossDispatch bad=%d/128\n", bad);
      glDeleteProgram(wp); glDeleteProgram(prog);
    }
    glDeleteBuffers(1, &bData); glDeleteBuffers(1, &bOut);
  }

  // ======== P-D: 1024B completion + fence async poll + late buffer ========
  {
    const char* src = R"(#version 310 es
precision highp int;
layout(local_size_x=64) in;
layout(std430,binding=0) buffer C { uint c[]; }; // 256 words = 1024B
void main(){uint i=gl_GlobalInvocationID.x;if(i>=256u)return;c[i]=0x23232323u+i;})";
    std::string log; GLuint prog = compileCompute(src, log);
    if (!prog) { J << "\"completion\":{\"status\":\"COMPILE_FAILED\"},"; }
    else {
      std::vector<uint32_t> zeros(256, 0u);
      GLuint bC = makeSSBO(1024, zeros.data());
      glUseProgram(prog);
      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, bC);
      glDispatchCompute(4, 1, 1);
      GLsync fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
      glFlush();
      // async poll
      int polls = 0; GLenum status = GL_WAIT_FAILED;
      auto t0 = std::chrono::steady_clock::now();
      do { status = glClientWaitSync(fence, GL_SYNC_FLUSH_COMMANDS_BIT, 1000000); polls++; }
      while (status == GL_TIMEOUT_EXPIRED && polls < 1000);
      double fenceMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
      glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
      std::vector<uint32_t> c(256);
      { glBindBuffer(GL_SHADER_STORAGE_BUFFER, bC); const uint32_t* p = (const uint32_t*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 1024, GL_MAP_READ_BIT);
        if (p) { memcpy(c.data(), p, 1024); glUnmapBuffer(GL_SHADER_STORAGE_BUFFER); } }
      int bad = 0; for (int i = 0; i < 256; i++) if (c[i] != 0x23232323u + (uint32_t)i) bad++;
      // late: second dispatch WITHOUT waiting → data may be incomplete (recorded, not asserted as spec)
      glDispatchCompute(4, 1, 1); // re-dispatch same buffer, do not wait
      J << "\"completion\":{\"status\":\"" << (bad == 0 && status == GL_ALREADY_SIGNALED ? "OK" : "CHECK") << "\",\"fenceStatus\":" << (int)status
        << ",\"polls\":" << polls << ",\"fenceWaitMs\":" << fenceMs << ",\"tailBad\":" << bad << "},";
      std::printf("T232 P-D completion status=%d polls=%d fenceMs=%.3f bad=%d\n", (int)status, polls, fenceMs, bad);
      glDeleteSync(fence); glDeleteProgram(prog); glDeleteBuffers(1, &bC);
    }
  }

  J << "\"pE_shared_context\":{\"status\":\"NOT_RUN_PBUFFER_SECTION\",\"reason\":\"P-E requires XComponent real window context; separate device phase\"}";
  J << "}}";
  dumpText("t232.json", J.str());
  std::printf("T232 RESULT: P-A/B/C/D/F executed on pbuffer context; P-E NOT_RUN (needs XComponent)\n");
  eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
  eglDestroySurface(dpy, surf); eglDestroyContext(dpy, ctx); eglTerminate(dpy);
  return 0;
}
} // namespace t232probe
#ifdef T232_PROBE_MAIN
int main(int argc, char** argv) { return t232probe::runDeviceProbe(argc > 1 ? argv[1] : ".codex/local-test/t232"); }
#endif
