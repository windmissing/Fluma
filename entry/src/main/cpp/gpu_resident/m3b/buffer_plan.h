#pragma once
#include "work_layout.h"
#include "../m2/buffer_plan.h"
namespace grs::m3b {
struct ResidentBuffersV5 {
    // Baseline objects retain their documented allocation; old unused ranges
    // are not deducted from the conservative payload ledger.
    std::array<size_t,28> bytes{};
    size_t total=0,padding=0;
    static bool Make(const Profile& profile,size_t alignment,ResidentBuffersV5& out){
        WorkLayout layout;if(!WorkLayout::Make(profile.k,profile.kw,alignment,layout))return false;
        const m2::Profile* old=nullptr;for(const auto& p:m2::kProfiles)if(p.k==profile.k&&p.kw==profile.kw)old=&p;
        if(!old)return false;
        ResidentBuffersV5 next;auto baseline=m2::ResidentBufferBytes(*old);
        for(size_t i=0;i<baseline.size();++i)next.bytes[i]=baseline[i];
        for(size_t i=0;i<3;++i)next.bytes[i]=profile.stateBytes;
        // Existing three quantum frame arenas grow by the new 255-row bound.
        for(size_t i=8;i<=10;++i)next.bytes[i]=size_t(profile.quantumFrameRows)*256+size_t(SelectLineageProfile(profile.k,profile.kw)->ea)*4*128;
        next.bytes[15]=profile.sourceWorkBytes;next.bytes[16]=profile.quantumWorkBytes;
        next.bytes[20]=next.bytes[21]=layout.combinedBytes;
        // Third semantic stream is the expected segment already appended to
        // QUANTUM WORK, not an additional separately allocated object.
        for(size_t i=22;i<25;++i)next.bytes[i]=size_t(profile.sourceRows)*512;
        for(size_t i=25;i<28;++i)next.bytes[i]=8192;
        for(auto n:next.bytes){if(!n||n>std::numeric_limits<size_t>::max()-next.total)return false;next.total+=n;}
        next.padding=2*(layout.combinedEventOffset-profile.stateBytes);
        if(next.total!=profile.payloadBytes+next.padding)return false;
        out=next;return true;
    }
};
}
