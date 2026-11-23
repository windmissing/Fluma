#pragma once
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

namespace grs::pc {
struct CandidatePulse {
  uint64_t sequence=0,callbackQpc=0,receiptQpc=0,callbackNs=0,receiptNs=0;
};

inline constexpr uint64_t kCandidateGapNs=33333333, kCandidateJobNs=250000000, kPollNs=2000000;

inline uint64_t CandidatePollNs(uint64_t now,uint64_t deadline) {
  return now<deadline ? std::min(kPollNs,deadline-now) : 0;
}

inline const char *CandidatePulseOrder(const CandidatePulse &prev,const CandidatePulse &next) {
  if (!next.sequence || next.receiptQpc<next.callbackQpc || next.receiptNs<next.callbackNs)
    return "PULSE_REVERSED";
  if (!prev.sequence) return nullptr;
  if (next.sequence!=prev.sequence+1) return "PULSE_SEQUENCE_GAP";
  if (next.callbackNs<=prev.callbackNs || next.receiptNs<=prev.receiptNs) return "PULSE_REVERSED";
  if (next.callbackNs-prev.callbackNs>kCandidateGapNs || next.receiptNs-prev.receiptNs>kCandidateGapNs)
    return "PULSE_GAP";
  return nullptr;
}

inline const char *CandidatePulseFreshness(const CandidatePulse &last,uint64_t now) {
  if (!last.sequence) return "PULSE_MISSING";
  if (now<last.receiptNs) return "PULSE_REVERSED";
  return now-last.receiptNs>kCandidateGapNs ? "PULSE_STALE" : nullptr;
}

struct CandidateTiming {
  struct Wait { uint64_t enterQpc=0,exitQpc=0,timeoutNs=0; int result=1; };
  std::array<Wait,128> polls{};
  Wait last,retire;
  size_t count=0;
  uint64_t stopQpc=0;
  const char *stop=nullptr;
  void AddPoll(Wait wait) {
    last=wait;
    if (count<polls.size()) polls[count]=wait;
    ++count;
  }
  void Stop(const char *cause,uint64_t qpc) {
    if (!stop) { stop=cause; stopQpc=qpc; }
  }
};
}
