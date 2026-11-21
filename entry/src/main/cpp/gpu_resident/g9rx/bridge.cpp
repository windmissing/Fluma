#include "../napi_words_contract.h"
#include "../program_cache/artifact_store.h"
#include "cold.h"
#include "controller.h"
#include "flat_json.h"
#include "scene.h"
#include <ace/xcomponent/native_interface_xcomponent.h>
#include <condition_variable>
#ifdef GRS_G9_BOOT_SOURCE
#include <fstream>
#include <unistd.h>
#endif
#include <hilog/log.h>
#include <napi/native_api.h>
#include <native_window/external_window.h>
#include <sstream>
#include <thread>
#include <window_manager/oh_display_manager.h>
namespace grs::g9rx {
extern "C" const uint32_t GrsG9RxAbi[16];
namespace {
thread_local bool uiIngressChannel = false;
#ifdef GRS_G9_BOOT_SOURCE
std::string ProcessStartTick() {
  std::ifstream file("/proc/self/stat");
  std::string line, value;
  std::getline(file, line);
  const auto end = line.rfind(')');
  if (end == std::string::npos)
    return {};
  std::istringstream fields(line.substr(end + 1));
  for (int field = 3; field <= 22; ++field)
    if (!(fields >> value))
      return {};
  return value.find_first_not_of("0123456789") == std::string::npos ? value : "";
}
#endif
class App {
  struct ConfigureReceipt {
    uint64_t ticket = 0, enqueueNs = 0, ownerNs = 0, applyNs = 0, finishNs = 0;
    uint64_t run = 0, context = 0;
    uint64_t firstSwapNs = 0;
    const char *phase = "OFF";
    const char *failure = "";
    char mappedBuildId[129]{};
#ifdef GRS_G9_BOOT_SOURCE
    std::string firstSourceSwapToken;
#endif
    int32_t result = -1, shaderStatus = -1, linkStatus = -1;
    EGLint firstEgl = EGL_SUCCESS;
    GLenum firstGl = GL_NO_ERROR;
  };
  std::mutex mutex_;
  std::condition_variable wake_;
  std::thread owner_;
  bool quit_ = false;
  void *pendingWindow_ = nullptr;
  bool windowPending_ = false, configurePending_ = false;
  bool ownerSeen_ = false, applying_ = false, cancelApplying_ = false;
#ifdef GRS_G9_BOOT_SOURCE
  uint64_t qualifiedSourceGeometry_ = 0;
  bool sourceWake_ = false;
  uint64_t sourceWakeSequence_ = 0, sourceWakeReceipt_ = 0;
#endif
  Config pendingConfig_{};
  std::string pendingCacheDirectory_;
  ConfigureReceipt receipt_{};
  uint32_t width_ = 0, height_ = 0;
  uint64_t contextGeneration_ = 0, geometryGeneration_ = 1, frame_ = 0,
           runSerial_ = 0;
  EGLDisplay display_ = EGL_NO_DISPLAY;
  EGLContext context_ = EGL_NO_CONTEXT;
  EGLSurface surface_ = EGL_NO_SURFACE;
  void *window_ = nullptr;
  bool windowReady_ = false;
  const char *windowFailure_ = "";
  EGLint windowError_ = EGL_SUCCESS;
  Controller controller_;
  Scene scene_;
  Cold cold_;
  std::string stage_ = "OFF";
  std::atomic<int32_t> densityStatus_{-1}, rotationStatus_{-1};
  static bool Swap(void *user) {
    auto &app = *static_cast<App *>(user);
    return eglSwapBuffers(app.display_, app.surface_) == EGL_TRUE;
  }
  void Retire() {
    windowReady_ = false;
    controller_.Stop();
    if (context_ != EGL_NO_CONTEXT) {
      controller_.Release(false);
      scene_.Reset(false);
      cold_.Reset();
      eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
      if (eglDestroyContext(display_, context_))
        GrsFullGpuContextRetired(reinterpret_cast<uint64_t>(context_));
    }
    if (surface_ != EGL_NO_SURFACE)
      eglDestroySurface(display_, surface_);
    if (display_ != EGL_NO_DISPLAY)
      eglTerminate(display_);
    context_ = EGL_NO_CONTEXT;
    surface_ = EGL_NO_SURFACE;
    display_ = EGL_NO_DISPLAY;
    if (window_)
      OH_NativeWindow_NativeObjectUnreference(window_);
    window_ = nullptr;
  }
  bool Window(void *window) {
    Retire();
    windowFailure_ = "";
    windowError_ = EGL_SUCCESS;
    window_ = window;
    if (!window)
      return false;
    const auto fail = [this](const char *call) {
      windowFailure_ = call;
      windowError_ = eglGetError();
      Retire();
      return false;
    };
    display_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    EGLint major = 0, minor = 0, count = 0;
    EGLConfig config = nullptr;
    const EGLint attributes[] = {EGL_SURFACE_TYPE,
                                 EGL_WINDOW_BIT,
                                 EGL_RENDERABLE_TYPE,
                                 EGL_OPENGL_ES3_BIT,
                                 EGL_RED_SIZE,
                                 8,
                                 EGL_GREEN_SIZE,
                                 8,
                                 EGL_BLUE_SIZE,
                                 8,
                                 EGL_ALPHA_SIZE,
                                 8,
                                 EGL_NONE};
    const EGLint ctx[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    if (display_ == EGL_NO_DISPLAY)
      return fail("eglGetDisplay");
    if (!eglInitialize(display_, &major, &minor))
      return fail("eglInitialize");
    if (!eglChooseConfig(display_, attributes, &config, 1, &count))
      return fail("eglChooseConfig");
    if (count != 1)
      return fail("EGL_CONFIG_COUNT");
    context_ = eglCreateContext(display_, config, EGL_NO_CONTEXT, ctx);
    if (context_ == EGL_NO_CONTEXT)
      return fail("eglCreateContext");
    surface_ = eglCreateWindowSurface(
        display_, config, reinterpret_cast<EGLNativeWindowType>(window),
        nullptr);
    if (surface_ == EGL_NO_SURFACE)
      return fail("eglCreateWindowSurface");
    if (!eglMakeCurrent(display_, surface_, surface_, context_))
      return fail("eglMakeCurrent");
    if (contextGeneration_ == UINT64_MAX)
      return fail("CONTEXT_IDENTITY_EXHAUSTED");
    ++contextGeneration_;
    return windowReady_ = true;
  }
  // Called with the App mutex held. Keep the original stage and raw code
  // together so a later scene/status update cannot erase the first failure.
  void LogConfigureStage(const char *phase, uint64_t ns) {
    OH_LOG_Print(LOG_APP, LOG_INFO, 0xE719, "G9RxConfigure",
                 "ticket=%{public}llu ns=%{public}llu phase=%{public}s run=%{public}llu context=%{public}llu result=%{public}d failure=%{public}s egl=%{public}d gl=%{public}u shader=%{public}d link=%{public}d mappedBuildId=%{public}s sourceSHA=%{public}s",
                 static_cast<unsigned long long>(receipt_.ticket),
                 static_cast<unsigned long long>(ns),
                 phase, static_cast<unsigned long long>(receipt_.run),
                 static_cast<unsigned long long>(receipt_.context),
                 receipt_.result, receipt_.failure, receipt_.firstEgl,
                 receipt_.firstGl, receipt_.shaderStatus, receipt_.linkStatus,
                 receipt_.mappedBuildId, PipelineSourceHash());
  }
  void ConfigureStage(const char *phase) {
    receipt_.phase = phase;
    LogConfigureStage(phase, CheckedClockRead().ns);
  }
  static void ObserveProgram(void *user, SubmitStage stage, uint64_t,
                             uint32_t code, uint32_t, uint64_t) {
    auto &app = *static_cast<App *>(user);
    std::lock_guard<std::mutex> lock(app.mutex_);
    switch (stage) {
    case SubmitStage::ShaderEnter:
      app.ConfigureStage("SHADER_COMPILE_ENTER");
      break;
    case SubmitStage::ShaderExit:
      app.receipt_.shaderStatus = code;
      app.ConfigureStage("SHADER_COMPILE_EXIT");
      break;
    case SubmitStage::ProgramEnter:
      app.ConfigureStage("PROGRAM_LINK_ENTER");
      break;
    case SubmitStage::ProgramExit:
      app.receipt_.linkStatus = code;
      app.ConfigureStage("PROGRAM_LINK_EXIT");
      break;
    default:
      break;
    }
  }
  void SetStage(const std::string &value) {
    std::lock_guard<std::mutex> lock(mutex_);
    stage_ = value;
  }
  void Loop();

public:
  App() {
    owner_ = std::thread([this] { Loop(); });
  }
  ~App() {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      quit_ = true;
      wake_.notify_all();
    }
    owner_.join();
  }
  Controller &Core() { return controller_; }
  Cold &Archive() { return cold_; }
  void SetWindow(void *window) {
    controller_.Stop();
    std::lock_guard<std::mutex> lock(mutex_);
    if (pendingWindow_)
      OH_NativeWindow_NativeObjectUnreference(pendingWindow_);
    pendingWindow_ = window;
    windowPending_ = true;
#ifdef GRS_G9_BOOT_SOURCE
    qualifiedSourceGeometry_ = 0;
#endif
    wake_.notify_all();
  }
  void Resize(uint32_t w, uint32_t h) {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (w == width_ && h == height_)
        return;
    }
    controller_.InvalidateGeometry();
    std::lock_guard<std::mutex> lock(mutex_);
    width_ = w;
    height_ = h;
#ifdef GRS_G9_BOOT_SOURCE
    qualifiedSourceGeometry_ = 0;
#endif
    if (geometryGeneration_ != UINT64_MAX)
      ++geometryGeneration_;
    wake_.notify_all();
  }
  std::string Configure(const Config &config, std::string cacheDirectory) {
    if (cold_.HasLive(CheckedClockRead()))
      return "{\"state\":\"COLD_SWITCH_WOULD_BLOCK\"}";
    std::lock_guard<std::mutex> lock(mutex_);
    if (configurePending_ || applying_)
      return "{\"state\":\"BUSY\"}";
    if (receipt_.ticket == UINT64_MAX)
      return "{\"state\":\"IDENTITY_EXHAUSTED\"}";
    controller_.Stop();
    const auto ticket = receipt_.ticket + 1;
#ifdef GRS_G9_BOOT_SOURCE
    qualifiedSourceGeometry_ = 0;
#endif
    receipt_ = {};
    receipt_.ticket = ticket;
    receipt_.enqueueNs = CheckedClockRead().ns;
    BuildIdForAddress(reinterpret_cast<const void *>(&ObserveProgram),
                      receipt_.mappedBuildId);
    pendingConfig_ = config;
    pendingCacheDirectory_ = std::move(cacheDirectory);
    configurePending_ = true;
    ownerSeen_ = false;
    ConfigureStage("ENQUEUED");
    wake_.notify_all();
    return "{\"state\":\"CONFIG_QUEUED\",\"ticket\":\"" +
           std::to_string(ticket) + "\"}";
  }
  bool CancelConfigure(uint64_t ticket) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!ticket || ticket != receipt_.ticket || (!configurePending_ && !applying_))
      return false;
    if (applying_) {
      cancelApplying_ = true;
    } else {
      configurePending_ = false;
      pendingCacheDirectory_.clear();
      receipt_.finishNs = CheckedClockRead().ns;
      ConfigureStage("CANCELLED");
    }
    return true;
  }
  std::string Status() {
    std::lock_guard<std::mutex> lock(mutex_);
    return StatusLocked();
  }
  std::string StatusLocked() {
    auto text = controller_.Status();
    text.pop_back();
    text += ",\"initialization\":\"" + stage_ + "\"";
    text += ",\"sceneFirstGl\":" + std::to_string(scene_.FirstGl()) +
            ",\"sceneFirstEgl\":" + std::to_string(scene_.FirstEgl()) +
            ",\"densityApiStatus\":" + std::to_string(densityStatus_.load()) +
            ",\"rotationApiStatus\":" + std::to_string(rotationStatus_.load());
    std::ostringstream configure;
    configure << ",\"configure\":{\"ticket\":\"" << receipt_.ticket
              << "\",\"phase\":\"" << receipt_.phase << "\",\"failure\":\""
              << receipt_.failure << "\",\"mappedBuildId\":\""
              << receipt_.mappedBuildId << "\",\"enqueueNs\":\"" << receipt_.enqueueNs
              << "\",\"ownerNs\":\"" << receipt_.ownerNs << "\",\"applyNs\":\""
              << receipt_.applyNs << "\",\"finishNs\":\"" << receipt_.finishNs
              << "\",\"firstSwapNs\":\"" << receipt_.firstSwapNs
              << "\",\"run\":\"" << receipt_.run << "\",\"context\":\""
              << receipt_.context << "\",\"result\":" << receipt_.result
              << ",\"firstEgl\":" << receipt_.firstEgl
              << ",\"firstGl\":" << receipt_.firstGl
              << ",\"shaderStatus\":" << receipt_.shaderStatus
              << ",\"linkStatus\":" << receipt_.linkStatus;
#ifdef GRS_G9_BOOT_SOURCE
    configure << ",\"firstSourceSwapToken\":\"" << receipt_.firstSourceSwapToken << '"';
#endif
    configure << '}';
    text += configure.str();
#ifdef GRS_G9_BOOT_SOURCE
    static const auto start = ProcessStartTick();
    text += ",\"pid\":" + std::to_string(getpid()) + ",\"processStartTick\":\"" + start + "\"";
    text += ",\"sourceGeometryReady\":";
    text += (!windowPending_ && qualifiedSourceGeometry_ == geometryGeneration_ &&
             std::strcmp(receipt_.phase, "APPLIED") == 0) ? "true" : "false";
#endif
    text += ",\"abi\":[";
    for (size_t i = 0; i < 16; ++i) {
      if (i)
        text += ',';
      text += std::to_string(GrsG9RxAbi[i]);
    }
    return text + "]}";
  }
