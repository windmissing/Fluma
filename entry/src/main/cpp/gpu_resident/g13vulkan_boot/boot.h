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
#include "m1_d1_drain.h"
#include "m1_observation.h"
#include "m1_compact.h"
#include "m1_recovery.h"
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
  M1D1Intake *d1Intake=nullptr;
  std::function<void(const std::string &,bool)> d1Report;
  bool m1Preview=false;
  std::function<pc::M1Gravity()> m1Gravity;
  pc::M1Observation m1Observation;
  bool m1OneQuantumGrant=false;
  bool t899SlotSnapshot=false;
  std::function<void(pc::M1GravityDisplay)> gravityDisplay;
  uint32_t m1Kappa=24;
  bool m1StageTiming=false,g13TailTiming=false,m1InventoryV1=false,g13SealV1=false;
  bool m1AngleWitness=false;
  std::function<void(uint64_t)> suspendM1Hold;
  std::function<void(uint64_t,uint64_t,const M1Pulse &)> resumeM1Hold;
  std::function<bool(const SourceOffer &,const std::function<void()> &)> authorizeM1Offer;
  bool g13CompileTrace=false;
  bool m1Compact=false;
  bool m1Integrated=false,m1AuditBreakdown=false;
  std::wstring m1PrivateReference,m1PrivateOutput;
  bool m1PrivateObserve=false;
  std::function<uint64_t(pc::CompactView &)> presentedCompact;
  std::function<void(const SourceOffer &,uint32_t,bool)> compactSourceResult;
  bool g13SplitOp48=false;
  bool m1ParallelInventory=false;
  bool g13SealStages=false;
  std::string g13CycleScope;
#else
  NativeWindow *window;
#endif
};

using Publish = std::function<bool(const std::string &)>;
std::string RunBoot(const Attempt &attempt, const Publish &publish);
} // namespace grs::g13vk
