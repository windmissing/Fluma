#pragma once
#include "profiles_generated.h"
#include <limits>
namespace grs::m3b {
struct WorkLayout {
    const Profile* profile=nullptr;
    size_t combinedEventOffset=0,combinedBytes=0;
    static bool Make(uint32_t k,uint32_t kw,size_t alignment,WorkLayout& out){
        const auto* p=SelectProfile(k,kw);
        if(!p||alignment==0||alignment%4!=0)return false;
        const auto remainder=p->stateBytes%alignment;
        const auto padding=remainder?alignment-remainder:0;
        if(padding>std::numeric_limits<size_t>::max()-p->stateBytes)return false;
        const auto eventOffset=p->stateBytes+padding;
        const auto* v5=SelectLineageProfile(k,kw);if(!v5)return false;const size_t events=size_t(p->scopeRows)*256+size_t(v5->ea)*128;
        if(events>std::numeric_limits<size_t>::max()-eventOffset)return false;
        out={p,eventOffset,eventOffset+events};return true;
    }
    bool Supports(size_t maximumBlockBytes,uint32_t storageBindings,uint32_t computeBlocks,uint32_t fragmentBlocks,uint32_t workgroupInvocations,uint32_t sharedBytes)const{
        if(!profile||storageBindings<4||computeBlocks<4||fragmentBlocks<1||workgroupInvocations<64||sharedBytes<4096)return false;
        return combinedBytes<=maximumBlockBytes&&profile->sourceWorkBytes<=maximumBlockBytes&&profile->quantumWorkBytes<=maximumBlockBytes&&size_t(profile->sourceRows)*512<=maximumBlockBytes&&size_t(profile->quantumFrameRows)*256+size_t(SelectLineageProfile(profile->k,profile->kw)->ea)*4*128<=maximumBlockBytes;
    }
};
}
