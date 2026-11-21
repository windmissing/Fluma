#include "source_claim.h"

using grs::g9rx::SourceClaim;
using grs::g9rx::SourceClaimGate;

static_assert(SourceClaimGate(true, true, true, true, true, 0) == SourceClaim::Claim);
static_assert(SourceClaimGate(true, true, true, true, true, 1) == SourceClaim::QuotaAlreadyPending);
static_assert(SourceClaimGate(true, true, true, true, false, 4) == SourceClaim::QuotaAlreadyPending);
static_assert(SourceClaimGate(true, false, true, true, true, 0) == SourceClaim::Wait);
static_assert(SourceClaimGate(true, true, false, true, true, 0) == SourceClaim::Wait);
static_assert(SourceClaimGate(true, true, true, false, true, 0) == SourceClaim::Wait);
static_assert(SourceClaimGate(true, true, true, true, false, 0) == SourceClaim::Wait);
static_assert(SourceClaimGate(false, true, true, true, true, 0) == SourceClaim::Wait);
