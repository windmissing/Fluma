#pragma once
#include "progress_clock_v3.h"
namespace grs::m3b {
// Fixed producer storage. Every method is called with the same owner mutex as
// the clock and Run; it neither allocates nor calls GL or application callbacks.
class SourceQueueV5 {
public:
    struct Stats {uint64_t offered=0,enqueued=0,backpressured=0,replayed=0,rejected=0;};
private:
    // Only the existing packet FIFO is stored here. The two intent words per
    // slot live in ProgressClockV3::intents and count toward its 1024B bound.
    std::array<SourcePacketV5,16> slots_{};
    size_t head_=0,count_=0;
    uint64_t headBoundary_=0;
    Stats stats_;
public:
    Stats StatsLocked()const{return stats_;}
    size_t SizeLocked()const{return count_;}
    ProgressClockV3::Intent FrontIntentLocked(const ProgressClockV3& clock)const{return count_?clock.intents[head_]:ProgressClockV3::Intent{};}
    ControlResult EnqueueLocked(ProgressClockV3& clock,const ControlIdentityV5& identity,
                                const SourceIntentKeyV5& key,const SourcePacketV5& packet){
        if(stats_.offered==UINT64_MAX||stats_.enqueued==UINT64_MAX||stats_.backpressured==UINT64_MAX||stats_.replayed==UINT64_MAX||stats_.rejected==UINT64_MAX){clock.fault=ControlResult::QUOTA_OVERFLOW;return ControlResult::QUOTA_OVERFLOW;}
        ++stats_.offered;
        if(identity!=clock.identity||!clock.open||!clock.ready){++stats_.rejected;return ControlResult::STALE_REJECTED;}
        const auto pair=[&](size_t at){return uint64_t(packet.words[at])|(uint64_t(packet.words[at+1])<<32);};
        if(pair(4)!=identity.run||pair(8)!=identity.window||pair(28)!=identity.config||!pair(6)){++stats_.rejected;return ControlResult::INVALID_ARGUMENT;}
        for(size_t i=0;i<count_;++i){
            const auto& old=slots_[(head_+i)%slots_.size()].words;
            if(old[4]==packet.words[4]&&old[5]==packet.words[5]&&old[6]==packet.words[6]&&old[7]==packet.words[7]){
                if(old==packet.words){++stats_.replayed;return ControlResult::ALREADY_RETIRED;}
                ++stats_.rejected;return ControlResult::INVALID_ARGUMENT;
            }
        }
        if(count_==slots_.size()){++stats_.backpressured;return ControlResult::WOULD_BLOCK;}
        const auto slot=(head_+count_)%slots_.size();
        // Register only after a real slot has been reserved. Register performs
        // all overflow checks transactionally; the following fixed-size copy
        // cannot fail. Thus queue admission and meaningfulInputSeq are atomic.
        const auto result=clock.RegisterSourceIntentLocked(key,packet,slot);
        if(result!=ControlResult::OK){if(result==ControlResult::ALREADY_RETIRED)++stats_.replayed;else ++stats_.rejected;return result;}
        slots_[slot]=packet;if(count_==0)headBoundary_=clock.counters.sourceBoundary;++count_;
        ++stats_.enqueued;
        return ControlResult::OK;
    }
    ControlResult PeekForPlanLocked(const ProgressClockV3& clock,const RunControl& run,
                                   const PlanGrantV3& grant,SourcePacketV5& out)const{
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
        slots_[head_]=SourcePacketV5{};clock.intents[head_]={};
        head_=(head_+1)%slots_.size();--count_;if(count_)++headBoundary_;
        return ControlResult::OK;
    }
};
static_assert(sizeof(SourceQueueV5)<=16*sizeof(SourcePacketV5)+72,"No duplicate unbudgeted FIFO identity/sidecar queue");
static_assert(sizeof(ProgressClockV3)+sizeof(SourceQueueV5)-16*sizeof(SourcePacketV5)<=1024,"V3 clock, FIFO sidecars and bounded producer counters fit the new control budget");
}
