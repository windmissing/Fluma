#include "../g9rx/loaded_identity.h"
#include "../napi_words_contract.h"
#include "controller.h"
#include "scene.h"
#include "wire.h"
#include <ace/xcomponent/native_interface_xcomponent.h>
#include <cmath>
#include <condition_variable>
#include <native_window/external_window.h>
#include <thread>
extern "C" __attribute__((visibility("default"))) uint32_t
G11NativeIdentityAnchor() {
  return 2;
}
namespace grs::g11vis {
namespace {
struct Command {
  std::string api, a, b, c;
  Words64 config{};
  Input128 input{};
  uint64_t x = 0, y = 0, z = 0;
  uint32_t d = 0;
  bool flag = false;
  void *window = nullptr;
  int reply = -1;
};
class App {
  std::mutex mutex_;
  std::condition_variable wake_;
  std::array<Command, 32> queue_{};
  size_t head_ = 0, count_ = 0;
  struct Reply {
    bool used = false, ready = false, abandoned = false;
    std::string text;
  };
  std::array<Reply, 32> replies_{};
  bool quit_ = false;
  std::thread thread_;
  Controller owner_;
  CompositeScene scene_;
  Scope id_{};
  EGLDisplay display_ = EGL_NO_DISPLAY;
  EGLContext context_ = EGL_NO_CONTEXT;
  EGLSurface surface_ = EGL_NO_SURFACE;
  void *window_ = nullptr;
  uint64_t run_ = 0, contextSerial_ = 0, surfaceSerial_ = 0, geometry_ = 0,
           frame_ = 0, lastDraw_ = 0;
  std::string status_ = "{\"state\":\"OFF\"}", lastReply_ = "{}",
              sceneState_ = "OFF", scope_ = "{}",
              loaded_ = "{\"state\":\"NOT_OBSERVED\"}";
  bool Create() {
    if (!window_ || context_ != EGL_NO_CONTEXT ||
        contextSerial_ == UINT64_MAX || surfaceSerial_ == UINT64_MAX ||
        geometry_ == UINT64_MAX)
      return false;
    display_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display_ == EGL_NO_DISPLAY ||
        !eglInitialize(display_, nullptr, nullptr))
      return false;
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
    EGLConfig config = nullptr;
    EGLint count = 0;
    if (!eglChooseConfig(display_, attributes, &config, 1, &count) ||
        count != 1)
      return false;
    const EGLint version[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    context_ = eglCreateContext(display_, config, EGL_NO_CONTEXT, version);
    if (context_ == EGL_NO_CONTEXT)
      return false;
    surface_ = eglCreateWindowSurface(
        display_, config, reinterpret_cast<EGLNativeWindowType>(window_),
        nullptr);
    if (surface_ == EGL_NO_SURFACE ||
        !eglMakeCurrent(display_, surface_, surface_, context_))
      return false;
    ++contextSerial_;
    ++surfaceSerial_;
    ++geometry_;
    return true;
  }
  bool Destroy(bool keepWindow = false) {
    owner_.RevokeAsync();
    const bool current =
        context_ != EGL_NO_CONTEXT && eglGetCurrentContext() == context_;
    owner_.Release(current);
    scene_.Reset(current);
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
        context_ = EGL_NO_CONTEXT;
        // This acknowledgement follows the real EGL destruction, never a
        // logical epoch change.
        if (!owner_.Release(false, true))
          return false;
      }
      if (!eglTerminate(display_))
        return false;
      display_ = EGL_NO_DISPLAY;
    }
    if (!keepWindow && window_) {
      OH_NativeWindow_NativeObjectUnreference(window_);
      window_ = nullptr;
    }
    return true;
  }
  std::string Execute(Command &c) {
    if (c.api == "WINDOW") {
      if (window_ == c.window) {
        OH_NativeWindow_NativeObjectUnreference(c.window);
        return "{\"state\":\"WINDOW_AVAILABLE\"}";
      }
      if (!Destroy()) {
        OH_NativeWindow_NativeObjectUnreference(c.window);
        return "{\"state\":\"CONTEXT_RETIRE_FAILED\"}";
      }
      window_ = c.window;
      return "{\"state\":\"WINDOW_AVAILABLE\"}";
    }
    if (c.api == "DESTROY")
      return Destroy() ? "{\"state\":\"WINDOW_RETIRED\"}"
                       : "{\"state\":\"CONTEXT_RETIRE_FAILED\"}";
    if (c.api == "RESIZE") {
      if (c.window != window_)
        return "{\"state\":\"STALE_WINDOW\"}";
      owner_.Stop();
      if (geometry_ != UINT64_MAX)
        ++geometry_;
      return "{\"state\":\"RECONFIGURE_REQUIRED\"}";
    }
    if (c.api == "g11Preset") {
      auto p = ParentAdapter::Research(c.flag ? ParentKind::DM : ParentKind::R1,
                                       uint32_t(c.x), c.d != 0);
      if (!p)
        return "{\"state\":\"INVALID_PARENT\"}";
      const auto config = VisualConfig(*p, uint32_t(c.y), uint32_t(c.z));
      if (!ValidateVisualConfig(config, *p))
        return "{\"state\":\"INVALID_CONFIG\"}";
      std::ostringstream o;
      o << "{\"parentRequest\":" << Quote(p->Request())
        << ",\"seedReceipt\":" << Quote(p->SeedReceipt()) << ",\"config\":[";
      for (size_t i = 0; i < 64; ++i) {
        if (i)
          o << ',';
        o << config[i];
      }
      o << "]}";
      return o.str();
    }
    if (c.api == "g11ResearchConfigure") {
      const auto kind = c.a == "DM" ? ParentKind::DM : ParentKind::R1;
      auto parent = ParentAdapter::Parse(kind, c.b, c.c);
      if (!parent || !ValidateVisualConfig(c.config, *parent))
        return "{\"state\":\"PARENT_CONFIG_REJECTED\"}";
      if (!window_)
        return "{\"state\":\"NO_WINDOW\"}";
      if (!Destroy(true) || !Create())
        return "{\"state\":\"EGL_CREATE_FAILED\"}";
      if (run_ == UINT64_MAX)
        return "{\"state\":\"RUN_OVERFLOW\"}";
      ++run_;
      id_ = {run_,           1,    contextSerial_, run_, run_, run_,
             surfaceSerial_, run_, surfaceSerial_};
      frame_ = lastDraw_ = 0;
      const auto actual = g9rx::ReadLoadedIdentity(
          reinterpret_cast<void *>(&G11NativeIdentityAnchor));
      loaded_ = actual.receipt;
      if (!actual.valid) {
        Destroy(true);
        return "{\"state\":\"LOADED_MODULE_IDENTITY_UNAVAILABLE\","
               "\"loadedModule\":" +
               loaded_ + "}";
      }
      const auto result = owner_.Configure(*parent, c.config, id_);
      if (result != Controller::Result::IN_PROGRESS)
        Destroy(true);
      else {
        EGLint w = 0, h = 0;
        const bool size = eglQuerySurface(display_, surface_, EGL_WIDTH, &w) &&
                          eglQuerySurface(display_, surface_, EGL_HEIGHT, &h);
        sceneState_ = size && w >= 128 && h > 0 &&
                              scene_.PrepareFrame(uint32_t(w), uint32_t(h),
                                                  contextSerial_)
                          ? "COLD_PREPARED"
                          : "SCENE_PREPARE_FAILED";
      }
      return "{\"state\":\"CONFIGURE_RESULT\",\"code\":" +
             std::to_string(uint32_t(result)) + ",\"loadedModule\":" + loaded_ +
             "}";
    }
    Scope scope;
    if (c.api == "g11BeginSession")
      return DecodeScope(c.a, scope) && owner_.BeginSession(scope)
                 ? "{\"state\":\"SESSION_OPEN\"}"
                 : "{\"state\":\"SESSION_REJECTED\"}";
    if (c.api == "g11SetActive")
      return DecodeScope(c.a, scope) && owner_.SetActive(c.flag, scope)
                 ? "{\"state\":\"ACTIVE_UPDATED\"}"
                 : "{\"state\":\"ACTIVE_REJECTED\"}";
    if (c.api == "g11PagePulse")
      return owner_.Pulse(c.x, c.y, c.z) ? "{\"state\":\"PULSE_ACCEPTED\"}"
                                         : "{\"state\":\"PULSE_REJECTED\"}";
    if (c.api == "g11Stop") {
      if (!DecodeScope(c.a, scope) || !owner_.SetActive(false, scope))
        return "{\"state\":\"STALE_SCOPE\"}";
      owner_.Stop();
      return "{\"state\":\"STOPPED\"}";
    }
    if (c.api == "g11Offer128" || c.api == "g11ModelCommand128") {
      auto rc = c.api == "g11Offer128" ? owner_.Offer(c.input, c.x)
                                       : owner_.Model(c.input, c.x);
      return "{\"state\":\"INPUT_RESULT\",\"code\":" +
             std::to_string(uint32_t(rc)) + "}";
    }
    if (c.api == "g11Target")
      return DecodeScope(c.a, scope) && owner_.Target(int32_t(c.d), scope)
                 ? "{\"state\":\"TARGET_ACCEPTED\"}"
                 : "{\"state\":\"TARGET_REJECTED\"}";
    if (c.api == "g11BootstrapSeal")
      return owner_.SealBootstrap()
                 ? "{\"state\":\"BOOTSTRAP_SEALED_WITNESS_FREED\"}"
                 : "{\"state\":\"BOOTSTRAP_SEAL_REJECTED\"}";
    if (c.api == "g11BootstrapState")
      return owner_.BootstrapState();
    if (c.api == "g11BootstrapPage")
      return owner_.BootstrapPage(uint32_t(c.x), uint32_t(c.y));
    if (c.api == "g11ColdBegin")
      return owner_.ColdBegin(c.a, c.flag);
    if (c.api == "g11ColdState")
      return owner_.ColdState(c.x);
    if (c.api == "g11ColdPage")
      return owner_.ColdPage(c.x, uint32_t(c.y), uint32_t(c.z));
    if (c.api == "g11ColdEnd")
      return owner_.ColdEnd(c.x);
    if (c.api == "g11ColdCapabilities")
      return owner_.ColdCapabilities();
    if (c.api == "g11WitnessArm")
      return owner_.ArmWitness(c.x, uint32_t(c.y), uint32_t(c.z), c.d)
                 ? "{\"state\":\"WITNESS_ARMED\"}"
                 : "{\"state\":\"WITNESS_WOULD_BLOCK\"}";
    if (c.api == "g11WitnessState")
      return owner_.WitnessState();
    if (c.api == "g11WitnessPage")
      return owner_.WitnessPage(uint32_t(c.x));
    if (c.api == "g11WitnessEnd")
      return owner_.EndWitness() ? "{\"state\":\"WITNESS_FREED\"}"
                                 : "{\"state\":\"RETIRED_NOT_FREED\"}";
    return "{\"state\":\"UNKNOWN_COMMAND\"}";
  }
  void Publish(const std::string &reply = "") {
    auto text = owner_.Status();
    text.pop_back();
    std::string latest;
    {
      std::lock_guard<std::mutex> l(mutex_);
      if (!reply.empty())
        lastReply_ = reply.find("\"words\":[") != std::string::npos
                         ? "{\"state\":\"RAW_PAGE_RETURNED\"}"
                         : reply;
      latest = lastReply_;
    }
    text += ",\"scene\":" + Quote(sceneState_) +
            ",\"loadedModule\":" + loaded_ + ",\"lastReply\":" + latest + "}";
    std::lock_guard<std::mutex> l(mutex_);
    status_ = std::move(text);
    scope_ = owner_.ScopeJson();
  }
  static bool Swap(void *u) {
    auto &self = *static_cast<App *>(u);
    return eglSwapBuffers(self.display_, self.surface_) == EGL_TRUE &&
           eglGetError() == EGL_SUCCESS;
  }
  void Loop() {
    for (;;) {
      Command cmd;
      bool has = false;
      {
        std::lock_guard<std::mutex> l(mutex_);
        if (quit_)
          break;
        if (count_) {
          cmd = std::move(queue_[head_]);
          queue_[head_] = {};
          head_ = (head_ + 1) % 32;
          --count_;
          has = true;
        }
      }
      if (has) {
        std::string reply;
        try {
          reply = Execute(cmd);
        } catch (const std::exception &) {
          owner_.Stop();
          reply = "{\"state\":\"NATIVE_EXCEPTION\"}";
        }
        Publish(reply);
        if (cmd.reply >= 0) {
          std::lock_guard<std::mutex> l(mutex_);
          auto &r = replies_[cmd.reply];
          if (r.abandoned)
            r = {};
          else {
            r.text = reply;
            r.ready = true;
          }
          wake_.notify_all();
        }
      }
      if (context_ != EGL_NO_CONTEXT && owner_.Configured()) {
        if (frame_ == UINT64_MAX)
          owner_.Stop();
        else
          owner_.Pump(frame_ + 1, geometry_);
        const auto now = Controller::Now();
        if (now >= lastDraw_ && now - lastDraw_ >= 33333333ull) {
          EGLint w = 0, h = 0;
          bool geometry = eglQuerySurface(display_, surface_, EGL_WIDTH, &w) &&
                          eglQuerySurface(display_, surface_, EGL_HEIGHT, &h);
          std::string token;
          if (geometry && w >= 128 && h > 0 &&
              scene_.Draw(owner_, uint32_t(w), uint32_t(h), Swap, this,
                          token)) {
            ++frame_;
            lastDraw_ = now;
          }
        }
        Publish();
      }
      if (!owner_.PendingOwnerWork()) {
        std::unique_lock<std::mutex> l(mutex_);
        wake_.wait_for(l, std::chrono::milliseconds(1),
                       [&] { return quit_ || count_; });
      }
    }
    Destroy();
  }

public:
  static constexpr size_t ReplyStorageBytes() {
    return sizeof(std::array<Reply, 32>);
  }
  App() {
    thread_ = std::thread([this] { Loop(); });
  }
  ~App() {
    {
      std::lock_guard<std::mutex> l(mutex_);
      quit_ = true;
    }
    wake_.notify_all();
    thread_.join();
  }
  void Revoke() { owner_.RevokeAsync(); }
  void RevokeScope(const std::string &raw) {
    Scope a, b;
    std::lock_guard<std::mutex> l(mutex_);
    if (!DecodeScope(raw, a) || !DecodeScope(scope_, b))
      return;
    if (a.run == b.run && a.owner == b.owner && a.context == b.context &&
        a.inputEpoch == b.inputEpoch && a.pageSession == b.pageSession &&
        a.permission == b.permission && a.window == b.window &&
        a.config == b.config && a.surface == b.surface)
      owner_.RevokeAsync();
  }
  std::string Status() {
    std::lock_guard<std::mutex> l(mutex_);
    return status_;
  }
  std::string Submit(Command c, bool sync) {
    std::unique_lock<std::mutex> l(mutex_);
    if (count_ == 32) {
      if (c.window && c.api == "WINDOW")
        OH_NativeWindow_NativeObjectUnreference(c.window);
      return "{\"state\":\"COMMAND_QUEUE_FULL\"}";
    }
    int slot = -1;
    if (sync) {
      for (int i = 0; i < 32; ++i)
        if (!replies_[i].used) {
          slot = i;
          break;
        }
      if (slot < 0)
        return "{\"state\":\"REPLY_WOULD_BLOCK\"}";
      replies_[slot].used = true;
      c.reply = slot;
    }
    queue_[(head_ + count_) % 32] = std::move(c);
    ++count_;
    wake_.notify_all();
    if (!sync)
      return "{\"state\":\"QUEUED\"}";
    if (!wake_.wait_for(l, std::chrono::seconds(5),
                        [&] { return replies_[slot].ready; })) {
      replies_[slot].abandoned = true;
      return "{\"state\":\"REPLY_PENDING\"}";
    }
    auto text = std::move(replies_[slot].text);
    replies_[slot] = {};
    return text;
  }
};
// Compiled host bookkeeping, distinct from the 512B bootstrap receipt.
extern "C" __attribute__((visibility("default")))
const uint64_t G11OwnerAbiV2[] = {2,
                                  sizeof(App),
                                  sizeof(Command),
                                  32 * sizeof(Command),
                                  App::ReplyStorageBytes(),
                                  sizeof(std::thread),
                                  sizeof(g9rx::LoadedIdentity)};