#ifdef GRS_G9_BOOT_SOURCE
  void WakeSource(uint64_t sequence, uint64_t receipt) {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      sourceWake_ = true;
      sourceWakeSequence_ = sequence;
      sourceWakeReceipt_ = receipt;
    }
    wake_.notify_one();
  }
  std::string SourceDiagnostics() {
    std::lock_guard<std::mutex> lock(mutex_);
    return "{\"status\":" + StatusLocked() + ",\"trace\":" + controller_.SourceDiagnostics() + "}";
  }
#endif
};
App &Product() {
  static App app;
  return app;
}
void App::Loop() {
  uint64_t lastPresent = 0;
  while (true) {
    void *window = nullptr;
    bool replace = false, configure = false;
    Config config{};
    std::string cacheDirectory;
    uint32_t width = 0, height = 0;
    uint64_t geometry = 0;
#ifdef GRS_G9_BOOT_SOURCE
    bool pulseWake = false;
    uint64_t wokeForSequence = 0, wokeForReceipt = 0;
#endif
    {
      std::unique_lock<std::mutex> lock(mutex_);
      wake_.wait_for(lock, std::chrono::milliseconds(2),
                     [this] {
                       return quit_ || windowPending_ ||
#ifdef GRS_G9_BOOT_SOURCE
                              sourceWake_ ||
#endif
                              (configurePending_ && !ownerSeen_);
                     });
      if (quit_)
        break;
#ifdef GRS_G9_BOOT_SOURCE
      pulseWake = sourceWake_;
      wokeForSequence = sourceWakeSequence_;
      wokeForReceipt = sourceWakeReceipt_;
      sourceWake_ = false;
#endif
      if (configurePending_ && !ownerSeen_) {
        ownerSeen_ = true;
        receipt_.ownerNs = CheckedClockRead().ns;
        ConfigureStage("OWNER_SEEN");
      }
      replace = windowPending_;
      if (replace) {
        window = pendingWindow_;
        pendingWindow_ = nullptr;
        windowPending_ = false;
        if (configurePending_ && window)
          ConfigureStage("EGL_ENTER");
      }
      width = width_;
      height = height_;
      geometry = geometryGeneration_;
    }
    if (replace) {
      const bool ready = Window(window);
      SetStage(ready ? "WINDOW_READY" :
                        (*windowFailure_ ? "WINDOW_FAILED" : "WAIT_NATIVE_WINDOW"));
    }
    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (configurePending_) {
        if (!windowPending_) {
          if (*windowFailure_) {
            receipt_.failure = windowFailure_;
            receipt_.firstEgl = windowError_;
            receipt_.finishNs = CheckedClockRead().ns;
            configurePending_ = false;
            ConfigureStage("FAILED");
          } else if (!windowReady_) {
            if (std::strcmp(receipt_.phase, "WAIT_NATIVE_WINDOW") != 0)
              ConfigureStage("WAIT_NATIVE_WINDOW");
          } else if (!controller_.ReadyToReconfigure()) {
            if (std::strcmp(receipt_.phase, "WAIT_RETIREMENT") != 0)
              ConfigureStage("WAIT_RETIREMENT");
          } else if (runSerial_ == UINT64_MAX) {
            receipt_.failure = "RUN_IDENTITY_EXHAUSTED";
            receipt_.finishNs = CheckedClockRead().ns;
            configurePending_ = false;
            ConfigureStage("FAILED");
          } else {
            config = pendingConfig_;
            cacheDirectory = std::move(pendingCacheDirectory_);
            configurePending_ = false;
            configure = applying_ = true;
            receipt_.run = ++runSerial_;
            receipt_.context = contextGeneration_;
          }
        }
      }
    }
    if (configure) {
      if (!cacheDirectory.empty()) {
        {
          std::lock_guard<std::mutex> lock(mutex_);
          ConfigureStage("CACHE_PRELOAD_ENTER");
        }
        bool loaded = false;
        try {
          loaded = programcache::Store().Configure(cacheDirectory);
        } catch (const std::exception &) {
          // Persistent cache is optional. Run the existing source path.
        }
        std::lock_guard<std::mutex> lock(mutex_);
        ConfigureStage(loaded ? "CACHE_PRELOAD_READY" : "CACHE_PRELOAD_FAILED");
      }
      {
        std::lock_guard<std::mutex> lock(mutex_);
        if (cancelApplying_ || windowPending_) {
          applying_ = cancelApplying_ = false;
          receipt_.finishNs = CheckedClockRead().ns;
          ConfigureStage("CANCELLED");
          continue;
        }
        receipt_.applyNs = CheckedClockRead().ns;
        ConfigureStage("APPLY_ENTER");
      }
      controller_.Release(true);
      ControlIdentity id{
          runSerial_,         contextGeneration_, contextGeneration_,
          runSerial_,         runSerial_,         1,
          contextGeneration_, runSerial_,         contextGeneration_};
      GLenum firstGl = GL_NO_ERROR;
      EGLint firstEgl = EGL_SUCCESS;
      auto result = controller_.ConfigureResearch(
          config, id, reinterpret_cast<const void *>(&Product),
          {this, ObserveProgram}, &firstGl, &firstEgl);
      {
        std::lock_guard<std::mutex> lock(mutex_);
        receipt_.result = int32_t(result);
        receipt_.firstGl = firstGl;
        receipt_.firstEgl = firstEgl;
        ConfigureStage("CONFIGURE_RESEARCH_EXIT");
      }
      const bool boot = result == Result::BOOT_AUDIT_OK;
      const char *failedAt = result == Result::OK || boot ? "" : "CONFIGURE_RESEARCH";
      if (result == Result::OK) {
        SceneCapabilityV2 cap{};
        result = scene_.PrepareAttributeScene(controller_.Caps(), &cap);
        if (result != Result::OK) {
          failedAt = cap.failure;
        }
      }
      std::lock_guard<std::mutex> lock(mutex_);
      const bool cancelled = cancelApplying_ || windowPending_;
      if ((result != Result::OK && !boot) || cancelled)
        controller_.Stop();
      applying_ = cancelApplying_ = false;
      receipt_.result = int32_t(result);
      receipt_.firstGl = firstGl;
      receipt_.firstEgl = firstEgl;
      receipt_.failure = receipt_.shaderStatus == 0 ? "SHADER_COMPILE" :
                         receipt_.linkStatus == 0 ? "PROGRAM_LINK" : failedAt;
      receipt_.finishNs = CheckedClockRead().ns;
      if (result != Result::OK && !boot) {
        stage_ = result == Result::SOURCE_IDENTITY_MISMATCH
                     ? "SOURCE_IDENTITY_MISMATCH"
                     : "CONFIGURE_FAILED_" + std::to_string(uint32_t(result));
        ConfigureStage("FAILED");
      } else if (cancelled) {
        stage_ = "OFF";
        ConfigureStage("CANCELLED");
      } else if (boot) {
        stage_ = "BOOT_AUDIT_OK";
        ConfigureStage("BOOT_AUDIT_OK");
      } else {
        stage_ = "CONFIGURED";
        ConfigureStage("APPLIED");
#ifdef GRS_G9_BOOT_SOURCE
        controller_.SourceOwnerEdge("APPLIED", receipt_.finishNs, receipt_.ticket, receipt_.run);
#endif
      }
    }
    if (!windowReady_)
      continue;
    auto now = CheckedClockRead();
    if (!now.ok) {
      controller_.Stop();
      continue;
    }
    controller_.PollSceneFence();
    cold_.Build(controller_, now);
    const auto extent = [&] {
      EGLint actualWidth = 0, actualHeight = 0;
      return eglQuerySurface(display_, surface_, EGL_WIDTH, &actualWidth) &&
             eglQuerySurface(display_, surface_, EGL_HEIGHT, &actualHeight) &&
             actualWidth == int(width) && actualHeight == int(height);
    };
#ifdef GRS_G9_BOOT_SOURCE
    const bool extentKnown = width && height && extent();
    bool claimWindow = false;
    bool newExtent = false;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      claimWindow = extentKnown && !windowPending_ && geometry == geometryGeneration_ &&
                    std::strcmp(receipt_.phase, "APPLIED") == 0;
      newExtent = claimWindow && qualifiedSourceGeometry_ != geometry;
      qualifiedSourceGeometry_ = claimWindow ? geometry : 0;
    }
    if (newExtent)
      controller_.SourceOwnerEdge("EXTENT_QUALIFIED", CheckedClockRead().ns, width, height);
    if (pulseWake)
      controller_.SourceOwnerEdge("OWNER_WAKE", CheckedClockRead().ns, wokeForSequence, wokeForReceipt);
