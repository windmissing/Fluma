#pragma once

#include <cstdint>
#include <functional>
#include <string>

struct NativeWindow;

namespace grs::g13vk {
struct Attempt {
  uint64_t ticket;
  uint64_t surface;
  uint64_t geometry;
  NativeWindow *window;
};

using Publish = std::function<bool(const std::string &)>;
std::string RunBoot(const Attempt &attempt, const Publish &publish);
} // namespace grs::g13vk
