// T260 independent GL host (tester). Own ANGLE SwiftShader pbuffer driver;
// product transport/scene/control objects are exercised as the system under
// test, but scenario selection, capture and evidence export are independent.
// PC software GL only; not a device execution.
#include "source_transport.h"
#include "scene.h"
#include "run.h"
#include "foundation.h"
#include "angles_generated.h"
#include "source_hash.h"
#include <fstream>
#include <string>
#include <iostream>
#include <cassert>
#include <chrono>
#include <sstream>
using namespace grs;
namespace { std::string kernel; const char* kernelHash = kHostSourceHash; }
namespace grs {
const char* SourceKernelSource() { return kernel.c_str(); } const char* SourceKernelSha() { return kernelHash; }
FoundationInfo GetFoundationInfo() { return { "GRS_Q20_V1","GRS_FIXED120_TIME_V1","GRS_GPU_SCENE_Q20_V1",kLayoutManifestSha256,kAngleSha256,"HOST_NOT_USED",0,false,false }; }
const uint32_t* ColdAngleWords(size_t& size) { size = 61444; return kAngleWords; }
}
void hashWords(const char* s, std::array<uint32_t, 8>& out) { for (size_t i = 0; i < 8; i++) { uint32_t value = 0; for (size_t j = 0; j < 4; j++) { char hex[3] = { s[i * 8 + j * 2], s[i * 8 + j * 2 + 1], 0 }; value |= uint32_t(std::stoul(hex, nullptr, 16)) << (8 * j); } out[i] = value; } }
void raw(std::array<uint32_t, 32>& a, uint32_t at, double v) { uint64_t bits; std::memcpy(&bits, &v, 8); a[at] = uint32_t(bits); a[at + 1] = uint32_t(bits >> 32); }
static std::ofstream trace;
static void dumpWords(const char* name, const std::vector<uint32_t>& w) {
 trace << name << " " << w.size();
 for (auto x : w) trace << " " << x;
 trace << "\n";
}
static void dumpArr(const char* name, const uint32_t* w, size_t n) { trace << name << " " << n; for (size_t i = 0; i < n; i++) trace << " " << w[i]; trace << "\n"; }
static void capture(SourceGpuTransport& gpu, const char* tag) {
 if (gpu.FreezeDiagnostics(true) != TransportCode::OK) { trace << "CAPTURE_FAIL " << tag << "\n"; return; }
 std::vector<uint32_t> state; gpu.CaptureAcceptedState(state); dumpWords("STATE", state);
 gpu.FreezeDiagnostics(false);
 trace << "HEADER";
 const auto& h = gpu.Owner().Header();
 for (size_t i = 0; i < 256; i++) trace << " " << h[i];
 trace << "\n";
}
int main(int argc, char** argv) {
 trace.open("test/reports/t260/host_trace.txt", std::ios::binary);
 // SwiftShader explicit software platform (ANGLE official eglext_angle.h constants)
 using GetPlatform = EGLDisplay(EGLAPIENTRYP)(EGLenum, void*, const EGLint*);
 auto platform = reinterpret_cast<GetPlatform>(eglGetProcAddress("eglGetPlatformDisplayEXT")); assert(platform);
 EGLint platformArgs[] = { 0x3203,0x3450,0x3209,0x3487,EGL_NONE };
 EGLDisplay display = platform(0x3202, nullptr, platformArgs);
 EGLint major = 0, minor = 0; assert(eglInitialize(display, &major, &minor)); EGLConfig config = nullptr; EGLint count = 0;
 EGLint attrs[] = { EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_NONE }; assert(eglChooseConfig(display, attrs, &config, 1, &count) && count > 0); assert(eglBindAPI(EGL_OPENGL_ES_API));
 EGLint ca[] = { EGL_CONTEXT_CLIENT_VERSION,3,EGL_CONTEXT_MINOR_VERSION,1,EGL_NONE }; EGLContext context = eglCreateContext(display, config, EGL_NO_CONTEXT, ca);
 EGLint pa[] = { EGL_WIDTH,256,EGL_HEIGHT,1024,EGL_NONE }; EGLSurface surface = eglCreatePbufferSurface(display, config, pa); assert(context != EGL_NO_CONTEXT && surface != EGL_NO_SURFACE && eglMakeCurrent(display, surface, surface, context));
 const char* version = reinterpret_cast<const char*>(glGetString(GL_VERSION)); const char* renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
 trace << "GL " << version << " | " << renderer << "\n";
 std::ifstream file(".codex/local-test/t236-shaders/m0_source.comp", std::ios::binary); kernel.assign(std::istreambuf_iterator<char>(file), {}); assert(!kernel.empty());
 Identity id; id.run = { 7,0 }; id.owner = { 8,0 }; id.context = { 9,0 }; id.surface = { 10,0 }; id.geometry = { 11,0 }; id.config = { 1,0 };
 hashWords(kLayoutManifestSha256, id.layoutSha); hashWords(kAngleSha256, id.angleSha); hashWords(kernelHash, id.shaderSha);
 SourceGpuTransport gpu; auto created = gpu.Create(192, id); trace << "CREATE " << int(created) << "\n";
 GLint maxWG[3]={0,0,0}; glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT,0,&maxWG[0]);glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT,1,&maxWG[1]);glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT,2,&maxWG[2]);
 trace << "LIMITS wgcount " << maxWG[0] << " " << maxWG[1] << " " << maxWG[2] << "\n";
 if (created != TransportCode::OK) return 2;
 uint32_t frame = 0, nonce = 0;
 std::array<uint32_t, 32> command{}; command[0] = wire::magicWords::sourceCommand; command[1] = 1; command[4] = 7; command[5] = 0; command[6] = 100; command[8] = 1; command[10] = 1; command[12] = 11; command[14] = 1; command[15] = 1; command[22] = 16; command[24] = 1; command[26] = 1; command[27] = 1; command[28] = 1;
 raw(command, 16, 64.5); raw(command, 18, 100.); raw(command, 20, .25);
 auto submit = [&](std::array<uint32_t, 32> cmd, bool stop = false) {
  frame++; RootRequest r; r.nonce = { ++nonce,0 }; r.control = { frame,0 }; r.frame = { frame,0 }; r.publicationSlot = (frame - 1) % 2; r.eventArena = (frame - 1) % 2; r.eventCapacity = 133 * 192 + 508;
  auto s = gpu.Submit(cmd, r); assert(s == TransportCode::OK); if (stop) assert(gpu.Stop() == ControlCode::QUARANTINED);
  AcceptedRoot out; auto start = std::chrono::steady_clock::now();
  TransportCode code; do { code = gpu.Poll(out); if (code != TransportCode::BUSY) break; std::this_thread::yield(); } while (std::chrono::steady_clock::now() - start < std::chrono::seconds(30));
  trace << "GLERR " << gpu.Stats().firstGlError << " DISPATCHES " << gpu.Stats().dispatches << "\n";
  trace << "ROOT " << frame << " code=" << int(code) << " outcome=" << out.outcome << " source=" << out.source.lo << " physical=" << out.physics.lo << " success=" << out.sourceSuccess << "\n";
  dumpArr("COMPLETION", out.completion.data(), 256);
  dumpArr("LASTCOMPLETION", gpu.LastCompletion().data(), 256);
  capture(gpu, "root");
  return out;
 };
 auto first = submit(command); assert(first.sourceSuccess);
 trace << "PHASE_SOURCE_OK\n";
 // quantum sequence: free fall 4 quanta at angle 0
 frame++; uint32_t frameSourceEnd = 0;
 for (uint32_t q = 1; q <= 4; q++) {
  RootRequest r; r.nonce = { ++nonce,0 }; r.control = { frame,0 }; r.frame = { frame,0 }; r.quantumOrdinal = q; r.publicationSlot = (nonce - 1) % 2; r.eventArena = (frame - 1) % 2; r.eventCapacity = 133 * 192 + 508;
  assert(gpu.SubmitQuantum(r, 0) == TransportCode::OK);
  AcceptedRoot out; TransportCode code; auto pollDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
  do { code = gpu.Poll(out); if (code != TransportCode::BUSY) break; std::this_thread::yield(); } while (std::chrono::steady_clock::now() < pollDeadline);
  const auto& rc = gpu.LastCompletion();
  trace << "QUANTUM " << q << " code=" << int(code) << " reason=" << rc[29] << " stage=" << rc[30] << " mask=" << rc[97] << " audit=" << rc[99] << " motion=" << rc[94] << " bed=" << rc[95] << " angle=" << int32_t(rc[107]) << "\n";
  dumpArr("COMPLETION", out.completion.data(), 256);
  dumpArr("LASTCOMPLETION", gpu.LastCompletion().data(), 256);
  capture(gpu, "quantum");
  assert(code == TransportCode::OK && !out.sourceSuccess && out.source.lo == 1 && out.physics.lo == 1 + q);
  frameSourceEnd = out.completion[53];
 }
 trace << "PHASE_QUANTUM_OK\n";
 trace.close();
 std::cout << "{\"scope\":\"T260_INDEPENDENT_SWIFTSHADER_CAPTURE\",\"ok\":true}" << std::endl;
 auto released = gpu.Release(); assert(released == TransportCode::OK);
 eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT); eglDestroySurface(display, surface); eglDestroyContext(display, context); eglTerminate(display);
 return 0;
}