App &Product() {
  static App app;
  return app;
}
napi_value Text(napi_env e, const std::string &s) {
  napi_value out = nullptr;
  napi_create_string_utf8(e, s.data(), s.size(), &out);
  return out;
}
bool ReadText(napi_env e, napi_value v, std::string &out) {
  size_t n = 0;
  if (napi_get_value_string_utf8(e, v, nullptr, 0, &n) != napi_ok || n > 4096)
    return false;
  std::array<char, 4097> b{};
  size_t actual = 0;
  if (napi_get_value_string_utf8(e, v, b.data(), b.size(), &actual) !=
          napi_ok ||
      actual != n)
    return false;
  out.assign(b.data(), actual);
  return out.find('\0') == std::string::npos;
}
bool ReadU64(napi_env e, napi_value v, uint64_t &n) {
  std::string s;
  return ReadText(e, v, s) && Decimal(s, n);
}
bool ReadU32(napi_env e, napi_value v, uint32_t &n) {
  double x = 0;
  if (napi_get_value_double(e, v, &x) != napi_ok || !std::isfinite(x) ||
      x < 0 || x > 4294967295.0 || std::floor(x) != x)
    return false;
  n = uint32_t(x);
  return true;
}
napi_value Api(napi_env e, napi_callback_info info) {
  napi_value args[6]{};
  size_t argc = 6;
  void *data = nullptr;
  if (napi_get_cb_info(e, info, &argc, args, nullptr, &data) != napi_ok ||
      !data)
    return Text(e, "{\"state\":\"CB_INFO_FAILED\"}");
  Command c;
  c.api = static_cast<const char *>(data);
  bool ok = false, sync = false;
  const auto &api = c.api;
  if (api == "g11Status")
    return Text(e,
                argc == 0 ? Product().Status() : "{\"state\":\"ARG_COUNT\"}");
  if (api == "g11Preset") {
    uint32_t k = 0, v = 0, b = 0;
    ok = argc == 5 && ReadText(e, args[0], c.a) &&
         (c.a == "R1" || c.a == "DM") && ReadU32(e, args[1], k) &&
         ReadU32(e, args[2], v) &&
         napi_get_value_bool(e, args[3], &c.flag) == napi_ok &&
         ReadU32(e, args[4], b);
    c.x = k;
    c.y = v;
    c.z = b;
    c.d = c.flag;
    c.flag = c.a == "DM";
    sync = true;
  } else if (api == "g11ResearchConfigure") {
    ok = argc == 4 && ReadText(e, args[0], c.a) &&
         (c.a == "R1" || c.a == "DM") && ReadText(e, args[1], c.b) &&
         ReadText(e, args[3], c.c);
    if (ok) {
      auto decoded = words::ReadTypedValue<64>(e, args[2]);
      ok = decoded.ok;
      c.config = decoded.words;
      if (!ok)
        return Text(e, "{\"state\":\"TYPED_CONFIG_REJECTED\",\"stage\":" +
                           Quote(words::StageName(decoded.firstFailedStage)) +
                           "}");
    }
    if (ok)
      Product().Revoke();
  } else if (api == "g11BeginSession" || api == "g11Stop") {
    ok = argc == 1 && ReadText(e, args[0], c.a);
    Scope s;
    ok = ok && DecodeScope(c.a, s);
    if (ok && api == "g11Stop")
      Product().RevokeScope(c.a);
  } else if (api == "g11SetActive") {
    ok = argc == 2 && napi_get_value_bool(e, args[0], &c.flag) == napi_ok &&
         ReadText(e, args[1], c.a);
    Scope s;
    ok = ok && DecodeScope(c.a, s);
    if (ok && !c.flag)
      Product().RevokeScope(c.a);
  } else if (api == "g11PagePulse") {
    ok = argc == 3 && ReadU64(e, args[0], c.x) && ReadU64(e, args[1], c.y) &&
         ReadU64(e, args[2], c.z);
  } else if (api == "g11Offer128" || api == "g11ModelCommand128") {
    ok = argc == 2 && ReadU64(e, args[1], c.x);
    if (ok) {
      auto decoded = words::ReadTypedValue<128>(e, args[0]);
      ok = decoded.ok;
      c.input = decoded.words;
      if (!ok)
        return Text(e, "{\"state\":\"TYPED_INPUT_REJECTED\",\"stage\":" +
                           Quote(words::StageName(decoded.firstFailedStage)) +
                           "}");
    }
  } else if (api == "g11Target") {
    double angle = 0;
    ok = argc == 2 && napi_get_value_double(e, args[0], &angle) == napi_ok &&
         std::isfinite(angle) && std::floor(angle) == angle && angle >= -7680 &&
         angle <= 7680 && ReadText(e, args[1], c.a);
    if (ok)
      c.d = uint32_t(int32_t(angle));
  } else if (api == "g11BootstrapSeal" || api == "g11BootstrapState" ||
             api == "g11ColdCapabilities" || api == "g11WitnessState" ||
             api == "g11WitnessEnd") {
    ok = argc == 0;
    sync = api != "g11BootstrapSeal";
  } else if (api == "g11BootstrapPage") {
    uint32_t s = 0, o = 0;
    ok = argc == 2 && ReadU32(e, args[0], s) && ReadU32(e, args[1], o) &&
         s < 4 && o % 4 == 0;
    c.x = s;
    c.y = o;
    sync = true;
  } else if (api == "g11ColdBegin") {
    ok = argc == 2 && ReadText(e, args[0], c.a) && ReadText(e, args[1], c.b) &&
         (c.b == "ROOT" || c.b == "FULL");
    c.flag = c.b == "FULL";
    sync = true;
  } else if (api == "g11ColdState" || api == "g11ColdEnd") {
    ok = argc == 1 && ReadU64(e, args[0], c.x);
    sync = true;
  } else if (api == "g11ColdPage") {
    uint32_t s = 0, o = 0;
    ok = argc == 3 && ReadU64(e, args[0], c.x) && ReadU32(e, args[1], s) &&
         ReadU32(e, args[2], o) && s <= 17;
    c.y = s;
    c.z = o;
    sync = true;
  } else if (api == "g11WitnessArm") {
    uint32_t m = 0, p = 0, r = 0;
    ok = argc == 4 && ReadU64(e, args[0], c.x) && ReadU32(e, args[1], m) &&
         ReadU32(e, args[2], p) && ReadU32(e, args[3], r);
    c.y = m;
    c.z = p;
    c.d = r;
    sync = true;
  } else if (api == "g11WitnessPage") {
    uint32_t o = 0;
    ok = argc == 1 && ReadU32(e, args[0], o);
    c.x = o;
    sync = true;
  }
  if (!ok)
    return Text(e, "{\"state\":\"NAPI_DECODE_REJECTED\",\"api\":" + Quote(api) +
                       "}");
  return Text(e, Product().Submit(std::move(c), sync));
}
void Created(OH_NativeXComponent *, void *window) {
  if (!window || OH_NativeWindow_NativeObjectReference(window))
    return;
  Command c;
  c.api = "WINDOW";
  c.window = window;
  Product().Submit(std::move(c), false);
}
void Changed(OH_NativeXComponent *, void *window) {
  Product().Revoke();
  Command c;
  c.api = "RESIZE";
  c.window = window;
  Product().Submit(std::move(c), false);
}
void Destroyed(OH_NativeXComponent *, void *) {
  Product().Revoke();
  Command c;
  c.api = "DESTROY";
  Product().Submit(std::move(c), false);
}
OH_NativeXComponent_Callback callbacks = {Created, Changed, Destroyed, nullptr};
napi_value Init(napi_env e, napi_value exports) {
  const char *names[] = {"g11Preset",
                         "g11ResearchConfigure",
                         "g11BeginSession",
                         "g11SetActive",
                         "g11PagePulse",
                         "g11Offer128",
                         "g11ModelCommand128",
                         "g11Target",
                         "g11Stop",
                         "g11Status",
                         "g11BootstrapState",
                         "g11BootstrapPage",
                         "g11BootstrapSeal",
                         "g11ColdCapabilities",
                         "g11ColdBegin",
                         "g11ColdState",
                         "g11ColdPage",
                         "g11ColdEnd",
                         "g11WitnessArm",
                         "g11WitnessState",
                         "g11WitnessPage",
                         "g11WitnessEnd"};
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
} // namespace grs::g11vis
static napi_module g11Module = {1,         0,       nullptr, grs::g11vis::Init,
                                "grs_g11", nullptr, {0}};
extern "C" __attribute__((constructor)) void RegisterG11() {
  napi_module_register(&g11Module);
}
