#pragma once
#include "research_config.h"

namespace grs::g9rx {
struct SourceReceipt {
  struct PassReceipt {
    uint32_t op = 0, role = 0, j = 0, micro = 0;
    std::array<uint32_t,4> objects{};
  };
  Permit permit{};
  std::array<PassReceipt,12> passes{};
  Completion256 tail{};
  std::array<uint32_t,8> expected{};
  uint64_t ownerTid = 0;
  uintptr_t eglContext = 0;
  uint64_t fenceNs = 0, commitNs = 0, generation = 0, command = 0, intent = 0;
  uint32_t bootProgram = 0, program = 0, currentProgram = 0, work = 0, workOffset = 0;
  uint32_t dispatches = 0, barriers = 0, copies = 0, fencePolls = 0, rawWait = 0;
  uint32_t gl = 0, egl = 0, queueBefore = 0, queueAfter = 0, pending = 0, activeRemaining = 0;
  uint32_t candidateObject = 0, committedObject = 0, fault = 0;
  bool fenceCreated = false, mapReturned = false, unmapped = false, mapped = false;
  bool typed = false, retiredPlan = false, committed = false;
};
} // namespace grs::g9rx
