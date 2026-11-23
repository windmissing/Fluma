#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#ifdef _WIN32
#include "platform.h"
#include "m1_ingress.h"
#include "m1_hot_capture.h"
#include "m1_pulse_queue.h"
#include "g13_candidate_timing.h"
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
  bool t854PrivateWrites = false;
  std::function<void(SourceScope)> sourceScope;
  M1Ingress *ingress = nullptr;
  M1HotCapture *hotCapture = nullptr;
  bool m1DualSource = false;
  bool m1ColdOnBusy = false;
  std::wstring t833CandidateFile;
  std::string t833RevokeAt;
  std::function<void()> t833Revoke;
  std::string t833Case;
  std::wstring t833ConfigFile,t833SeedFile;
  uint32_t t833ForcedJ=0;
  bool m1RootCapture=false;
  std::wstring m1ReplayPrefix;
  bool m1D1=false;
  std::function<std::vector<pc::CandidatePulse>(uint64_t)> t833Pulses;
  bool t833F05Rest=false;
  M1PulseQueue *m1Pulses=nullptr;
  std::function<void()> disarmM1Input;
  bool m1Trace=false;
  std::string t877Mode;
#else
  NativeWindow *window;
#endif
};

using Publish = std::function<bool(const std::string &)>;
std::string RunBoot(const Attempt &attempt, const Publish &publish);
} // namespace grs::g13vk
