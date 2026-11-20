#include "boot.h"
#include <ace/xcomponent/native_interface_xcomponent.h>
#include <condition_variable>
#include <hilog/log.h>
#include <mutex>
#include <napi/native_api.h>
#include <native_window/external_window.h>
#include <optional>
#include <thread>

namespace grs::g13vk {
namespace {
constexpr const char *kWaiting = "{\"schema\":\"G13VulkanBootV1\",\"state\":\"WAITING_FOR_XCOMPONENT\"}";
constexpr const char *kIdle = "{\"schema\":\"G13VulkanBootV1\",\"state\":\"IDLE\"}";
constexpr const char *kRevoked = "{\"schema\":\"G13VulkanBootV1\",\"state\":\"REVOKED\"}";
constexpr const char *kBusy = "{\"schema\":\"G13VulkanBootV1\",\"state\":\"OWNER_BUSY\"}";
constexpr const char *kRefFailure = "{\"schema\":\"G13VulkanBootV1\",\"state\":\"WINDOW_REFERENCE_FAILED\"}";

class Owner {
public:
  Owner() : worker_([this] { Loop(); }) {}
  ~Owner() {
    {
      std::lock_guard lock(mutex_);
      exiting_ = true;
      active_ = 0;
      if (window_)
        OH_NativeWindow_NativeObjectUnreference(window_);
    }
    signal_.notify_one();
    worker_.join();
  }
  std::string Start() {
    std::lock_guard lock(mutex_);
    if (busy_)
      return kBusy;
    if (!window_)
      return status_;
    if (OH_NativeWindow_NativeObjectReference(window_) != 0)
      return status_ = kRefFailure;
    active_ = ++ticket_;
    pending_ = Attempt{active_, surface_, geometry_, window_};
    busy_ = true;
    status_ = "{\"schema\":\"G13VulkanBootV1\",\"state\":\"QUEUED\",\"ticket\":\"" +
              std::to_string(active_) + "\"}";
    signal_.notify_one();
    return status_;
  }
  std::string Status() {
    std::lock_guard lock(mutex_);
    return status_;
  }
  std::string Stop() {
    std::lock_guard lock(mutex_);
    Revoke();
    return status_;
  }
  void Attach(OH_NativeXComponent *component, NativeWindow *window) {
    std::lock_guard lock(mutex_);
    if (component == component_ && window == window_)
      return;
    Revoke();
    ReleaseWindow();
    if (!window || OH_NativeWindow_NativeObjectReference(window) != 0) {
      status_ = kRefFailure;
      return;
    }
    component_ = component;
    window_ = window;
    ++surface_;
    ++geometry_;
    status_ = kIdle;
  }
  void Change(OH_NativeXComponent *component, NativeWindow *window) {
    std::lock_guard lock(mutex_);
    if (component == component_ && window == window_) {
      ++geometry_;
      Revoke();
    }
  }
  void Detach(OH_NativeXComponent *component, NativeWindow *window) {
    std::lock_guard lock(mutex_);
    if (component == component_ && (!window || window == window_)) {
      Revoke();
      ReleaseWindow();
    }
  }

private:
  std::mutex mutex_;
  std::condition_variable signal_;
  std::optional<Attempt> pending_;
  NativeWindow *window_ = nullptr;
  OH_NativeXComponent *component_ = nullptr;
  uint64_t ticket_ = 0, surface_ = 0, geometry_ = 0, active_ = 0;
  bool busy_ = false, exiting_ = false;
  std::string status_ = kWaiting;
  std::thread worker_;

  void ReleaseWindow() {
    if (window_)
      OH_NativeWindow_NativeObjectUnreference(window_);
    window_ = nullptr;
    component_ = nullptr;
  }
  void Revoke() {
    if (active_) {
      active_ = 0;
      status_ = kRevoked;
    }
  }
  void Loop() {
    for (;;) {
      std::unique_lock lock(mutex_);
      signal_.wait(lock, [this] { return exiting_ || pending_.has_value(); });
      if (exiting_) {
        if (pending_)
          OH_NativeWindow_NativeObjectUnreference(pending_->window);
        return;
      }
      const auto attempt = *pending_;
      pending_.reset();
      lock.unlock();
      const auto result = RunBoot(attempt, [this, &attempt](const std::string &value) {
        std::lock_guard guard(mutex_);
        if (active_ != attempt.ticket)
          return false;
        status_ = value;
        return true;
      });
      OH_NativeWindow_NativeObjectUnreference(attempt.window);
      lock.lock();
      busy_ = false;
      if (active_ != attempt.ticket)
        continue;
      status_ = result;
      active_ = 0;
      lock.unlock();
      constexpr size_t chunk = 600; // Keep each HiLog line below the platform payload cap.
      for (size_t offset = 0; offset < result.size(); offset += chunk) {
        const std::string part = result.substr(offset, chunk);
        OH_LOG_Print(LOG_APP, LOG_INFO, 0x0D00143, "G13VkBoot",
            "receipt %{public}llu %{public}u/%{public}u %{public}s",
            static_cast<unsigned long long>(attempt.ticket), unsigned(offset / chunk + 1),
            unsigned((result.size() + chunk - 1) / chunk), part.c_str());
      }
    }
  }
};

Owner &Probe() {
  static Owner owner;
  return owner;
}
void Created(OH_NativeXComponent *component, void *window) {
  Probe().Attach(component, static_cast<NativeWindow *>(window));
}
void Changed(OH_NativeXComponent *component, void *window) {
  Probe().Change(component, static_cast<NativeWindow *>(window));
}
void Destroyed(OH_NativeXComponent *component, void *window) {
  Probe().Detach(component, static_cast<NativeWindow *>(window));
}
OH_NativeXComponent_Callback callbacks{Created, Changed, Destroyed, nullptr};

using Action = std::string (Owner::*)();
template <Action action>
napi_value Call(napi_env env, napi_callback_info) {
  const auto reply = (Probe().*action)();
  napi_value value = nullptr;
  napi_create_string_utf8(env, reply.data(), reply.size(), &value);
  return value;
}

napi_value Init(napi_env env, napi_value exports) {
  napi_property_descriptor properties[] = {
      {"g13VkBootStart", nullptr, Call<&Owner::Start>, nullptr, nullptr, nullptr, napi_default, nullptr},
      {"g13VkBootStatus", nullptr, Call<&Owner::Status>, nullptr, nullptr, nullptr, napi_default, nullptr},
      {"g13VkBootStop", nullptr, Call<&Owner::Stop>, nullptr, nullptr, nullptr, napi_default, nullptr}};
  napi_define_properties(env, exports, sizeof(properties) / sizeof(properties[0]), properties);
  napi_value component = nullptr;
  OH_NativeXComponent *native = nullptr;
  if (napi_get_named_property(env, exports, OH_NATIVE_XCOMPONENT_OBJ, &component) == napi_ok &&
      napi_unwrap(env, component, reinterpret_cast<void **>(&native)) == napi_ok && native) {
    const int result = OH_NativeXComponent_RegisterCallback(native, &callbacks);
    if (result != OH_NATIVEXCOMPONENT_RESULT_SUCCESS)
      OH_LOG_Print(LOG_APP, LOG_ERROR, 0x0D00143, "G13VkBoot",
                   "OH_NativeXComponent_RegisterCallback=%{public}d", result);
  }
  return exports;
}
} // namespace
} // namespace grs::g13vk

static napi_module module{1, 0, nullptr, grs::g13vk::Init, "grs_g13vk_boot", nullptr, {0}};
extern "C" __attribute__((constructor)) void RegisterG13VulkanBoot() {
  napi_module_register(&module);
}
