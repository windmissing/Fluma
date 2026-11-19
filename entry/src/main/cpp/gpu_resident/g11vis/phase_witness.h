#pragma once
#include "bootstrap.h"
#include "g11vis_pipeline.h"
namespace grs::g11vis {
// One optional reservation. It borrows no ROOT/FULL slot and never adds a
// dispatch/fence.
class PhaseWitness {
  uint64_t ticket_ = 0, context_ = 0, nonce_ = 0, created_ = 0;
  uint32_t micro_ = 0, phase_ = 0, role_ = 0;
  GLuint gpu_ = 0;
  std::unique_ptr<uint8_t[]> native_;
  size_t bytes_ = 0, done_ = 0;
  uint32_t copies_ = 0, maps_ = 0, rawGl_ = 0;
  bool old_ = false, captured_ = false, retired_ = false, reader_ = false,
       invalid_ = false;
  const char *state_ = "EMPTY";
  p011gpu::Sha256 hash_;
  Digest digest_{};
  bool Good();
  bool Copy(GLuint, size_t, size_t, size_t);

public:
  bool Arm(const Scope &, const ParentAdapter &, const Words64 &,
           const Digest &pipeline, const VisualSeedReceipt &, uint64_t context,
           uint64_t nonce, uint32_t micro, uint32_t phase, uint32_t role,
           const VisualResources &, uint64_t now);
  bool After(const G11Pass &, uint64_t nonce, GLuint work,
             const VisualResources &, uint32_t markerBank);
  void RootRetired(uint64_t nonce, const accept647::Tail *, bool accepted);
  bool Pump(
      uint64_t now); // only after the existing root fence has actually signaled
  std::string State() const;
  std::string Page(uint32_t wordOffset, uint64_t now);
  void CancelUnsubmitted() {
    if (ticket_ && !copies_)
      retired_ = true;
  }
  bool End(bool current, bool actualContextRetired = false);
  void Invalidate() { invalid_ = true; }
  bool NeedsPump() const {
    return ticket_ && retired_ && captured_ && !invalid_ && done_ < bytes_;
  }
  bool Reserved() const { return ticket_ != 0; }
  bool Capturing() const { return ticket_ && !retired_; }
};
} // namespace grs::g11vis
