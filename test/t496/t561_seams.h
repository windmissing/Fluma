// T561 (problem T547 §3, manifest highLevelSeams): the 14 high-level cold
// observer seams for the frozen-tree Create chain. Developer provides the seam
// emitters; the tester harness assembles them (this header is included only
// by test/t496/external_source_diff.cpp, never by production TUs).
//
// Assembly map (per manifest):
//  TRANSPORT_CREATE  — frozen app_bridge bootNote "TRANSPORT_CREATE_ENTER/EXIT"
//                      (T456 boot probe); the GL layer beneath is the forward
//                      sink. The harness cannot insert code inside loop(), so
//                      this seam records the observed transition with the
//                      actual code/firstGl/allocatedBytes carried by the
//                      bootNote arguments at the next poll.
//  SOURCE_CREATE / INTERMEDIATE_CREATE / FINAL_BUILD / REFLECT_PREPARE /
//  COLD_PREFIX / SCENE_PREPARE / SCENE_ROLE_BUILD — same bootNote/observer
//                      mapping (TRANSPORT_CAPABILITY, TRANSPORT_BUFFER,
//                      TAIL_OBSERVER, SCENE_PROGRAM_PREPARE_EXIT stages).
//  RESOURCE_COMMIT   — recovery.Resources() result observed via
//                      a.v2ResourcesReady at the harness poll boundary.
//  CAPTURE_EPOCH     — a.captureCommittedEpoch transition.
//  SESSION_HANDOFF   — harness wrapper around RecoverySessionLocked.
//  FIRST_PAGE_PULSE  — harness wrapper around PagePulseLocked.
//  READY             — harness wrapper around TryReadyV2Locked.
//  RETIRE_RETRY      — the harness does not drive retire; recorded as
//                      observation only when phase reflects FAULT/retire.
// Observation records use event="SEAM" with the seam id so they are never
// confused with durable ENTER/EXIT proof pairs.
#pragma once
#include "gl_sink.inc"
namespace t561seams {
using t496sink::SeamEnter;
using t496sink::SeamExit;

inline uint64_t Enter(uint32_t fid, const std::string& detail) { return t496sink::SeamEnter(fid, detail); }
inline bool Exit(uint64_t seq, uint32_t fid, const std::string& result) { return t496sink::SeamExit(seq, fid, result); }

// Observation record (no paired ENTER/EXIT claim).
inline void Observe(uint32_t fid, const std::string& detail) {
  t496sink::EmitRecord(fid, "SEAM", t496sink::CallSeq().fetch_add(1), detail);
}

// RAII seam guard for the harness-wrappable public App methods.
struct Guard {
  uint64_t seq; uint32_t fid;
  Guard(uint32_t f, const std::string& detail) : seq(SeamEnter(f, detail)), fid(f) {}
  ~Guard() { if (seq) SeamExit(seq, fid, "returned"); }
  Guard(const Guard&) = delete;
  Guard& operator=(const Guard&) = delete;
};
} // namespace t561seams