#endif
    if (controller_.RootLive()) {
      if (controller_.NeedsRetirement())
        controller_.Drain();
      else
        controller_.PumpBounded(now);
    } else {
      controller_.Drain();
#ifdef GRS_G9_BOOT_SOURCE
      if (claimWindow)
#endif
      controller_.BeginRoot(CheckedClockRead(), ++frame_, geometry);
    }
#ifndef GRS_G9_BOOT_SOURCE
    const bool extentKnown = extent();
#endif
    if (!extentKnown && controller_.ReadyForScene()) {
      scene_.ObserveErrors();
      controller_.MarkSceneFault();
      SetStage("SURFACE_GEOMETRY_STALE");
    }
    if (extentKnown && controller_.ReadyForScene() && width && height &&
        now.ns >= lastPresent && now.ns - lastPresent >= 33333333) {
      std::string token;
      if (scene_.Draw(controller_, width, height, Swap, this, token)) {
        auto presented = CheckedClockRead();
        {
          std::lock_guard<std::mutex> lock(mutex_);
          if (presented.ok && !configurePending_ && !windowPending_ &&
              receipt_.run == runSerial_ && !receipt_.firstSwapNs &&
              std::strcmp(receipt_.phase, "APPLIED") == 0) {
            receipt_.firstSwapNs = presented.ns;
#ifdef GRS_G9_BOOT_SOURCE
            receipt_.firstSourceSwapToken = token;
            controller_.SourceOwnerEdge("SWAP", presented.ns,
                                        controller_.SceneGeneration(), receipt_.ticket);
#endif
            LogConfigureStage("FIRST_VALID_SWAP", presented.ns);
          }
        }
        float density = 0;
        NativeDisplayManager_Rotation rotation{};
        int32_t densityCode =
            OH_NativeDisplayManager_GetDefaultDisplayDensityPixels(&density);
        densityStatus_.store(densityCode);
        rotationStatus_.store(-1);
        int32_t rotationCode = -1;
        if (densityCode == 0) {
          rotationCode =
              OH_NativeDisplayManager_GetDefaultDisplayRotation(&rotation);
          rotationStatus_.store(rotationCode);
        }
        if (densityCode == 0 && rotationCode == 0 &&
            rotation == DISPLAY_MANAGER_ROTATION_0 &&
            controller_.PublishMapping(controller_.SceneGeneration(), width,
                                       height, double(density), presented))
          SetStage("CONFIGURED");
        else {
          controller_.InvalidatePresentedMapping();
          SetStage("INPUT_MAPPING_API_UNQUALIFIED");
        }
        lastPresent = now.ns;
      } else {
        controller_.MarkSceneFault();
        SetStage("PRESENT_STALE_OR_FAULT");
      }
    }
