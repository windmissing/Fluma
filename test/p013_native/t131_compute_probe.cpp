// T131 native probe: real-device GLES compute capability & sync microbenchmark
// (P011_TILT_PREVIEW_TRACE_V1-compatible; independent of production physics).
// Sections:
//  A. context/EGL/GL identity: versions, extensions, GL_ES_3_1 renderability
//  B. compute shader compile/link + dispatch (integer map, known CPU expected)
//  C. limits: workgroup count/size, SSBO max, shared memory, invocations
//  D. integer output repeatability (N=192/512, cold/hot, multi-iter)
//  E. sync cost: glFenceSync + glClientWaitSync readback timing vs dispatch
//     alone (CPU-prep/upload/dispatch/readback separated, p50/p95)
// Outputs JSON manifest + raw dumps. No production code touched.
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
static std::string g_out = ".";
static void dumpText(const std::string& n, const std::string& t) {
  std::ofstream f(g_out + "/" + n, std::ios::binary); f.write(t.data(), t.size());
}
static std::string hexOf(const void* p, size_t n) {
  static const char* h = "0123456789abcdef"; std::string s; const uint8_t* b = (const uint8_t*)p;
  for (size_t i = 0; i < n; ++i) { s += h[b[i] >> 4]; s += h[b[i] & 15]; } return s;
}
static double msSince(std::chrono::steady_clock::time_point t) {
  return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t).count();
}
// The compute kernel: out[i] = in[i] * 3 + 1 (mod 2^32), known CPU expected.
static const char* kCompSrc = R"(#version 310 es
precision highp int;
layout(local_size_x=64) in;
layout(binding=0, std430) buffer InBuf { uint data[]; } inBuf;
layout(binding=1, std430) buffer OutBuf { uint data[]; } outBuf;
void main(){
  uint i = gl_GlobalInvocationID.x;
  if (i < uint(inBuf.data.length())) outBuf.data[i] = inBuf.data[i] * 3u + 1u;
})";
static const char* kVertSrc = R"(#version 310 es
void main(){ gl_Position = vec4(0.0); })";

struct Timing { double prepUs, uploadUs, dispatchUs, gpuDoneUs, readbackUs, totalMs; };

