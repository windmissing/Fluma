#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include "lineage_schema.h"
namespace grs::m3b {
inline constexpr uint32_t kQ=1048576,kChannelBitmap=15;
inline constexpr uint32_t kColumnCapacity=268435456,kSourcePasses=72;
inline constexpr const char* kDesignSha=kEngineeringSha;
namespace sourceMeta32 {
inline constexpr size_t sourceKind=0;
inline constexpr size_t lambdaQ16=1;
inline constexpr size_t attemptCount=2;
inline constexpr size_t seedCount=3;
inline constexpr size_t birthCount=4;
inline constexpr size_t mergeCount=5;
inline constexpr size_t admittedUnits=6;
inline constexpr size_t requestedUnits=7;
inline constexpr size_t sourceCommitLo=8;
inline constexpr size_t sourceCommitHi=9;
inline constexpr size_t physicsCommitLo=10;
inline constexpr size_t physicsCommitHi=11;
inline constexpr size_t payloadRow=12;
inline constexpr size_t payloadWords=13;
inline constexpr size_t sourceId=14;
inline constexpr size_t requestSeq=15;
inline constexpr size_t firstId=16;
inline constexpr size_t nextIdBefore=17;
inline constexpr size_t nextIdAfter=18;
inline constexpr size_t etaHi=19;
inline constexpr size_t etaResidual=20;
inline constexpr size_t yRawLo=21;
inline constexpr size_t yRawHi=22;
inline constexpr size_t waterBeforeLo=23;
inline constexpr size_t waterBeforeHi=24;
inline constexpr size_t stopCode=25;
inline constexpr size_t requestLo=26;
inline constexpr size_t requestHi=27;
inline constexpr size_t reserved28=28;
inline constexpr size_t reserved29=29;
inline constexpr size_t reserved30=30;
inline constexpr size_t reserved31=31;
}
namespace roles {
inline constexpr uint32_t ROOT=0;
inline constexpr uint32_t REQUESTED=1;
inline constexpr uint32_t CLAMPED=2;
inline constexpr uint32_t OLD=3;
inline constexpr uint32_t AUTHORITATIVE=4;
inline constexpr uint32_t CONSISTENCY=5;
inline constexpr uint32_t SOURCE_TRIAL=6;
}
namespace sourceStopCodes {
inline constexpr uint32_t COMPLETE=0;
inline constexpr uint32_t PARTIAL_LAMBDA=1;
inline constexpr uint32_t CAPACITY_BLOCKED=2;
inline constexpr uint32_t NO_WATER=3;
inline constexpr uint32_t NUMERIC_RESOLUTION=4;
inline constexpr uint32_t INPUT_REJECTED=5;
inline constexpr uint32_t DEDUP_REPLAY=6;
}
namespace trialReasons {
inline constexpr uint32_t SUCCESS=0;
inline constexpr uint32_t CAP_BOUND_RETRY=1;
inline constexpr uint32_t ETA_CAPACITY_RETRY=2;
inline constexpr uint32_t ETA_TRUNCATION_RETRY=3;
inline constexpr uint32_t SEED_NO_PAIR_RETRY=4;
inline constexpr uint32_t BIRTH_LIMIT_RETRY=5;
inline constexpr uint32_t ID_LIMIT_RETRY=6;
inline constexpr uint32_t SEED_GEOMETRY_RETRY=7;
inline constexpr uint32_t NOT_EXECUTED=8;
}
namespace fatalReasons {
inline constexpr uint32_t M3_INPUT_SCHEMA=27;
inline constexpr uint32_t M3_SCOPE_IDENTITY=28;
inline constexpr uint32_t M3_PLANNER_MISMATCH=29;
inline constexpr uint32_t M3_WATER_PROJECTION=30;
inline constexpr uint32_t M3_TRIAL_REPLAY=31;
inline constexpr uint32_t M3_WORK_CREDIT=32;
inline constexpr uint32_t M3_PREEXISTING_HARD_GUARD=33;
}
namespace plannerStatuses {
inline constexpr uint32_t ACCEPTED=0;
inline constexpr uint32_t CLAMPED=1;
inline constexpr uint32_t HELD=2;
inline constexpr uint32_t BLOCKED=3;
inline constexpr uint32_t NOT_EXECUTED=4;
}
namespace rootOutcomes {
inline constexpr uint32_t SUCCESS=1;
inline constexpr uint32_t METADATA_REJECT=2;
inline constexpr uint32_t DEDUP_REPLAY=3;
inline constexpr uint32_t FATAL_ERROR=4;
inline constexpr uint32_t PLANNING_BLOCKED=5;
}
namespace auditFamilies {
inline constexpr uint32_t stockCapacity=0;
inline constexpr uint32_t globalConservation=1;
inline constexpr uint32_t idsGeneration=2;
inline constexpr uint32_t sourceProvenance=3;
inline constexpr uint32_t budgets=4;
inline constexpr uint32_t hardGuard=5;
inline constexpr uint32_t eventSchemaOrder=6;
inline constexpr uint32_t replay=7;
inline constexpr uint32_t publicationQualification=8;
inline constexpr uint32_t reservedMetadata=9;
inline constexpr uint32_t waterProjection=10;
inline constexpr uint32_t lambdaSeeds=11;
inline constexpr uint32_t branchPurity=12;
inline constexpr uint32_t plannerConsistency=13;
inline constexpr uint32_t rawPayloadIdentity=14;
}
namespace sourceEvent128 {
inline constexpr uint32_t magic=876950349u;
inline constexpr uint32_t revision=4u;
inline constexpr uint32_t seal=3235774468u;
inline constexpr uint32_t MERGE=15;
inline constexpr std::array<uint32_t,4> MERGEMask={4160749567u,4294967295u,4294967295u,255u};
inline constexpr uint32_t BIRTH=16;
inline constexpr std::array<uint32_t,4> BIRTHMask={4160684031u,65535u,65535u,255u};
inline constexpr uint32_t DIRECT=17;
inline constexpr std::array<uint32_t,4> DIRECTMask={4160651263u,0u,0u,255u};
inline constexpr uint32_t END=18;
inline constexpr std::array<uint32_t,4> ENDMask={4269555711u,0u,0u,255u};
inline constexpr uint32_t ETA_COLUMN=20;
inline constexpr std::array<uint32_t,4> ETA_COLUMNMask={3456270335u,2047u,0u,255u};
inline constexpr uint32_t SEED_BEGIN=21;
inline constexpr std::array<uint32_t,4> SEED_BEGINMask={3456532479u,1536u,0u,255u};
inline constexpr uint32_t TRIAL_END=22;
inline constexpr std::array<uint32_t,4> TRIAL_ENDMask={3464265727u,63488u,0u,255u};
inline constexpr uint32_t TRIAL_BEGIN=23;
inline constexpr std::array<uint32_t,4> TRIAL_BEGINMask={3456139263u,6144u,0u,255u};
}
namespace quantumEvent64 {
inline constexpr uint32_t revision=4u;
}
namespace control128 {
inline constexpr uint32_t magic=876825680u;
inline constexpr uint32_t revision=4u;
inline constexpr uint32_t seal=3235774468u;
inline constexpr uint32_t PREVIEW_DECISION=24;
inline constexpr uint32_t PLANNER_END=25;
}
}