#ifdef GRS_G9_BOOT_SOURCE
    bool presented = false;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      presented = receipt_.firstSwapNs != 0;
    }
    controller_.PrepareSourceCacheIfSettled(CheckedClockRead(), presented);
#endif
  }
  Retire();
}
napi_value Text(napi_env env, const std::string &text) {
  napi_value out = nullptr;
  napi_create_string_utf8(env, text.c_str(), text.size(), &out);
  return out;
}
bool String(napi_env env, napi_value value, std::string &out) {
  size_t n = 0;
  if (napi_get_value_string_utf8(env, value, nullptr, 0, &n) != napi_ok ||
      n > 4096)
    return false;
  out.resize(n + 1);
  if (napi_get_value_string_utf8(env, value, out.data(), out.size(), &n) !=
      napi_ok)
    return false;
  out.resize(n);
  return true;
}
bool Decimal(const std::string &s, uint64_t &value) {
  if (s.empty() || s.size() > 20)
    return false;
  value = 0;
  for (char c : s) {
    if (c < '0' || c > '9' || value > (UINT64_MAX - (c - '0')) / 10)
      return false;
    value = value * 10 + c - '0';
  }
  return true;
}
ControlIdentity Identity(const uint32_t *w) {
  ControlIdentity id{};
  uint64_t *fields[] = {&id.run,        &id.owner,       &id.context,
                        &id.inputEpoch, &id.pageSession, &id.permission,
                        &id.window,     &id.config,      &id.surface};
  for (size_t i = 0; i < 9; ++i)
    *fields[i] = Pair(w, 2 * i);
  return id;
}
std::string IngressUnqualified(const char *state, CheckedTime receipt) {
  return "{\"revision\":\"G9_RX_REPLY_V1\",\"profile\":\"GNR1\",\"domain\":"
         "\"NATIVE_RECEIPT_SPATIAL_CONTROL\",\"boot\":\"UNKNOWN\",\"scope\":"
         "\"UNKNOWN\",\"attempt\":\"UNKNOWN\",\"sampleAge\":\"UNKNOWN\","
         "\"receiptValid\":" +
         std::string(receipt.ok ? "true" : "false") + ",\"receiptNs\":\"" +
         std::to_string(receipt.ns) +
         "\",\"rawClockErrno\":" + std::to_string(receipt.rawErrno) +
         ",\"state\":\"" + state + "\"}";
}
std::string DecodeReceipt(std::string reply,
                          const words::ReadWordsResult<64> &decoded) {
  const auto &o = decoded.observed;
  if (reply.empty() || reply.back() != '}')
    return reply;
  reply.pop_back();
  std::ostringstream out;
  out << reply << ",\"decode\":{\"rawReturnedLength\":\"" << o.rawReturnedLength
      << "\",\"lengthUnit\":\"" << provider629::Name(o.lengthUnit)
      << "\",\"normalizedElementCount\":\"" << o.normalizedElementCount
      << "\",\"normalizationObserved\":"
      << (o.normalizationObserved ? "true" : "false") << ",\"backingBytes\":\""
      << o.backingBytes << "\",\"byteOffset\":\"" << o.byteOffset
      << "\",\"typedInfoStatus\":" << o.typedarrayInfoStatus
      << ",\"providerProfile\":\"" << o.providerProfileId.data()
      << "\",\"providerEvidenceSHA\":\"" << o.providerEvidenceSHA.data()
      << "\",\"copyComplete\":" << (o.copyComplete ? "true" : "false")
      << ",\"firstFailedStage\":\""
      << (decoded.ok ? "NONE" : words::StageName(decoded.firstFailedStage))
      << "\"}}";
  auto value = out.str();
  return value.size() < 2048 ? value
                             : "{\"revision\":\"G9_RX_REPLY_V1\",\"profile\":"
                               "\"GNR1\",\"domain\":\"NATIVE_RECEIPT_SPATIAL_"
                               "CONTROL\",\"state\":\"REPLY_LIMIT\"}";
}
napi_value Touch(napi_env env, napi_callback_info info) {
  const auto receipt =
      CheckedClockRead(); // first operation, before singleton, locks or NAPI
#ifdef GRS_G9_BOOT_SOURCE
  return Text(env, IngressUnqualified("SOURCE_ONLY", receipt));
#else
  auto &protocol = Product().Core().Protocol();
  if (!uiIngressChannel ||
      protocol.sync.ingressEnv.load() != reinterpret_cast<uintptr_t>(env))
    return Text(env, IngressUnqualified("SINGLE_UI_CHANNEL_REQUIRED", receipt));
  if (protocol.sync.ingress.test_and_set()) {
    protocol.sync.CancelStroke();
    return Text(env, IngressUnqualified("INGRESS_BUSY", receipt));
  }
  struct Exit {
    std::atomic_flag &flag;
    ~Exit() { flag.clear(); }
  } exit{protocol.sync.ingress};
  if (!protocol.SnapshotMappingBeforeParse(receipt))
    return Text(env, IngressUnqualified(!receipt.ok ? "CLOCK_READ_FAILED"
                                        : !RoundingQualified()
                                            ? "ROUNDING_MODE_UNSUPPORTED"
                                            : "WAIT_PRESENTED_MAPPING",
                                        receipt));
  size_t argc = 2;
  napi_value argv[2]{};
  if (napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr) != napi_ok ||
      argc != 1)
    return Text(env, IngressUnqualified("BAD_ARGUMENTS", receipt));
  auto raw = words::ReadTypedValue<64>(env, argv[0]);
  if (!raw.ok) {
    protocol.sync.CancelStroke();
    return Text(env,
                DecodeReceipt(IngressUnqualified(
                                  "RAW64_PROVIDER_OR_LAYOUT_REJECTED", receipt),
                              raw));
  }
  return Text(
      env,
      DecodeReceipt(protocol.ValidateStroke(raw.words.data(), receipt), raw));
