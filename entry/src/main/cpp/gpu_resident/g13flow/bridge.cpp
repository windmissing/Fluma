#include "../napi_words_contract.h"
#include "../program_cache/artifact_store.h"
#include "cold.h"
#include "controller.h"
#include "init_diagnostics.h"
#include "json.h"
#include "scene.h"
#include <ace/xcomponent/native_interface_xcomponent.h>
#include <condition_variable>
#include <mutex>
#include <napi/native_api.h>
#include <native_window/external_window.h>
#include <thread>
namespace grs::g13 {
namespace {
enum class Kind {
  Configure,
  Diagnostic,
  Offer,
  Session,
  Active,
  Pulse,
  Stop,
  View,
  ColdBegin,
  ColdPage,
  ColdEnd,
  Window,
  Resize,
  Destroy
};
struct Command {
  Kind kind;
  Config words{};
  std::array<uint32_t, 64> view{};
  std::string text, text2, api;
  uint64_t a = 0, b = 0, d = 0, attempt = 0, queueSerial = 0, received = 0;
  int replySlot = -1;
  void *window = nullptr;
  ConfigTicket ticket{};
  bool ticketPrepared = false;
};
bool IsConfigure(Kind kind) {
  return kind == Kind::Configure || kind == Kind::Diagnostic;
}
class App {
  std::mutex mutex_;
  std::condition_variable wake_;
  std::array<Command, 32> queue_{};
  size_t head_ = 0, count_ = 0;
  std::thread thread_;
  bool quit_ = false;
  bool configureCallInFlight_ = false;
  std::atomic<uint64_t> revoked_{0}, armed_{UINT64_MAX};
  uint64_t serial_ = 0, handled_ = 0;
  struct ReturnSlot {
    bool used = false, ready = false;
    std::string reply;
  };
  std::array<ReturnSlot, 32> returns_{};
  std::string status_ = "{}", reply_ = "{}", scope_ = "{}", sourceReply_ = "{}",
              coldReply_ = "{}";
  Controller controller_;
  Cold cold_;
  Scene scene_;
  Identity id_{};
  EGLDisplay display_ = EGL_NO_DISPLAY;
  EGLContext context_ = EGL_NO_CONTEXT;
  EGLSurface surface_ = EGL_NO_SURFACE;
  void *window_ = nullptr;
  bool configured_ = false, sceneReady_ = false, rebindPending_ = false,
       configurePending_ = false;
  Command deferredConfigure_{};
  uint64_t lastDraw_ = 0, firstSwapRun_ = 0, contextGeneration_ = 0, surfaceGeneration_ = 0,
           geometryGeneration_ = 0;
  bool initializing_ = false;
  ConfigTicket pendingTicket_{};
  void Publish(const std::string &reply = "") {
    std::string status = controller_.Status(), scope = ScopeJson(id_);
    status.pop_back();
    status += ",\"scene\":" + scene_.Receipt() + "}";
    std::lock_guard<std::mutex> lock(mutex_);
    status_ = status;
    scope_ = scope;
    if (!reply.empty())
      reply_ = reply;
  }
  bool Create() {
    if (!window_ || controller_.Gpu().RetirementPending() ||
        context_ != EGL_NO_CONTEXT || display_ != EGL_NO_DISPLAY)
      return false;
    InitObservation().State("CONTEXT_CREATE");
    auto api = [&](const char *phase, auto action) {
      InitStageEvent(phase, "ENTER");
      const auto result = action();
      InitObservation().Stage(phase, "EXIT", 0, -1, nullptr, -1, -1, -1, -1, 0,
                              result ? 1 : 0);
      return result;
    };
    if (!api("EGL_GET_DISPLAY", [&] {
          display_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);
          return display_ != EGL_NO_DISPLAY;
        }))
      return false;
    if (!api("EGL_INITIALIZE", [&] {
          return eglInitialize(display_, nullptr, nullptr) == EGL_TRUE;
        }))
      return false;
    EGLint attrs[] = {EGL_SURFACE_TYPE,
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
    EGLConfig cfg = nullptr;
    EGLint count = 0;
    if (!api("EGL_CHOOSE_CONFIG", [&] {
          return eglChooseConfig(display_, attrs, &cfg, 1, &count) ==
                     EGL_TRUE &&
                 count == 1;
        }))
      return false;
    EGLint ctx[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    if (!api("EGL_CREATE_CONTEXT", [&] {
          context_ = eglCreateContext(display_, cfg, EGL_NO_CONTEXT, ctx);
          return context_ != EGL_NO_CONTEXT;
        }))
      return false;
    InitObservation().Environment(window_, context_);
    if (!api("EGL_CREATE_SURFACE", [&] {
          surface_ = eglCreateWindowSurface(
              display_, cfg, reinterpret_cast<EGLNativeWindowType>(window_),
              nullptr);
          return surface_ != EGL_NO_SURFACE;
        }))
      return false;
    if (!api("EGL_MAKE_CURRENT", [&] {
          return eglMakeCurrent(display_, surface_, surface_, context_) ==
                 EGL_TRUE;
        }))
      return false;
    ++contextGeneration_;
    ++surfaceGeneration_;
    ++geometryGeneration_;
    return true;
  }
  bool Destroy() {
    InitObservation().State("RETIRING");
    InitStageEvent("RETIRING", "ENTER");
    initializing_ = false;
    cold_.CancelPending(10, "CONTEXT_RETIRED");
    auto retiredContext = context_;
    const bool current =
        context_ != EGL_NO_CONTEXT && eglGetCurrentContext() == context_;
    controller_.Release(current);
    scene_.Release(current);
    sceneReady_ = false;
    configured_ = false;
    configurePending_ = rebindPending_ = false;
    if (display_ != EGL_NO_DISPLAY) {
      if (!eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE,
                          EGL_NO_CONTEXT))
        return false;
      if (surface_ != EGL_NO_SURFACE) {
        if (!eglDestroySurface(display_, surface_))
          return false;
        surface_ = EGL_NO_SURFACE;
      }
      if (context_ != EGL_NO_CONTEXT) {
        if (!eglDestroyContext(display_, context_))
          return false;
        controller_.ContextRetired(retiredContext);
        context_ = EGL_NO_CONTEXT;
      }
      if (!eglTerminate(display_))
        return false;
    }
    display_ = EGL_NO_DISPLAY;
    if (window_)
      OH_NativeWindow_NativeObjectUnreference(window_);
    window_ = nullptr;
    InitObservation().Environment(nullptr, nullptr);
    InitStageEvent("RETIRING", "EXIT");
    InitObservation().State("UNCONFIGURED");
    return true;
  }
  std::string Execute(Command &cmd) {
    auto api = "g13Command";
    uint32_t error = 0;
    if (cmd.kind == Kind::Window) {
      if (window_ == cmd.window) {
        OH_NativeWindow_NativeObjectUnreference(cmd.window);
        return Result(api, "WINDOW_AVAILABLE");
      }
      if ((window_ || context_ != EGL_NO_CONTEXT) && !Destroy()) {
        OH_NativeWindow_NativeObjectUnreference(cmd.window);
        return Result(api, "CONTEXT_RETIRE_PENDING", 34);
      }
      window_ = cmd.window;
      return Result(api, "WINDOW_AVAILABLE");
    }
    if (cmd.kind == Kind::Resize) {
      if (window_ != cmd.window)
        return Result(api, "STALE_WINDOW", 2);
      controller_.Stop();
      id_ = controller_.Scope();
      ++geometryGeneration_;
      rebindPending_ = configured_;
      return Result(api, "VIEW_REBIND_PENDING");
    }
    if (cmd.kind == Kind::Destroy) {
      return Result(api,
                    Destroy() ? "WINDOW_RETIRED" : "CONTEXT_RETIRE_PENDING",
                    context_ == EGL_NO_CONTEXT ? 0 : 34);
    }

    if (cmd.kind == Kind::Session) {
      if (configured_) {
        controller_.Stop();
        id_ = controller_.Scope();
      }
      Json session(cmd.text);
      if (!session.Valid())
        return Result("g13BeginSession", "BAD_JSON", 5);
      auto files = session.Text("filesDir");
      if (!files.empty()) {
        bool loaded = false;
        try {
          loaded = programcache::Store().Configure(files);
        } catch (const std::exception &) {
          // The optional cache falling back is not a session failure.
        }
        OH_LOG_Print(LOG_APP, LOG_INFO, 0xE720, "G13Init",
                     "boot=%{public}llu cachePreload=%{public}s",
                     static_cast<unsigned long long>(kBoot),
                     loaded ? "READY" : "FAILED");
      }
      ++id_.session;
      ++id_.permission;
      ++id_.inputEpoch;
      id_.owner = 13;
      id_.control++;
      return std::string(
                 "{\"route\":13,\"schema\":1,\"api\":\"g13BeginSession\","
                 "\"state\":\"SESSION\",\"scope\":") +
             ScopeJson(id_) + "}";
    }
    if (cmd.kind == Kind::Configure || cmd.kind == Kind::Diagnostic) {
      if (cmd.ticket.cancelGeneration != revoked_.load()) {
        InitStageEvent("CONFIG_CANCELLED", "EXIT");
        return Result("g13Configure", "CONFIG_CANCELLED", 11, cmd.attempt);
      }
      if (!cmd.ticketPrepared) {
        cmd.ticket.proposed = id_;
        ++cmd.ticket.proposed.run;
        ++cmd.ticket.proposed.config;
        cmd.ticket.proposed.context = context_ == EGL_NO_CONTEXT
                                          ? contextGeneration_ + 1
                                          : contextGeneration_;
        cmd.ticket.proposed.surface = context_ == EGL_NO_CONTEXT
                                          ? surfaceGeneration_ + 1
                                          : surfaceGeneration_;
        cmd.ticket.proposed.geometry = context_ == EGL_NO_CONTEXT
                                           ? geometryGeneration_ + 1
                                           : geometryGeneration_;
        cmd.ticketPrepared = true;
      }
      if (initializing_) {
        deferredConfigure_ = cmd;
        configurePending_ = true;
        return Result("g13Configure", "CONFIG_DEFERRED", 34, cmd.attempt);
      }
      InitObservation().Ticket(cmd.ticket);
      InitObservation().Environment(window_, context_);
      InitObservation().State("CONFIG_QUEUED");
      if (context_ == EGL_NO_CONTEXT) {
        if (!window_) {
          deferredConfigure_ = cmd;
          configurePending_ = true;
          InitObservation().State("WAIT_WINDOW");
          InitStageEvent("WAIT_WINDOW", "ENTER");
          return Result("g13Configure", "WAIT_WINDOW", 0, cmd.attempt);
        }
        if (!Create()) {
          configurePending_ = false;
          InitObservation().State("CONFIG_FAILED");
          InitObservation().Failure(
              "CONTEXT_CREATE_FAILED: inspect ENTER/EXIT; raw EGL NOT_OBSERVED",
              cmd.attempt);
          return Result("g13Configure", "CONTEXT_CREATE_FAILED", 29,
                        cmd.attempt);
        }
      }
      if (cmd.ticket.proposed.context != contextGeneration_ ||
          cmd.ticket.proposed.surface != surfaceGeneration_) {
        InitObservation().Failure("STALE_CONFIG_ENVIRONMENT", cmd.attempt);
        return Result("g13Configure", "STALE_SCOPE", 2, cmd.attempt);
      }

      Json json(cmd.text);
      if (!json.Valid()) {
        configurePending_ = false;
        InitObservation().State("CONFIG_FAILED");
        InitObservation().Failure("BAD_SEED_RECEIPT", cmd.attempt);
        return Result("g13Configure", "BAD_SEED_RECEIPT", 5);
      }
      Seed seed;
      seed.path = json.Text("path");
      seed.sha = json.Text("sha256");
      seed.receipt = cmd.text;
      uint64_t mode = 0, forced = 0;
      if (!json.Number("diagnosticMode", mode, false) ||
          !json.Number("forcedJ", forced, false) || mode > 3 ||
          (forced != 0 && forced != 1 && forced != 2 && forced != 4)) {
        configurePending_ = false;
        InitObservation().State("CONFIG_FAILED");
        InitObservation().Failure("BAD_DIAGNOSTIC", cmd.attempt);
        return Result("g13Configure", "BAD_DIAGNOSTIC", 5);
      }
      seed.diagnostic =
          cmd.kind == Kind::Diagnostic ? uint32_t(mode ? mode : 1) : 0;
      seed.forcedJ = uint32_t(forced);
      auto fault = json.Text("fault");
      if (!fault.empty()) {
        if (!PipelineTestHooks() || seed.diagnostic != 3)
          return Result("g13DiagnosticBegin", "TEST_HOOKS_DISABLED", 4);
        seed.testHook = fault == "PRESSURE_DISABLED"          ? 1
                        : fault == "PRESSURE_SIGN_REVERSED"   ? 2
                        : fault == "RHS_INCOMPATIBLE"         ? 3
                        : fault == "GAUGE_OFFSET"             ? 4
                        : fault == "NORMAL_WALL"              ? 5
                        : fault == "PERIODIC_ALIAS_DUPLICATE" ? 6
                                                              : 7;
      }
      const Identity proposed = cmd.ticket.proposed;
      error = controller_.Configure(cmd.words, seed, proposed);
      if (!error) {
        pendingTicket_ = cmd.ticket;
        initializing_ = true;
        configured_ = false;
        configurePending_ = false;
        sceneReady_ = scene_.Prepare(proposed.context, cmd.words);
      } else if (error == 34) {
        deferredConfigure_ = cmd;
        configurePending_ = true;
      } else {
        configurePending_ = false;
        initializing_ = false;
        InitObservation().State("CONFIG_FAILED");
        InitObservation().Failure("CONFIGURE_ERROR_" + std::to_string(error),
                                  cmd.attempt);
      }
      return Result("g13Configure", error ? "REJECTED" : "CONFIG_PENDING",
                    error, cmd.attempt);
    }
    if (cmd.kind == Kind::Stop) {
      Identity supplied;
      Json j(cmd.text);
      if (!ReadScope(j, supplied) || !(supplied == id_))
        return Result("g13Stop", "STALE_SCOPE", 2);
      controller_.Stop();
      id_ = controller_.Scope();
      return Result("g13Stop", "REVOKED");
    }
    if (cmd.kind == Kind::Active) {
      if (!configured_ || !controller_.Ready())
        return Result("g13SetActive", "CONFIG_NOT_READY", 34);
      Identity supplied;
      Json j(cmd.text);
      if (!ReadScope(j, supplied) || !(supplied == id_))
        return Result("g13SetActive", "STALE_SCOPE", 2);
      if (cmd.a)
        armed_.store(revoked_.load());
      error = controller_.SetActive(cmd.a != 0, id_);
      if (!error)
        id_ = controller_.Scope();
      return Result("g13SetActive",
                    error == 34 ? "CONFIG_NOT_READY"
                    : error     ? "STALE_SCOPE"
                    : cmd.a     ? "ACTIVE"
                                : "INACTIVE",
                    error);
    }
    if (cmd.kind == Kind::Pulse) {
      error = controller_.Pulse(cmd.a, cmd.b, cmd.d, cmd.received);
      return Result("g13PagePulse", error ? "REJECTED" : "PULSE", error);
    }
    if (cmd.kind == Kind::Offer) {
      error = controller_.Offer(cmd.words, Now());
      return Result("g13Offer128",
                    error == 31 ? "ALREADY_CONSUMED_OR_STALE"
                    : error     ? "REJECTED"
                                : "QUEUED",
                    error, cmd.attempt);
    }
    if (cmd.kind == Kind::View) {
      error = scene_.View(uint32_t(cmd.a), cmd.view) ? 0 : 5;
      return Result("g13ViewMode", error ? "REJECTED" : "VIEW_READY", error);
    }
    if (cmd.kind == Kind::ColdBegin) {
      auto result =
          cold_.Begin(controller_, cmd.text, cmd.a != 0, scene_.Receipt());
      {
        std::lock_guard<std::mutex> lock(mutex_);
        coldReply_ = result;
      }
      return result;
    }
    if (cmd.kind == Kind::ColdPage)
      return cold_.Page(cmd.a, uint32_t(cmd.b), size_t(cmd.d));
    if (cmd.kind == Kind::ColdEnd)
      return cold_.End(cmd.a);
    return Result(api, "BAD_COMMAND", 5);
  }
  std::string RunCommand(Command &command) {
    try {
      return Execute(command);
    } catch (const std::exception &) {
      if (IsConfigure(command.kind)) {
        InitObservation().State("CONFIG_FAILED");
        InitObservation().Failure("OWNER_EXCEPTION", command.attempt);
        InitStageEvent("OWNER_EXCEPTION", "EXIT");
      }
      return Result(command.api.empty() ? "g13Lifecycle" : command.api.c_str(),
                    "OWNER_EXCEPTION", 30, command.attempt);
    }
  }
  void ServiceLifecycle() {
    if (controller_.Configured())
      controller_.PumpBounded(Now());
    if (initializing_) {
      if (pendingTicket_.cancelGeneration != revoked_.load()) {
        if (controller_.RootIdle()) {
          controller_.Release(context_ != EGL_NO_CONTEXT);
          initializing_ = false;
          configured_ = false;
          InitObservation().State("UNCONFIGURED");
          InitStageEvent("CONFIG_CANCELLED", "EXIT");
        }
      } else if (controller_.Ready()) {
        id_ = pendingTicket_.proposed;
        gGeneration.store(id_.config);
        configured_ = true;
        initializing_ = false;
        InitObservation().State("COMPUTE_READY");
        InitObservation().Handled(pendingTicket_.attempt);
        InitStageEvent("COMPUTE_READY", "EXIT");
      } else if (controller_.FailureCode() && controller_.RootIdle()) {
        initializing_ = false;
        InitObservation().State("CONFIG_FAILED");
        InitObservation().Failure("INIT_ERROR_" +
                                      std::to_string(controller_.FailureCode()),
                                  pendingTicket_.attempt);
      }
    }
    if (configurePending_ && controller_.RootIdle()) {
      if (deferredConfigure_.ticket.cancelGeneration != revoked_.load()) {
        configurePending_ = false;
        InitObservation().State("UNCONFIGURED");
        InitStageEvent("CONFIG_CANCELLED", "EXIT");
      } else if (window_) {
        // A synchronous caller already in the FIFO must finish before a
        // deferred configure can enter an unbounded driver call.
        {
          std::lock_guard<std::mutex> lock(mutex_);
          if (count_)
            return;
          configureCallInFlight_ = true;
        }
        auto pending = deferredConfigure_;
        configurePending_ = false;
        Publish(RunCommand(pending));
        std::lock_guard<std::mutex> lock(mutex_);
        configureCallInFlight_ = false;
      }
    }
  }
  void Loop() {
    InstallInitObserver();
    id_.owner = 13;
    id_.session = id_.permission = id_.inputEpoch = id_.control = 1;
    while (true) {
      Command command{};
      bool have = false;
      {
        std::unique_lock<std::mutex> lock(mutex_);
        if (!count_ && !quit_)
          wake_.wait_for(lock,
                         std::chrono::microseconds(configured_ ? 100 : 5000));
        if (quit_)
          break;
        if (count_) {
          command = std::move(queue_[head_]);
          queue_[head_] = {};
          head_ = (head_ + 1) % queue_.size();
          --count_;
          configureCallInFlight_ = IsConfigure(command.kind);
          have = true;
        }
      }
      if (have) {
        const auto reply = RunCommand(command);
        Publish(reply);
        {
          std::lock_guard<std::mutex> lock(mutex_);
          configureCallInFlight_ = false;
          handled_ = command.queueSerial;
          if (command.replySlot >= 0) {
            auto &slot = returns_[size_t(command.replySlot)];
            slot.reply = reply;
            slot.ready = true;
          }
          if (command.kind == Kind::Offer)
            sourceReply_ = reply;
        }
        wake_.notify_all();
      }
      ServiceLifecycle();
      if (configured_) {
        if (rebindPending_) {
          auto e = controller_.Rebind(id_.surface, geometryGeneration_);
          if (!e) {
            id_ = controller_.Scope();
            rebindPending_ = false;
          }
        }
        auto coldReply = cold_.Service(controller_, scene_.Receipt());
        if (!coldReply.empty()) {
          std::lock_guard<std::mutex> lock(mutex_);
          coldReply_ = coldReply;
        }
        if (!cold_.Pending() && !rebindPending_ && !configurePending_)
          controller_.BeginGranted(Now());
        uint64_t now = Now();
        if (sceneReady_ && now - lastDraw_ >= 33333333ull) {
          EGLint width = 0, height = 0;
          eglQuerySurface(display_, surface_, EGL_WIDTH, &width);
          eglQuerySurface(display_, surface_, EGL_HEIGHT, &height);
          lastDraw_ = now;
          if (scene_.Draw(controller_, width, height, [&]() {
                return eglSwapBuffers(display_, surface_) == EGL_TRUE;
              })) {
            lastDraw_ = now;
            if (firstSwapRun_ != id_.run) {
              firstSwapRun_ = id_.run;
              InitStageEvent("FIRST_VALID_SWAP", "EXIT");
            }
          }
        }
        Publish();
      } else
        Publish();
    }
    Destroy();
  }

public:
  void Revoke() { revoked_.fetch_add(1); }
  App() {
    controller_.SetAuthority(this, [](void *p) {
      auto &app = *static_cast<App *>(p);
      return app.armed_.load() == app.revoked_.load();
    });
    thread_ = std::thread([this]() { Loop(); });
  }
  ~App() {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      quit_ = true;
    }
    wake_.notify_all();
    thread_.join();
  }
  std::string Submit(Command command, bool synchronous = false) {
    const bool stop = command.kind == Kind::Stop ||
                      (command.kind == Kind::Active && !command.a);
    if (stop || IsConfigure(command.kind) || command.kind == Kind::Destroy ||
        command.kind == Kind::Resize)
      revoked_.fetch_add(1);
    uint64_t queueId = 0;
    int slotIndex = -1;
    const std::string api = command.api.empty() ? "g13Lifecycle" : command.api;
    const uint64_t apiAttempt = command.attempt;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (synchronous) {
        bool configureAhead = configureCallInFlight_;
        for (size_t i = 0; !configureAhead && i < count_; ++i)
          configureAhead = IsConfigure(queue_[(head_ + i) % queue_.size()].kind);
        if (configureAhead) {
          if (!stop)
            return Result(api.c_str(), "CONFIGURE_IN_PROGRESS", 34, apiAttempt);
          // Authority was revoked above. Owner-side cleanup follows when the
          // driver returns, with the same scope checks as a synchronous stop.
          synchronous = false;
        }
      }
      if (count_ == queue_.size()) {
        if (command.kind == Kind::Window && command.window)
          OH_NativeWindow_NativeObjectUnreference(command.window);
        return Result(api.c_str(), "FULL", 30, apiAttempt);
      }
      if (synchronous) {
        for (size_t i = 0; i < returns_.size(); i++)
          if (!returns_[i].used) {
            slotIndex = int(i);
            returns_[i].used = true;
            break;
          }
        if (slotIndex < 0)
          return Result(api.c_str(), "REPLY_SLOTS_FULL", 30, apiAttempt);
      }
      queueId = command.queueSerial = ++serial_;
      if (command.kind == Kind::Session)
        revoked_.fetch_add(1);
      if (command.kind == Kind::Configure || command.kind == Kind::Diagnostic) {
        command.ticket.boot = kBoot;
        command.ticket.attempt = command.attempt;
        command.ticket.queueSerial = queueId;
        command.ticket.cancelGeneration = revoked_.load();
        command.ticket.configRawSHA =
            Hash(command.words.data(), sizeof(command.words));
        command.ticket.seedReceiptSHA =
            Hash(command.text.data(), command.text.size());
        InitObservation().Queued(command.ticket.attempt);
        InitObservation().State("CONFIG_QUEUED");
        InitStageEvent("CONFIG_QUEUED", "ENTER");
      }
      command.replySlot = slotIndex;
      queue_[(head_ + count_) % queue_.size()] = std::move(command);
      ++count_;
    }
    wake_.notify_all();
    if (synchronous) {
      std::unique_lock<std::mutex> lock(mutex_);
      wake_.wait(lock,
                 [&]() { return returns_[size_t(slotIndex)].ready || quit_; });
      auto reply = std::move(returns_[size_t(slotIndex)].reply);
      returns_[size_t(slotIndex)] = {};
      return reply;
    }
    return Result(api.c_str(), "QUEUED", 0, apiAttempt);
  }
  std::string Status() {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto init=InitObservation().Summary();
    auto result=std::string("{\"route\":13,\"schema\":1,\"native\":")+status_+",\"cold\":"+coldReply_+",\"lastSourceReply\":"+sourceReply_+",\"scope\":"+scope_+",\"lastCommand\":"+reply_+",\"initialization\":"+init+"}";
    if(result.size()>16384)result=std::string("{\"route\":13,\"schema\":1,\"native\":")+status_+",\"scope\":"+scope_+",\"initialization\":"+init+",\"cold\":{\"state\":\"READ_NAMED_COLD_ENDPOINT\"},\"statusOmitted\":true}";
    if(result.size()>16384)return "{\"route\":13,\"state\":\"STATUS_UTF8_LIMIT\",\"initialization\":"+init+"}";
    return result;
  }
};
App &Product() {
  static App app;
  return app;
}
struct Decode {
  uint64_t attempt = 0;
  const char *stage = "ENTER";
  napi_status raw = napi_ok;
  grs::words::Observation typedObservation{};
  bool valid = false;
  uint32_t copiedWords = 0;
  std::array<uint32_t, 128> snapshot{};
};
std::mutex decodeMutex;
Decode lastDecode;
uint64_t decodeSerial = 0;
template <size_t N>
bool Words(napi_env e, napi_value v, std::array<uint32_t, N> &out, Decode &d) {
  auto result = grs::words::ReadTypedValue<N>(e, v);
  d.typedObservation = result.observed;
  d.stage = grs::words::StageName(result.firstFailedStage);
  const auto &o = result.observed;
  int raw = o.arraybufferInfoStatus >= 0  ? o.arraybufferInfoStatus
            : o.isDetachedStatus >= 0     ? o.isDetachedStatus
            : o.typedarrayInfoStatus >= 0 ? o.typedarrayInfoStatus
                                          : o.isTypedarrayStatus;
  d.raw = raw >= 0 ? static_cast<napi_status>(raw) : napi_ok;
  if (!result.ok)
    return false;
  out = result.words;
  std::copy(out.begin(), out.end(), d.snapshot.begin());
  d.valid = true;
  d.copiedWords = N;
  d.stage = "COPIED";
  return true;
}

