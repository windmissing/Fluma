#pragma once
#include "wire.h"
namespace grs::g9boot { struct Capture; }

namespace grs::g9dm {
// Owner-thread observations of the one actual SOURCE flight. These values
// are exported after execution and cannot serve as inputs to the proof gate.
struct SourceTrace {
  const g9boot::Capture *bootCapture = nullptr;
  struct Dispatch {
    uint32_t op = 0, role = 0, j = 0, micro = 0;
    std::array<uint32_t, 4> objects{};
  };
  std::array<Dispatch, 12> passes{};
  Permit permit{};
  Input128 input{}, config{};
  Completion256 completion{};
  Digest expected{};
  uint64_t fenceNs = 0, commitNs = 0, commit = 0, quanta = 0, generation = 0;
  uintptr_t eglContext = 0, eglDisplay = 0, eglSurface = 0;
  uint32_t bootProgram = 0, program = 0, workObject = 0, workOffset = 0;
  uint32_t dispatches = 0, barriers = 0, copyBarriers = 0, fencePolls = 0, fenceWait = 0;
  uint32_t gl = 0, egl = 0, firstGl = 0, firstEgl = 0, queueBefore = 0, queueAfter = 0;
  uint32_t pendingAtClaim = 0, activeRemaining = 0, pendingAfter = 0, coldGl = 0, coldEgl = 0;
  uint32_t fault = 0;
  uint32_t uploadProgram = 0, baseObject = 0, candidateObject = 0, committedObject = 0;
  bool bootAudited = false, uploaded = false, fenceCreated = false, mapReturned = false, mapped = false;
  uint32_t unmapRaw = 0;
  bool typed = false, preflight = false, planRetired = false, committed = false;
};
SourceTrace *SetSourceTrace(SourceTrace *trace) noexcept;
SourceTrace *CurrentSourceTrace() noexcept;
const char *SourceRoleSetSHA();
} // namespace grs::g9dm