#endif
}
napi_value Api(napi_env env, napi_callback_info info) {
  auto receipt = CheckedClockRead();
  size_t argc = 4;
  napi_value argv[4]{};
  void *tag = nullptr;
  if (napi_get_cb_info(env, info, &argc, argv, nullptr, &tag) != napi_ok)
    return Text(env, "{\"state\":\"CB_INFO_FAILED\"}");
  std::string api = tag ? static_cast<const char *>(tag) : "";
  auto &app = Product();
  auto &core = app.Core();
  if (api == "g9RxStatus")
    return Text(env, app.Status());
  if (api == "g9RxSourceDiagnostics") {
#ifdef GRS_G9_BOOT_SOURCE
    return Text(env, app.SourceDiagnostics());
#else
    return Text(env, "{\"state\":\"UNSUPPORTED\"}");
#endif
  }
  if (api == "g9RxCancelConfigure") {
    std::string expected;
    uint64_t ticket = 0;
    return Text(env, argc == 1 && String(env, argv[0], expected) &&
                         Decimal(expected, ticket) && app.CancelConfigure(ticket)
                         ? "{\"state\":\"CANCEL_REQUESTED\"}"
                         : "{\"state\":\"STALE_CONFIGURE_TICKET\"}");
  }
  if (api == "g9RxSession")
    return Text(env, core.Session());
  if (api == "g9RxLoadedIdentity")
    return Text(env, core.LoadedReceipt());
  if (api == "g9RxResearchConfig") {
    uint32_t k = 0;
    if (argc != 1 || napi_get_value_uint32(env, argv[0], &k) != napi_ok ||
        (k != 192 && k != 512))
      return Text(env, "{\"state\":\"BAD_K\"}");
    const auto &words = k == 192 ? kConfig192 : kConfig512;
    std::string out = "{\"words\":[";
    for (size_t i = 0; i < 64; ++i) {
      if (i)
        out += ',';
      out += std::to_string(words[i]);
    }
    return Text(env, out + "]}");
  }
  if (api == "g9RxConfigure") {
    if (argc != 2 && argc != 3)
      return Text(env, "{\"state\":\"BAD_ARGUMENTS\"}");
    auto words = grs::words::ReadTypedValue<64>(env, argv[0]);
    std::string mode;
    Config config;
    if (!words.ok || !String(env, argv[1], mode) ||
        (mode != "RESEARCH_ONLY" && mode != "RESEARCH_ONLY_FULL") ||
        !DecodeConfig(words.words, config))
      return Text(env, "{\"state\":\"CONFIG_REJECTED\"}");
    std::string cacheDirectory;
    if (argc == 3 && (!String(env, argv[2], cacheDirectory) ||
                      cacheDirectory.empty() || cacheDirectory.front() != '/'))
      return Text(env, "{\"state\":\"BAD_CACHE_DIRECTORY\"}");
    config.fullTrialTrace = mode == "RESEARCH_ONLY_FULL";
    return Text(env, app.Configure(config, std::move(cacheDirectory)));
  }
  if (api == "g9RxPulse") {
    if (argc != 1)
      return Text(env, "{\"state\":\"BAD_ARGUMENTS\"}");
    auto w = words::ReadTypedValue<22>(env, argv[0]);
    if (!w.ok)
      return Text(env, "{\"state\":\"PULSE_LAYOUT_REJECTED\"}");
    auto result =
        core.Pulse({Identity(w.words.data()), Pair(w.words.data(), 18),
                    Pair(w.words.data(), 20)},
                   receipt);
#ifdef GRS_G9_BOOT_SOURCE
    if (result == ControlResult::OK)
      app.WakeSource(Pair(w.words.data(), 18), receipt.ns);
#endif
    return Text(env, "{\"code\":" + std::to_string(uint32_t(result)) + "}");
  }
  if (api == "g9RxTarget") {
    if (argc != 1)
      return Text(env, "{\"state\":\"BAD_ARGUMENTS\"}");
    auto w = words::ReadTypedValue<19>(env, argv[0]);
    if (!w.ok)
      return Text(env, "{\"state\":\"TARGET_LAYOUT_REJECTED\"}");
    return Text(env, "{\"code\":" +
                         std::to_string(uint32_t(
                             core.Target(Identity(w.words.data()),
                                         int32_t(w.words[18]), receipt))) +
                         "}");
  }
  if (api == "g9RxSetActive" || api == "g9RxStop") {
    std::string expected;
    bool value = false;
    if (api == "g9RxStop") {
      if (argc != 1 || !String(env, argv[0], expected))
        return Text(env, "{\"state\":\"SCOPE_REQUIRED\"}");
    } else if (argc != 2 ||
               napi_get_value_bool(env, argv[0], &value) != napi_ok ||
               !String(env, argv[1], expected))
      return Text(env, "{\"state\":\"SCOPE_REQUIRED\"}");
    if (expected != core.Session())
      return Text(env, "{\"state\":\"STALE_SCOPE\"}");
    if (!value) {
      core.Stop();
      return Text(env, "{\"state\":\"STOPPED\"}");
    }
    auto &protocol = core.Protocol();
    ControlIdentity id;
    {
      std::lock_guard<std::mutex> lock(protocol.sync.protocol);
      id = Identity(protocol.Capability().data() + 10);
    }
    return Text(env, core.SetActive(true, id, receipt)
                         ? "{\"state\":\"ACTIVE\"}"
                         : "{\"state\":\"ACTIVE_REJECTED\"}");
  }
  if (api == "g9RxSource") {
    std::string text;
    if (argc != 1 || !String(env, argv[0], text))
      return Text(env, "{\"state\":\"SOURCE_ENVELOPE_REQUIRED\"}");
    Json json(text);
    uint64_t intent = 0, source = 0;
    if (!json.Valid() || !json.Number("intent", intent) ||
        !json.Number("sourceId", source) || !source || source > 4096)
      return Text(env, "{\"state\":\"SOURCE_KEY_REJECTED\"}");
    Input128 body{};
    const char *names[] = {"mass", "x", "y", "vx", "vy"};
    for (size_t i = 0; i < 5; ++i) {
      auto value = json.Text(names[i]);
      char *end = nullptr;
      errno = 0;
      double number = std::strtod(value.c_str(), &end);
      if (value.empty() || errno || end != value.data() + value.size() ||
          !std::isfinite(number))
        return Text(env, "{\"state\":\"SOURCE_VALUE_REJECTED\"}");
      RealPut(body.data(), 64 + 2 * i, number);
    }
    body[74] = source;
    body[75] = 1;
    body[76] = 63701;
    return Text(env, "{\"code\":" +
                         std::to_string(uint32_t(
                             core.OfferSource(body, intent, receipt))) +
                         "}");
  }
  if (api == "g9RxColdBegin") {
    std::string replace, mode;
    uint64_t id = 0;
    if (argc != 2 || !String(env, argv[0], replace) || !Decimal(replace, id) ||
        !String(env, argv[1], mode) || (mode != "ROOT" && mode != "FULL"))
      return Text(env, "{\"state\":\"BAD_COLD_REQUEST\"}");
    return Text(env, app.Archive().Begin(core, id, mode == "FULL", receipt));
  }
  if (api == "g9RxColdState" || api == "g9RxColdEnd" || api == "g9RxColdPage") {
    std::string text;
    uint64_t id = 0;
    uint32_t page = 0;
    if (argc < (api == "g9RxColdPage" ? 2 : 1) || !String(env, argv[0], text) ||
        !Decimal(text, id))
      return Text(env, "{\"state\":\"BAD_COLD_ID\"}");
    if (api == "g9RxColdState")
      return Text(env, app.Archive().State(id, receipt));
    if (api == "g9RxColdEnd")
      return Text(env, app.Archive().End(id, receipt));
    if (napi_get_value_uint32(env, argv[1], &page) != napi_ok)
      return Text(env, "{\"state\":\"BAD_PAGE\"}");
    return Text(env, app.Archive().Page(id, page, receipt));
  }
  return Text(env, "{\"state\":\"UNKNOWN_API\"}");
}
void Created(OH_NativeXComponent *component, void *window) {
  if (!window || OH_NativeWindow_NativeObjectReference(window))
    return;
  Product().SetWindow(window);
  uint64_t w = 0, h = 0;
  if (OH_NativeXComponent_GetXComponentSize(component, window, &w, &h) == 0 &&
      w <= UINT32_MAX && h <= UINT32_MAX)
    Product().Resize(w, h);
}
void Changed(OH_NativeXComponent *component, void *window) {
  uint64_t w = 0, h = 0;
  if (OH_NativeXComponent_GetXComponentSize(component, window, &w, &h) == 0 &&
      w <= UINT32_MAX && h <= UINT32_MAX)
    Product().Resize(w, h);
}
void Destroyed(OH_NativeXComponent *, void *) { Product().SetWindow(nullptr); }
OH_NativeXComponent_Callback callbacks{Created, Changed, Destroyed, nullptr};
napi_value Init(napi_env env, napi_value exports) {
  const char *names[] = {
      "g9RxResearchConfig", "g9RxConfigure", "g9RxSession",   "g9RxSetActive",
      "g9RxPulse",          "g9RxTarget",    "g9RxStatus",    "g9RxStop",
      "g9RxLoadedIdentity", "g9RxSource",    "g9RxSourceDiagnostics", "g9RxColdBegin", "g9RxColdState",
      "g9RxColdPage",       "g9RxColdEnd", "g9RxCancelConfigure"};
  for (auto name : names) {
    napi_property_descriptor p{
        name,    nullptr, Api,          nullptr,
        nullptr, nullptr, napi_default, const_cast<char *>(name)};
    napi_define_properties(env, exports, 1, &p);
  }
  napi_property_descriptor touch{
      "g9RxTouchIngress", nullptr, Touch, nullptr, nullptr, nullptr,
      napi_default,       nullptr};
  napi_define_properties(env, exports, 1, &touch);
  napi_value value = nullptr;
  if (napi_get_named_property(env, exports, OH_NATIVE_XCOMPONENT_OBJ, &value) ==
      napi_ok) {
    OH_NativeXComponent *component = nullptr;
    if (napi_unwrap(env, value, reinterpret_cast<void **>(&component)) ==
            napi_ok &&
        component) {
      uiIngressChannel = true;
      Product().Core().Protocol().sync.ingressEnv.store(
          reinterpret_cast<uintptr_t>(env));
      OH_NativeXComponent_RegisterCallback(component, &callbacks);
    }
  }
  return exports;
}
} // namespace
} // namespace grs::g9rx
static napi_module rxModule{1,          0,       nullptr, grs::g9rx::Init,
                            "grs_g9rx", nullptr, {0}};
extern "C" __attribute__((constructor)) void RegisterG9Rx() {
  napi_module_register(&rxModule);
}
