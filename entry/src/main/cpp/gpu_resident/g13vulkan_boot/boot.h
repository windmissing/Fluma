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
#ifdef _WIN32
struct PointerSample {
  uint64_t hold=0, sequence=0, timeNs=0;
  double x=0, y=0;
};
#endif
struct Attempt {
  uint64_t ticket;
  uint64_t surface;
  uint64_t geometry;
#ifdef _WIN32
  HWND window;
  bool presentBoot = false;
  std::function<bool()> current;
  bool steadyQualification = false;
  std::string steadyRole;
  bool disableOptimization = false;
  std::wstring m1SourceModule;
  std::function<uint64_t()> sourcePulse;
  std::wstring m1QuantumCandidate, m1QuantumCommit, m1ColdFile;
  std::function<bool(const std::string &)> pointerReady;
  std::function<PointerSample()> pointer;
  std::function<bool(uint64_t, const std::function<void()> &)> acceptPointer;
  bool m1SinglePresent = false;
  std::wstring t849WriteFile;
#else
  NativeWindow *window;
#endif
};

using Publish = std::function<bool(const std::string &)>;
std::string RunBoot(const Attempt &attempt, const Publish &publish);
} // namespace grs::g13vk
