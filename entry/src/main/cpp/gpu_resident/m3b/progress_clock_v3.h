#pragma once
#include "control_types.h"
#include "quota_fields_generated.h"
namespace grs::m3b {
struct QuotaCounters {
    uint64_t session=0,pulseSequence=0,uiNs=0,receiveNs=0,pulses=0,maxNativeGap=0,maxUiGap=0;
    uint64_t generated=0,accepted=0,overload=0,revoked=0,carry=0,clearedCarry=0,planSerial=0;
    uint64_t blockedActive=0,blockedPending=0,blockedWait=0,blockedClearedCarry=0,blockedEpisodes=0,blockedPulses=0;
    uint64_t creditedElapsedNs=0,blockedWaitNs=0,meaningfulInputSeq=0,blockedOnInputSeq=0,rearmInputSeq=0,blockedOnPulseSeq=0,rearmAfterPulseSeq=0;
    uint64_t retiredPlan=0,retiredNonce=0,blockedBasePhysics=0,sourceIntentSeq=0,sourceBoundary=0;
};
class ProgressClockV3 {
public:
    static constexpr uint64_t GapNs=33333333,RootNs=250000000,Denominator=1000000000;
    QuotaCounters counters;
    ControlIdentityV5 identity,retiredIdentity;
    PlanGrantV3 grant;
    PlanningMode mode=PlanningMode::RUNNABLE;
    GravityMode gravityMode=GravityMode::MANUAL;
    ControlResult fault=ControlResult::OK;
    uint64_t sourceControlGeneration=1;
    uint32_t pending=0,activeRemaining=0,activeRootKind=0,blockReason=0;
    bool open=false,ready=false,baseline=false,hasPulse=false,jobLive=false,lastGoodStale=false;
    bool uiFresh=false,gravityFresh=false;
    int32_t target=0;
    // Sixteen existing producer slots receive only this intent sidecar.
    struct Intent {uint64_t generation=0,sequence=0;};
    std::array<Intent,16> intents{};

    ControlResult PulseLocked(const PagePulseV3&,uint64_t nativeNow);
    ControlResult RegisterTargetLocked(const ControlIdentityV5&,int32_t effectiveIndex);
    ControlResult RegisterModeLocked(const ControlIdentityV5&,GravityMode);
    ControlResult RevokeLocked(ControlFault);
    ControlResult BeginSessionLocked(const RecoveryHandoff&);
    ControlResult ResetRunLocked(const RetiredRunHandoff&,const RunControl&);
    bool Fresh(uint64_t now)const;
    bool CanClaim(uint64_t now,bool hasSource,const RunControl&)const;
    bool ClaimLocked(uint64_t now,bool hasSource,RunControl&,PlanGrantV3&);
    ControlResult RetireSourcePlanLocked(uint64_t plan,RunControl&);
    ControlResult RegisterSourceIntentLocked(const SourceIntentKeyV5&,const SourcePacketV5&,size_t freeSlot);
    QuotaSnapshotV3 SnapshotLocked(uint64_t now,bool hasSource,const RunControl&)const;
    bool Balanced()const;
    // Gate-created tails are consumed by PlanningControl, which coordinates Run.
private:
    ControlResult MeaningfulInput();
    ControlResult Overflow();
};
static_assert(sizeof(ProgressClockV3)<=1024,"T498 V3 native control state budget");
}
