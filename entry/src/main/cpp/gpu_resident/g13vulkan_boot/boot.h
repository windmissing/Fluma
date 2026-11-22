#pragma once

#include <cstdint>
#include <functional>
#include <string>

#ifdef _WIN32
#include "platform.h"
#else
struct NativeWindow;
#endif

namespace grs::g13vk {
struct Attempt {
  uint64_t ticket;
  uint64_t surface;
  uint64_t geometry;
#ifdef _WIN32
  HWND window;
  bool presentBoot = false;
  std::function<bool()> current;
  bool steadyQualification = false;
#else
  NativeWindow *window;
#endif
};

using Publish = std::function<bool(const std::string &)>;
std::string RunBoot(const Attempt &attempt, const Publish &publish);
} // namespace grs::g13vk