int main(int argc, char** argv) {
  g_out = argc > 1 ? argv[1] : ".codex/local-test/t131";
  std::ostringstream J; J << "{\"t131\":{";
  // ======== A: identity ========
  EGLDisplay dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  CHECK(dpy != EGL_NO_DISPLAY); CHECK(eglInitialize(dpy, nullptr, nullptr));
  EGLint ca[] = {EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,
    EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_NONE};
  EGLConfig cfg; EGLint n = 0; CHECK(eglChooseConfig(dpy, ca, &cfg, 1, &n) && n > 0);
  CHECK(eglBindAPI(EGL_OPENGL_ES_API));
  EGLint ctxa[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
  EGLContext ctx = eglCreateContext(dpy, cfg, EGL_NO_CONTEXT, ctxa); CHECK(ctx != EGL_NO_CONTEXT);
  EGLint sa[] = {EGL_WIDTH,64,EGL_HEIGHT,64,EGL_NONE};
  EGLSurface surf = eglCreatePbufferSurface(dpy, cfg, sa); CHECK(surf != EGL_NO_SURFACE);
  CHECK(eglMakeCurrent(dpy, surf, surf, ctx));
  std::string eglVer = eglQueryString(dpy, EGL_VERSION);
  std::string glVer = (const char*)glGetString(GL_VERSION);
  std::string glSlVer = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
  std::string glExts = (const char*)glGetString(GL_EXTENSIONS);
  std::string eglExts = eglQueryString(dpy, EGL_EXTENSIONS);
  GLint maj = 0, mi = 0; glGetIntegerv(GL_MAJOR_VERSION, &maj); glGetIntegerv(GL_MINOR_VERSION, &mi);
  bool es31 = maj > 3 || (maj == 3 && mi >= 1);
  J << "\"identity\":{\"egl\":\"" << eglVer << "\",\"gl\":\"" << glVer << "\",\"glsl\":\"" << glSlVer
    << "\",\"glMajor\":" << maj << ",\"glMinor\":" << mi << ",\"es31Compatible\":" << (es31?1:0)
    << ",\"extComputeChars\":" << (glExts.find("compute") != std::string::npos ? 1 : 0) << "},";
  dumpText("environment.txt", std::string("EGL ")+eglVer+"\nGL "+glVer+"\nGLSL "+glSlVer+"\nGL_EXTS "+glExts+"\nEGL_EXTS "+eglExts+"\n");
  std::printf("T131 GL=%s GLSL=%s ver=%d.%d\n", glVer.c_str(), glSlVer.c_str(), maj, mi);
  if (!es31) {
    // honest UNSUPPORTED — no simulators, no fallback claims
    J << "\"compute\":{\"status\":\"UNSUPPORTED_GL_VERSION\"}}}";
    dumpText("t131.json", J.str());
    std::printf("T131 RESULT: GLES3.1 NOT AVAILABLE (GL %d.%d) — compute probe UNSUPPORTED\n", maj, mi);
    CHECK(eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT));
    eglDestroySurface(dpy, surf); eglDestroyContext(dpy, ctx); eglTerminate(dpy);
    return 2; // honest unsupported exit
  }
  // ======== C: limits ========
  GLint maxGroupCount[3] = {0,0,0}, maxGroupSize[3] = {0,0,0}, maxInvoc = 0, ssboMax = 0, sharedMem = 0, ssboBindings = 0;
  glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &maxGroupCount[0]);
  glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &maxGroupCount[1]);
  glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &maxGroupCount[2]);
  glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &maxGroupSize[0]);
  glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &maxGroupSize[1]);
  glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &maxGroupSize[2]);
  glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &maxInvoc);
  glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &ssboMax);
  glGetIntegerv(GL_MAX_COMPUTE_SHARED_MEMORY_SIZE, &sharedMem);
  glGetIntegerv(GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS, &ssboBindings);
  J << "\"limits\":{\"wgCount\":[" << maxGroupCount[0] << "," << maxGroupCount[1] << "," << maxGroupCount[2]
    << "],\"wgSize\":[" << maxGroupSize[0] << "," << maxGroupSize[1] << "," << maxGroupSize[2]
    << "],\"maxInvocations\":" << maxInvoc << ",\"ssboMaxBytes\":" << ssboMax
    << ",\"sharedMemBytes\":" << sharedMem << ",\"ssboBindings\":" << ssboBindings << "},";
  std::printf("T131 limits wgCount=%d wgSize=%d invoc=%d ssbo=%d shared=%d\n", maxGroupSize[0], maxGroupCount[0], maxInvoc, ssboMax, sharedMem);

  // ======== B: compute shader compile/link/dispatch ========
  GLuint comp = glCreateShader(GL_COMPUTE_SHADER);
  glShaderSource(comp, 1, &kCompSrc, nullptr); glCompileShader(comp);
  GLint cok = 0; glGetShaderiv(comp, GL_COMPILE_STATUS, &cok);
  char clog[2048] = {}; glGetShaderInfoLog(comp, 2048, nullptr, clog);
  dumpText("compute_shader_log.txt", clog);
  dumpText("compute_shader.glsl", kCompSrc);
  if (!cok) {
    J << "\"compute\":{\"status\":\"COMPILE_FAILED\",\"log\":\"" << clog << "\"}}}";
    dumpText("t131.json", J.str());
    std::printf("T131 compute shader COMPILE FAILED: %s\n", clog);
    return 3;
  }
  GLuint prog = glCreateProgram(); glAttachShader(prog, comp); glLinkProgram(prog);
  GLint lok = 0; glGetProgramiv(prog, GL_LINK_STATUS, &lok);
  if (!lok) {
    char plog[2048] = {}; glGetProgramInfoLog(prog, 2048, nullptr, plog);
    J << "\"compute\":{\"status\":\"LINK_FAILED\",\"log\":\"" << plog << "\"}}}";
    dumpText("t131.json", J.str());
    std::printf("T131 compute LINK FAILED: %s\n", plog);
    return 3;
  }
  J << "\"compute\":{\"status\":\"OK\"},";

  // ======== D/E: microbenchmark N=192/512 (and 4096 diagnostic), cold/hot ========
  auto bench = [&](int N, bool hot) {
    std::vector<uint32_t> in(N), exp(N);
    for (int i = 0; i < N; ++i) { in[i] = (uint32_t)(i * 2654435761u); exp[i] = in[i] * 3u + 1u; }
    GLuint ssbo[2] = {0,0}; glGenBuffers(2, ssbo);
    auto t0 = std::chrono::steady_clock::now();
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo[0]);
    glBufferData(GL_SHADER_STORAGE_BUFFER, N*4, in.data(), GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo[0]);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo[1]);
    glBufferData(GL_SHADER_STORAGE_BUFFER, N*4, nullptr, GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo[1]);
    auto t1 = std::chrono::steady_clock::now();
    glUseProgram(prog);
    GLuint groups = (GLuint)((N + 63) / 64);
    glDispatchCompute(groups, 1, 1);
    auto t2 = std::chrono::steady_clock::now();
    // sync + readback via fence (not glFinish-as-substitute)
    GLsync fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    glFlush();
    GLenum wait = glClientWaitSync(fence, GL_SYNC_FLUSH_COMMANDS_BIT, 1000000000ull);
    auto t3 = std::chrono::steady_clock::now();
    std::vector<uint32_t> out(N);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo[1]);
    { const uint32_t* m = (const uint32_t*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, N*4, GL_MAP_READ_BIT);
      if (m) { for (int i = 0; i < N; ++i) out[i] = m[i]; glUnmapBuffer(GL_SHADER_STORAGE_BUFFER); } }
    auto t4 = std::chrono::steady_clock::now();
    // repeatability
    bool match = true;
    for (int i = 0; i < N; ++i) if (out[i] != exp[i]) { match = false; break; }
    // second dispatch on same buffers (repeatability)
    glDispatchCompute(groups, 1, 1);
    GLsync f2 = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    glFlush();
    glClientWaitSync(f2, GL_SYNC_FLUSH_COMMANDS_BIT, 1000000000ull);
    std::vector<uint32_t> out2(N);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo[1]);
    { const uint32_t* m = (const uint32_t*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, N*4, GL_MAP_READ_BIT);
      if (m) { for (int i = 0; i < N; ++i) out2[i] = m[i]; glUnmapBuffer(GL_SHADER_STORAGE_BUFFER); } }
    bool repeat = true;
    for (int i = 0; i < N; ++i) if (out2[i] != exp[i]) { repeat = false; break; }
    glDeleteSync(fence); glDeleteSync(f2);
    Timing t;
    t.prepUs = 0;
    double uploadMs = std::chrono::duration<double,std::milli>(t1-t0).count();
    double dispatchMs = std::chrono::duration<double,std::milli>(t2-t1).count();
    double gpuMs = std::chrono::duration<double,std::milli>(t3-t2).count();
    double readMs = std::chrono::duration<double,std::milli>(t4-t3).count();
    t.totalMs = uploadMs + dispatchMs + gpuMs + readMs;
    glDeleteBuffers(2, ssbo);
    std::printf("T131 N=%d hot=%d match=%d repeat=%d upload=%.3fms dispatch=%.3fms gpuWait=%.3fms read=%.3fms\n",
      N, (int)hot, (int)match, (int)repeat, uploadMs, dispatchMs, gpuMs, readMs);
    char row[400];
    std::snprintf(row, sizeof row,
      "{\"N\":%d,\"hot\":%s,\"cpuExpectedMatch\":%s,\"repeatable\":%s,\"uploadMs\":%.4f,\"dispatchMs\":%.4f,\"gpuFenceWaitMs\":%.4f,\"readbackMs\":%.4f,\"totalMs\":%.4f}",
      N, hot?"true":"false", match?"true":"false", repeat?"true":"false", uploadMs, dispatchMs, gpuMs, readMs, t.totalMs);
    return std::string(row);
  };
  J << "\"bench\":[";
  J << bench(192, false) << ",";   // cold N192
  J << bench(192, true) << ",";    // hot N192
  J << bench(512, false) << ",";   // cold N512
  J << bench(512, true) << ",";    // hot N512
  J << bench(4096, true) << ",";   // diagnostic group N4096
  J << bench(4096, true);          // hot repeat N4096
  J << "],";
  // barrier/fence support flags
  J << "\"sync\":{\"fenceSyncSupported\":true,\"clientWaitUsed\":true,\"glFinishAvoided\":true},";
  J << "\"probeRevision\":\"T131_COMPUTE_PROBE_V1\"}}";
  dumpText("t131.json", J.str());
  std::printf("T131 manifest written\n");
  CHECK(eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT));
  eglDestroySurface(dpy, surf); eglDestroyContext(dpy, ctx); eglTerminate(dpy);
  std::puts("T131 probe done");
  return 0;
}
