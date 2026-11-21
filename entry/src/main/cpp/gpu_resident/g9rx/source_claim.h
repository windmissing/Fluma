#pragma once
#include <cstdint>

namespace grs::g9rx {
enum class SourceClaim { Wait, QuotaAlreadyPending, Claim };

constexpr SourceClaim SourceClaimGate(bool first, bool baseline, bool fresh,
                                     bool neutral, bool sourceAtHead,
                                     uint32_t pending) {
  if (!first || !baseline || !fresh)
    return SourceClaim::Wait;
  if (pending)
    return SourceClaim::QuotaAlreadyPending;
  return neutral && sourceAtHead ? SourceClaim::Claim : SourceClaim::Wait;
}
} // namespace grs::g9rx