bool Text(napi_env e, napi_value v, std::string &out, Decode &d) {
  size_t size = 0;
  d.stage = "STRING_LENGTH";
  d.raw = napi_get_value_string_utf8(e, v, nullptr, 0, &size);
  if (d.raw != napi_ok || size > 4096)
    return false;
  std::array<char, 4097> bytes{};
  size_t copied = 0;
  d.stage = "STRING_COPY";
  d.raw = napi_get_value_string_utf8(e, v, bytes.data(), bytes.size(), &copied);
  if (d.raw != napi_ok || copied != size)
    return false;
  out.assign(bytes.data(), size);
  return true;
}
std::string Decorate(std::string reply, const std::string &api,
                     const Decode &decode) {
  if (reply.empty() || reply.back() != '}')
    return Result(api.c_str(), "BAD_NATIVE_ENVELOPE", 28, decode.attempt);
  reply.pop_back();
  const auto &o = decode.typedObservation;
  if (o.isTypedarrayStatus >= 0) {
    reply +=
        ",\"typedarray\":{\"diagnosticRevision\":2,\"rawReturnedLength\":" +
        (o.typedarrayInfoStatus == napi_ok ? std::to_string(o.rawReturnedLength)
                                           : "null") +
        ",\"elementLength\":" +
        (o.typedarrayInfoStatus == napi_ok ? std::to_string(o.elementLength)
                                           : "null") +
        ",\"lengthUnit\":" + Quote(provider629::Name(o.lengthUnit)) +
        ",\"normalizedElementCount\":" +
        (o.normalizationObserved ? std::to_string(o.normalizedElementCount)
                                 : "null") +
        ",\"providerProfileId\":" + Quote(o.providerProfileId.data()) +
        ",\"providerEvidenceSHA\":" + Quote(o.providerEvidenceSHA.data()) +
        ",\"providerQualification\":" +
        provider629::Lookup(o.providerProfileId.data()) + ",\"apiStatuses\":[" +
        std::to_string(o.isTypedarrayStatus) + "," +
        std::to_string(o.typedarrayInfoStatus) + "," +
        std::to_string(o.isDetachedStatus) + "," +
        std::to_string(o.arraybufferInfoStatus) + "],\"backingBytes\":" +
        (o.arraybufferInfoStatus == napi_ok ? std::to_string(o.backingBytes)
                                            : "null") +
        "}";
  }

  if (reply.find("\"api\":") == std::string::npos)
    reply += ",\"api\":" + Quote(api);
  if (reply.find("\"boot\":") == std::string::npos)
    reply += ",\"boot\":\"" + std::to_string(kBoot) + "\"";
  reply += ",\"diagGeneration\":\"" + std::to_string(gGeneration.load()) +
           "\",\"attemptId\":\"" + std::to_string(decode.attempt) +
           "\",\"decodeStage\":" + Quote(decode.stage);
  if (reply.find("\"rawNapiStatus\":") == std::string::npos)
    reply += ",\"rawNapiStatus\":" + std::to_string(int(decode.raw));
  return reply + "}";
}
void SaveInitDecode(const Decode &d, bool valid) {
  InitDecode record;
  auto &r = record.value;
  const auto &o = d.typedObservation;
  r.attempt = d.attempt;
  r.rawReturnedLength = o.rawReturnedLength;
  r.normalizedElementCount = o.normalizedElementCount;
  r.backingBytes = o.backingBytes;
  r.offset = o.byteOffset;
  r.rawNapi = d.raw;
  r.typedKind = o.elementKind;
  r.status[0] = o.cbInfoStatus;
  r.status[1] = o.isTypedarrayStatus;
  r.status[2] = o.typedarrayInfoStatus;
  r.status[3] = o.isDetachedStatus;
  r.status[4] = o.arraybufferInfoStatus;
  r.status[5] = o.isSendableStatus;
  r.flags = (valid ? 1 : 0) | (o.dataPresent ? 2 : 0) |
            (o.basePresent ? 4 : 0) | (o.baseEqualsData ? 8 : 0) |
            (o.normalizationObserved ? 16 : 0) | (o.detachedValue ? 32 : 0) |
            (o.sendableValue ? 64 : 0);
  r.lengthUnit = uint32_t(o.lengthUnit);
  r.copiedWords = d.copiedWords;
  r.snapshot = d.snapshot;
  std::snprintf(r.stage, 40, "%s", d.stage);
  std::snprintf(r.profileId, 48, "%s", o.providerProfileId.data());
  std::snprintf(r.profileEvidenceSHA, 65, "%s", o.providerEvidenceSHA.data());
  InitObservation().Decode(record, valid);
}
bool Boolean(napi_env e, napi_value value, bool &out, Decode &d) {
  d.stage = "BOOLEAN";
  d.raw = napi_get_value_bool(e, value, &out);
  return d.raw == napi_ok;
}
bool Double(napi_env e, napi_value value, double &out, Decode &d) {
  d.stage = "NUMBER";
  d.raw = napi_get_value_double(e, value, &out);
  return d.raw == napi_ok;
}
napi_value TextValue(napi_env e, const std::string &s) {
  bool pending = false;
  if (napi_is_exception_pending(e, &pending) != napi_ok || pending)
    return nullptr;
  napi_value v = nullptr;
  napi_create_string_utf8(e, s.data(), s.size(), &v);
  return v;
}
napi_value Api(napi_env e, napi_callback_info info) {
  size_t argc = 4;
  napi_value args[4]{};
  void *data = nullptr;
  napi_get_cb_info(e, info, &argc, args, nullptr, &data);
  const char *name = static_cast<const char *>(data);
  std::string api = name ? name : "";
  Command command{};
  command.received = Now();
  bool ok = true, sync = false;
  Decode decode;
  {
    std::lock_guard<std::mutex> lock(decodeMutex);
    decode.attempt = ++decodeSerial;
  }
  if (api == "g13Stop" || api == "g13Configure" ||
      api == "g13DiagnosticBegin")
    Product().Revoke();
  if (api == "g13InitDiagnostics") {
    std::string attempt;
    double page = 0;
    bool valid = argc == 2 && Text(e, args[0], attempt, decode) &&
                 Double(e, args[1], page, decode) && std::isfinite(page) &&
                 page >= 0 && page < 4 && std::floor(page) == page;
    uint64_t value = 0;
    if (valid) {
      Json j("{\"id\":" + Quote(attempt) + "}");
      valid = j.Number("id", value);
    }
    return TextValue(
        e, valid ? InitObservation().Page(value, uint32_t(page))
                 : Result("g13InitDiagnostics", "INVALID_ARGUMENT", 6));
  }
  if (api == "g13Status")
    return TextValue(e, Decorate(Product().Status(), api, decode));
  if (api == "g13Configure" || api == "g13DiagnosticBegin") {
    command.kind = api == "g13Configure" ? Kind::Configure : Kind::Diagnostic;
    ok = argc == 2 && Words(e, args[0], command.words, decode) &&
         Text(e, args[1], command.text, decode);
  } else if (api == "g13Offer128") {
    command.kind = Kind::Offer;
    ok = argc == 1 && Words(e, args[0], command.words, decode);
  } else if (api == "g13BeginSession") {
    command.kind = Kind::Session;
    ok = argc == 1 && Text(e, args[0], command.text, decode);
    sync = true;
  } else if (api == "g13SetActive") {
    command.kind = Kind::Active;
    bool active = false;
    ok = argc == 2 && Boolean(e, args[0], active, decode) &&
         Text(e, args[1], command.text, decode);
    command.a = active;
    sync = true;
  } else if (api == "g13Stop") {
    command.kind = Kind::Stop;
    ok = argc == 1 && Text(e, args[0], command.text, decode);
    sync = true;
  } else if (api == "g13PagePulse") {
    command.kind = Kind::Pulse;
    std::string session, seq, ns;
    ok = argc == 3 && Text(e, args[0], session, decode) &&
         Text(e, args[1], seq, decode) && Text(e, args[2], ns, decode);
    Json values("{\"s\":" + Quote(session) + ",\"q\":" + Quote(seq) +
                ",\"t\":" + Quote(ns) + "}");
    ok = ok && values.Number("s", command.a) && values.Number("q", command.b) &&
         values.Number("t", command.d);
  } else if (api == "g13ViewMode") {
    command.kind = Kind::View;
    std::string mode;
    ok = argc == 2 && Text(e, args[0], mode, decode) &&
         Words(e, args[1], command.view, decode);
    command.a = mode == "VELOCITY"           ? 0
                : mode == "TRACERS"          ? 1
                : mode == "VORTICITY"        ? 2
                : mode == "DYNAMIC_PRESSURE" ? 3
                : mode == "TOTAL_PRESSURE"   ? 4
                                             : 5;
  } else if (api == "g13ColdBegin") {
    command.kind = Kind::ColdBegin;
    ok = argc == 2 && Text(e, args[0], command.text, decode) &&
         Text(e, args[1], command.text2, decode);
    ok = ok && (command.text2 == "ROOT" || command.text2 == "FULL");
    command.a = command.text2 == "FULL";
    sync = true;
  } else if (api == "g13ColdPage") {
    command.kind = Kind::ColdPage;
    std::string id;
    double segment = 0, offset = 0;
    ok = argc == 3 && Text(e, args[0], id, decode) &&
         Double(e, args[1], segment, decode) &&
         Double(e, args[2], offset, decode) && std::isfinite(segment) &&
         std::isfinite(offset) && segment >= 0 && segment < 8 && offset >= 0 &&
         offset <= 2000000 && std::floor(segment) == segment &&
         std::floor(offset) == offset;
    Json j("{\"id\":" + Quote(id) + "}");
    ok = ok && j.Number("id", command.a);
    command.b = uint64_t(segment);
    command.d = uint64_t(offset);
    sync = true;
  } else if (api == "g13ColdEnd") {
    command.kind = Kind::ColdEnd;
    std::string id;
    ok = argc == 1 && Text(e, args[0], id, decode);
    Json j("{\"id\":" + Quote(id) + "}");
    ok = ok && j.Number("id", command.a);
    sync = true;
  } else
    ok = false;
  {
    std::lock_guard<std::mutex> lock(decodeMutex);
    lastDecode = decode;
  }
  if (!ok) {
    std::ostringstream out;
    out << "{\"route\":13,\"api\":" << Quote(api)
        << ",\"state\":\"NAPI_DECODE\",\"attempt\":\"" << decode.attempt
        << "\",\"stage\":" << Quote(decode.stage) << ",\"error\":6,\"boot\":\""
        << kBoot << "\",\"generation\":\"" << gGeneration.load()
        << "\",\"snapshotValid\":" << (decode.valid ? "true" : "false")
        << ",\"copiedWords\":" << decode.copiedWords
        << ",\"rawNapiStatus\":" << int(decode.raw) << ",\"snapshot\":[";
    if (decode.valid)
      for (size_t i = 0; i < decode.copiedWords; i++) {
        if (i)
          out << ',';
        out << decode.snapshot[i];
      }
    out << "]}";
    const auto failure = Decorate(out.str(), api, decode);
    if (api == "g13Configure" || api == "g13DiagnosticBegin") {
      SaveInitDecode(decode, false);
      InitObservation().Failure(failure, decode.attempt);
    }
    return TextValue(e, failure);
  }
  if (api == "g13Configure" || api == "g13DiagnosticBegin") {
    SaveInitDecode(decode, true);
    InitObservation().Stage("DECODE_OK", "EXIT", 0, -1, nullptr, -1, -1, -1, -1,
                            decode.attempt);
  }
  command.attempt = decode.attempt;
  command.api = api;
  return TextValue(
      e, Decorate(Product().Submit(std::move(command), sync), api, decode));
}
void Created(OH_NativeXComponent *, void *window) {
  if (!window || OH_NativeWindow_NativeObjectReference(window) != 0)
    return;
  Command c{};
  c.kind = Kind::Window;
  c.window = window;
  Product().Submit(c, false);
}
void Changed(OH_NativeXComponent *, void *window) {
  Command c{};
  c.kind = Kind::Resize;
  c.window = window;
  Product().Submit(c, false);
}
void Destroyed(OH_NativeXComponent *, void *) {
  Command c{};
  c.kind = Kind::Destroy;
  Product().Submit(c, false);
}
OH_NativeXComponent_Callback callbacks = {Created, Changed, Destroyed, nullptr};
napi_value Init(napi_env e, napi_value exports) {
  const char *names[] = {"g13Configure",       "g13Offer128",
                         "g13PagePulse",       "g13Stop",
                         "g13InitDiagnostics", "g13Status",
                         "g13ColdBegin",       "g13ColdPage",
                         "g13ColdEnd",         "g13DiagnosticBegin",
                         "g13BeginSession",    "g13SetActive",
                         "g13ViewMode"};
  for (auto name : names) {
    napi_property_descriptor p = {
        name,    nullptr, Api,          nullptr,
        nullptr, nullptr, napi_default, const_cast<char *>(name)};
    napi_define_properties(e, exports, 1, &p);
  }
  napi_value component = nullptr;
  if (napi_get_named_property(e, exports, OH_NATIVE_XCOMPONENT_OBJ,
                              &component) == napi_ok) {
    OH_NativeXComponent *native = nullptr;
    if (napi_unwrap(e, component, reinterpret_cast<void **>(&native)) ==
            napi_ok &&
        native)
      OH_NativeXComponent_RegisterCallback(native, &callbacks);
  }
  return exports;
}
} // namespace
} // namespace grs::g13
static napi_module g13Module = {1,         0,       nullptr, grs::g13::Init,
                                "grs_g13", nullptr, {0}};
extern "C" __attribute__((constructor)) void RegisterG13() {
  napi_module_register(&g13Module);
}
