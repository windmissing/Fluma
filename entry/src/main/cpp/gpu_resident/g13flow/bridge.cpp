#include "../napi_words_contract.h"
#include "../program_cache/artifact_store.h"
#include "cold.h"
#include "controller.h"
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
};
class App {
  std::mutex mutex_;
  std::condition_variable wake_;
  std::array<Command, 32> queue_{};
  size_t head_ = 0, count_ = 0;
  std::thread thread_;
  bool quit_ = false;
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
  uint64_t lastDraw_ = 0;
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
    display_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display_ == EGL_NO_DISPLAY ||
        !eglInitialize(display_, nullptr, nullptr))
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
    if (!eglChooseConfig(display_, attrs, &cfg, 1, &count) || count != 1)
      return false;
    EGLint ctx[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    context_ = eglCreateContext(display_, cfg, EGL_NO_CONTEXT, ctx);
    surface_ = eglCreateWindowSurface(
        display_, cfg, reinterpret_cast<EGLNativeWindowType>(window_), nullptr);
    if (context_ == EGL_NO_CONTEXT || surface_ == EGL_NO_SURFACE ||
        !eglMakeCurrent(display_, surface_, surface_, context_))
      return false;
    id_.context++;
    id_.surface++;
    id_.geometry++;
    return true;
  }
  bool Destroy() {
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
      if (!Destroy()) {
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
      rebindPending_ = true;
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
      if (!files.empty())
        programcache::Store().Configure(files);
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
      if (context_ == EGL_NO_CONTEXT && !Create())
        return Result("g13Configure", "WAIT_WINDOW", 29);
      Json json(cmd.text);
      if (!json.Valid())
        return Result("g13Configure", "BAD_SEED_RECEIPT", 5);
      Seed seed;
      seed.path = json.Text("path");
      seed.sha = json.Text("sha256");
      seed.receipt = cmd.text;
      uint64_t mode = 0, forced = 0;
      if (!json.Number("diagnosticMode", mode, false) ||
          !json.Number("forcedJ", forced, false) || mode > 3 ||
          (forced != 0 && forced != 1 && forced != 2 && forced != 4))
        return Result("g13Configure", "BAD_DIAGNOSTIC", 5);
      seed.diagnostic =
          cmd.kind == Kind::Diagnostic ? uint32_t(mode ? mode : 1) : 0;
      seed.forcedJ = uint32_t(forced);
      auto fault = json.Text("fault");
      if (!fault.empty()) {
        if (!PipelineTestHooks() || seed.diagnostic != 3)
          return Result("g13DiagnosticBegin", "TEST_HOOKS_DISABLED", 4);
        seed.testHook = fault == "PRESSURE_DISABLED"        ? 1
                        : fault == "PRESSURE_SIGN_REVERSED" ? 2
                        : fault == "RHS_INCOMPATIBLE"       ? 3
                        : fault == "GAUGE_OFFSET"           ? 4
                        : fault == "NORMAL_WALL"            ? 5
                                                            : 6;
      }
      Identity proposed = id_;
      ++proposed.run;
      ++proposed.config;
      error = controller_.Configure(cmd.words, seed, proposed);
      if (!error) {
        id_ = proposed;
        gGeneration.store(id_.config);
      }
      if (!error) {
        configured_ = true;
        configurePending_ = false;
        sceneReady_ = scene_.Prepare(id_.context, cmd.words);
      } else if (error == 34) {
        deferredConfigure_ = cmd;
        configurePending_ = true;
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
      Identity supplied;
      Json j(cmd.text);
      if (!ReadScope(j, supplied) || !(supplied == id_))
        return Result("g13SetActive", "STALE_SCOPE", 2);
      if (cmd.a)
        armed_.store(revoked_.load());
      controller_.SetActive(cmd.a != 0, id_);
      id_ = controller_.Scope();
      return Result("g13SetActive", cmd.a ? "ACTIVE" : "INACTIVE");
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
  void Loop() {
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
          have = true;
        }
      }
      if (have) {
        std::string reply;
        try {
          reply = Execute(command);
        } catch (const std::exception &) {
          reply =
              Result(command.api.empty() ? "g13Lifecycle" : command.api.c_str(),
                     "OWNER_EXCEPTION", 30, command.attempt);
        }
        Publish(reply);
        {
          std::lock_guard<std::mutex> lock(mutex_);
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
      if (configured_) {
        controller_.PumpBounded(Now());
        if (rebindPending_) {
          auto e = controller_.Rebind(id_.surface, id_.geometry + 1);
          if (!e) {
            id_ = controller_.Scope();
            rebindPending_ = false;
          }
        }
        if (configurePending_ && controller_.RootIdle()) {
          auto pending = deferredConfigure_;
          configurePending_ = false;
          Publish(Execute(pending));
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
              }))
            lastDraw_ = now;
        }
        Publish();
      }
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
    if (command.kind == Kind::Stop || command.kind == Kind::Session ||
        command.kind == Kind::Configure || command.kind == Kind::Diagnostic ||
        command.kind == Kind::Destroy || command.kind == Kind::Resize ||
        (command.kind == Kind::Active && !command.a))
      revoked_.fetch_add(1);
    uint64_t queueId = 0;
    int slotIndex = -1;
    const std::string api = command.api.empty() ? "g13Lifecycle" : command.api;
    const uint64_t apiAttempt = command.attempt;
    {
      std::lock_guard<std::mutex> lock(mutex_);
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
    return std::string("{\"route\":13,\"schema\":1,\"native\":") + status_ +
           ",\"cold\":" + coldReply_ + ",\"lastSourceReply\":" + sourceReply_ +
           ",\"scope\":" + scope_ + ",\"lastCommand\":" + reply_ + "}";
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
      api == "g13DiagnosticBegin" || api == "g13BeginSession")
    Product().Revoke();
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
    return TextValue(e, Decorate(out.str(), api, decode));
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
  Product().Submit(c, true);
}
void Changed(OH_NativeXComponent *, void *window) {
  Command c{};
  c.kind = Kind::Resize;
  c.window = window;
  Product().Submit(c, true);
}
void Destroyed(OH_NativeXComponent *, void *) {
  Command c{};
  c.kind = Kind::Destroy;
  Product().Submit(c, true);
}
OH_NativeXComponent_Callback callbacks = {Created, Changed, Destroyed, nullptr};
napi_value Init(napi_env e, napi_value exports) {
  const char *names[] = {
      "g13Configure",       "g13Offer128",     "g13PagePulse", "g13Stop",
      "g13Status",          "g13ColdBegin",    "g13ColdPage",  "g13ColdEnd",
      "g13DiagnosticBegin", "g13BeginSession", "g13SetActive", "g13ViewMode"};
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
