#pragma once
#include "control_types.h"
namespace grs::m3b {
struct PublicationTokenV5 {
    ControlIdentityV5 identity;
    uint64_t generation=0,source=0,physics=0,lease=0;
    uint32_t revision=5,features=31,slot=0,bank=0;
};
// Owner-lock metadata only. The GL transport owns each producer/release fence.
// A leased slot is unavailable for another root until its release fence retires.
class PublicationLeasesV5 {
    struct Slot {PublicationTokenV5 token;bool published=false,pinned=false,releasePending=false;};
    std::array<Slot,2> slots_{};
    uint64_t serial_=0;
public:
    bool WritableLocked(uint32_t slot)const{return slot<2&&!slots_[slot].pinned&&!slots_[slot].releasePending;}
    bool PublishLocked(PublicationTokenV5 token){
        if(token.revision!=5||token.features!=31||!token.generation||token.slot>=2||token.bank>=2||!WritableLocked(token.slot))return false;
        auto& slot=slots_[token.slot];if(slot.published&&slot.token.identity==token.identity&&token.generation<=slot.token.generation)return false;
        token.lease=0;slot={token,true,false,false};return true;
    }
    ControlResult AcquireLocked(const ControlIdentityV5& identity,uint32_t index,uint64_t generation,PublicationTokenV5& out){
        if(index>=2)return ControlResult::INVALID_ARGUMENT;
        auto& slot=slots_[index];
        if(!slot.published||slot.token.identity!=identity||slot.token.generation!=generation)return ControlResult::STALE_REJECTED;
        if(slot.pinned||slot.releasePending)return ControlResult::WOULD_BLOCK;
        if(serial_==UINT64_MAX)return ControlResult::QUOTA_OVERFLOW;
        slot.token.lease=++serial_;slot.pinned=true;out=slot.token;return ControlResult::OK;
    }
    ControlResult ReleaseLocked(const PublicationTokenV5& token){
        if(token.revision!=5||token.features!=31||token.slot>=2||!token.lease)return ControlResult::INVALID_ARGUMENT;
        auto& slot=slots_[token.slot];const auto& live=slot.token;
        if(!slot.pinned||token.identity!=live.identity||token.generation!=live.generation||token.lease!=live.lease||token.bank!=live.bank||token.source!=live.source||token.physics!=live.physics)return ControlResult::STALE_REJECTED;
        slot.pinned=false;slot.releasePending=true;return ControlResult::OK;
    }
    ControlResult RetireReleaseFenceLocked(const PublicationTokenV5& token){
        if(token.slot>=2)return ControlResult::INVALID_ARGUMENT;
        auto& slot=slots_[token.slot];
        if(!slot.releasePending||token.identity!=slot.token.identity||token.lease!=slot.token.lease||token.generation!=slot.token.generation)return ControlResult::STALE_REJECTED;
        slot.releasePending=false;slot.token.lease=0;return ControlResult::OK;
    }
    void ContextLostLocked(){slots_={};}
};
}
