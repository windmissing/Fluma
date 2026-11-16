#pragma once
#include "progress_clock_v3.h"
namespace grs::g9spf {
struct QuotaInspectionV3 {
    ControlResult result=ControlResult::INVALID_ARGUMENT;
    uint64_t generated=0,accepted=0,blocked=0,creditedElapsedNs=0;
};
QuotaInspectionV3 InspectQuotaSnapshotV3(const QuotaSnapshotV3&);
}
