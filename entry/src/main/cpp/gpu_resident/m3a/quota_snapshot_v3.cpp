#include "progress_clock_v3.h"
namespace grs::m3a {
QuotaSnapshotV3 ProgressClockV3::SnapshotLocked(uint64_t now,bool source,const RunControl& run)const{
    QuotaSnapshotV3 out{};
    auto pair=[&](size_t at,uint64_t value){out[at]=uint32_t(value);out[at+1]=uint32_t(value>>32);};
    out[0]=860963661;out[1]=3;out[2]=256;out[3]=0xc0de0003;out[4]=uint32_t(mode);
    const bool canClaim=CanClaim(now,source,run);
    const bool canSubmit=mode==PlanningMode::RUNNABLE&&ready&&baseline&&Fresh(now)&&grant.id&&run.active;
    out[5]=uint32_t(open)|(uint32_t(ready)<<1)|(uint32_t(baseline)<<2)|(uint32_t(hasPulse)<<3)|(uint32_t(jobLive)<<4)|(uint32_t(canClaim)<<5)|(uint32_t(canSubmit)<<6)|(uint32_t(lastGoodStale)<<7);
    out[6]=pending;out[7]=activeRemaining;out[8]=activeRootKind;out[9]=counters.retiredNonce?2:0;out[10]=blockReason;
    const auto& c=counters;
    const uint64_t values[]={c.session,c.pulseSequence,c.uiNs,c.receiveNs,c.pulses,c.maxNativeGap,c.maxUiGap,c.generated,c.accepted,c.overload,c.revoked,c.carry,c.clearedCarry,c.planSerial,c.blockedActive,c.blockedPending,c.blockedWait,c.blockedActive+c.blockedPending+c.blockedWait,c.blockedClearedCarry,c.blockedEpisodes,c.blockedPulses,c.creditedElapsedNs,c.blockedWaitNs,c.meaningfulInputSeq,c.blockedOnInputSeq,c.rearmInputSeq,c.blockedOnPulseSeq,c.rearmAfterPulseSeq,grant.id,c.retiredPlan,c.retiredNonce,c.blockedBasePhysics,identity.run,identity.owner,identity.context,identity.inputEpoch,identity.pageSession,c.sourceIntentSeq,c.sourceBoundary,identity.permission,grant.frozenInputSeq,uint64_t(int64_t(grant.target))};
    static_assert(sizeof(values)/sizeof(values[0])==42,"T508 snapshot full U64 fields");
    for(size_t i=0;i<42;++i)pair(16+i*2,values[i]);return out;
}
}
