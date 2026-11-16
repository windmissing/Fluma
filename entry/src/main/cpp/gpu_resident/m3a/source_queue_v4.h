#pragma once
#include "progress_clock_v3.h"
namespace grs::m3a {
// Fixed producer storage. Every method is called with the same owner mutex as
// the clock and Run; it neither allocates nor calls GL or application callbacks.
class SourceQueueV4 {
    // Only the existing packet FIFO is stored here. The two intent words per
    // slot live in ProgressClockV3::intents and count toward its 1024B bound.
    std::array<SourcePacketV4,16> slots_{};
    size_t head_=0,count_=0;
    uint64_t headBoundary_=0;
public:
    size_t SizeLocked()const{return count_;}
    ControlResult EnqueueLocked(ProgressClockV3& clock,const ControlIdentityV4& identity,
                                const SourceIntentKeyV4& key,const SourcePacketV4& packet){
        if(identity!=clock.identity||!clock.open||!clock.ready)return ControlResult::STALE_REJECTED;
        const auto pair=[&](size_t at){return uint64_t(packet.words[at])|(uint64_t(packet.words[at+1])<<32);};
        if(pair(4)!=identity.run||pair(8)!=identity.window||pair(28)!=identity.config||!pair(6))return ControlResult::INVALID_ARGUMENT;
        for(size_t i=0;i<count_;++i){
            const auto& old=slots_[(head_+i)%slots_.size()].words;
            if(old[4]==packet.words[4]&&old[5]==packet.words[5]&&old[6]==packet.words[6]&&old[7]==packet.words[7])
                return old==packet.words?ControlResult::ALREADY_RETIRED:ControlResult::INVALID_ARGUMENT;
        }
        if(count_==slots_.size())return ControlResult::WOULD_BLOCK;
        const auto slot=(head_+count_)%slots_.size();
        // Register only after a real slot has been reserved. Register performs
        // all overflow checks transactionally; the following fixed-size copy
        // cannot fail. Thus queue admission and meaningfulInputSeq are atomic.
        const auto result=clock.RegisterSourceIntentLocked(key,packet,slot);
        if(result!=ControlResult::OK)return result;
        slots_[slot]=packet;if(count_==0)headBoundary_=clock.counters.sourceBoundary;++count_;
        return ControlResult::OK;
    }
    ControlResult PeekForPlanLocked(const ProgressClockV3& clock,const RunControl& run,
                                   const PlanGrantV3& grant,SourcePacketV4& out)const{
        if(clock.mode!=PlanningMode::RUNNABLE||!clock.open||!run.active||!run.sourcePhase||
           !clock.grant.id||grant.id!=clock.grant.id||grant.id!=run.plan)return ControlResult::WOULD_BLOCK;
        if(!count_||headBoundary_>grant.sourceBoundary)return ControlResult::WOULD_BLOCK;
        // An expired head is returned for explicit cancellation; never skip it
        // to execute a newer tail or manufacture another SOURCE opportunity.
        const auto& intent=clock.intents[head_];
        if(intent.generation!=clock.sourceControlGeneration||!intent.sequence)return ControlResult::STALE_REJECTED;
        out=slots_[head_];return ControlResult::OK;
    }
    ControlResult PopLocked(ProgressClockV3& clock,uint64_t generation,uint64_t sequence){
        if(!count_)return ControlResult::WOULD_BLOCK;
        const auto& key=clock.intents[head_];
        if(key.generation!=generation||key.sequence!=sequence)return ControlResult::STALE_REJECTED;
        slots_[head_]=SourcePacketV4{};clock.intents[head_]={};
        head_=(head_+1)%slots_.size();--count_;if(count_)++headBoundary_;
        return ControlResult::OK;
    }
};
static_assert(sizeof(SourceQueueV4)<=16*sizeof(SourcePacketV4)+32,"No duplicate unbudgeted FIFO identity/sidecar queue");
}
