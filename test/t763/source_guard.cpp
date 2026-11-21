#include "source_guard.h"

using namespace grs::g9rx;

constexpr uint64_t begin = 20000000, deadline = begin + 250000000;
static_assert(SourceTimeGuard({true, true, begin + 2, begin + 1, begin, begin, deadline}) ==
              SourceReject::None);
static_assert(SourceTimeGuard({true, true, begin, begin + 1, begin, begin, deadline}) ==
              SourceReject::PulseFromFuture);
static_assert(SourceTimeGuard({true, true, deadline, deadline - 33333333, begin, begin, deadline}) ==
              SourceReject::None);
static_assert(SourceTimeGuard({true, true, deadline, deadline - 33333334, begin, begin, deadline}) ==
              SourceReject::PulseExpired);
static_assert(SourceTimeGuard({true, true, deadline + 1, deadline, begin, begin, deadline}) ==
              SourceReject::Deadline);
static_assert(SourceTimeGuard({true, true, begin, begin, begin + 1, begin, deadline}) ==
              SourceReject::CommandFromFuture);
static_assert(SourceTimeGuard({true, false, begin, 0, begin, begin, deadline}) ==
              SourceReject::MissingPulse);
