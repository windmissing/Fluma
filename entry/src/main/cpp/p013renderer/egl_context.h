// P-013 P0 v1. All operations are serialized by the session mutex.
#pragma once
#include <EGL/egl.h>
#include <ace/xcomponent/native_interface_xcomponent.h>

#include <thread>
#include <string>

#include "render_contract.h"
namespace p013 {
struct SurfaceKey {
  uint64_t epoch = 0;
  int width = 0, height = 0;
};
struct SurfaceInfo {
  int width = 0, height = 0;
  uint64_t epoch = 0;
};
enum class ResetReason { BACKGROUND, DESTROYED, RELEASE, FAILURE };
class EglContext {
 public:
  Status ConfigureComposition(bool);
  std::string eglVersion, eglExtensions, glVersion, glExtensions;
  bool composition = false;
  int drawFramebuffer = -1, readFramebuffer = -1;
  int alphaSize = -1, actualColorSpace = 0, attachmentEncoding = 0;
  bool writeControlSupported = false, srgbWriteEnabled = false;
  Status OnWindowCreated(OH_NativeXComponent*, void*);
  Status OnWindowChanged(OH_NativeXComponent*, void*);
  void OnWindowDestroyed(OH_NativeXComponent*, void*);
  Status PrepareCurrent(uint64_t, SurfaceInfo&);
  Status EnsureCurrent(const SurfaceKey&, SurfaceInfo&);
  Status Swap(uint64_t);
  Status Unbind();
  void DestroyUnbound(ResetReason);
  Status SetActive(bool);
  void Release();
  void QuerySize();
  State state = State::WAIT_WINDOW;
  bool active = true, current = false;
  int width = 0, height = 0;
  uint64_t surfaceEpoch = 0, contextEpoch = 0, surfaceCreateCount = 0,
           surfaceDestroyCount = 0;
  uint64_t contextCreateCount = 0, contextDestroyCount = 0,
           contextLossCount = 0, recoveryAttemptCount = 0,
           staleLifecycleCount = 0;
  EGLint error = EGL_SUCCESS;
  const char* errorStage = "NONE";
  double initMs = 0, currentMs = 0, unbindMs = 0, swapMs = 0;

 private:
  Status Failure(const char*, Status);
  void Hint(OH_NativeXComponent*, void*);
  EGLDisplay display = EGL_NO_DISPLAY;
  EGLConfig config = nullptr;
  EGLSurface surface = EGL_NO_SURFACE;
  EGLContext context = EGL_NO_CONTEXT;
  std::thread::id currentOwner{};
  void* window = nullptr;
  uint64_t attemptedEpoch = UINT64_MAX;
  bool recovered = false, badWindow = false;
};
}  // namespace p013
