#include "completion_gate.h"
namespace grs::m3a {
bool CompletionGate::Common(const Profile& profile,const RootPermitV4& p,const Words256& w,const NativeRootEvidence& evidence){
    if(p.kind<1||p.kind>2||!p.plan||!p.nonce||!p.control||!p.frame||p.baseBank>=5||p.candidateBank>=5||p.baseBank==p.candidateBank||p.eventArena>1||p.publicationSlot>1)return false;
    if(evidence.gl||evidence.egl||!evidence.fenceSignalledNs||evidence.fenceSignalledNs>p.deadline)return false;
    const uint32_t requiredPasses=p.kind==1?72:uint32_t(profile.passCount);
    if(w[0]!=1381192263||w[1]!=4||w[2]!=256||w[3]!=3235823838u||w[27]!=p.kind||w[26]!=p.ordinal)return false;
    if(w[42]!=1||w[43]!=1||w[44]!=1||w[45]!=3||w[46]!=4||w[47]!=4||w[48]!=4||w[49]!=15||w[50])return false;
    if(w[97]!=requiredPasses||w[98]||w[99]!=0x7fff||w[100]||w[30]||w[31]||w[104]||w[105]||w[113])return false;
    if(w[110]!=requiredPasses||evidence.submittedPasses!=requiredPasses||w[111]!=evidence.barriers||w[112]!=p.uploadedBytes)return false;
    if(Pair(w,4)!=p.nonce||Pair(w,6)!=p.identity.run||Pair(w,8)!=p.identity.owner||Pair(w,10)!=p.identity.context||Pair(w,12)!=p.identity.surface)return false;
    if(Pair(w,14)!=p.sourceCommit||Pair(w,16)!=p.physicsCommit||Pair(w,22)!=p.control||Pair(w,24)!=p.frame||Pair(w,38)!=p.geometry||Pair(w,40)!=p.identity.config)return false;
    if(w[32]!=p.baseBank||w[33]!=p.candidateBank||w[35]!=p.publicationSlot||w[88]>profile.k||w[89]!=profile.k-w[88]||w[90]>profile.kw||w[91]!=profile.kw-w[90])return false;
    for(size_t i=0;i<8;++i)if(w[114+i]!=p.shaderSha[i]||w[122+i]!=p.angleSha[i]||w[130+i]!=p.layoutSha[i])return false;
    for(size_t i=189;i<256;++i)if(w[i])return false;
    if(w[173]!=1||w[174]!=7||w[184]!=512||w[188]!=1)return false;
    if(p.kind==1){if(p.ordinal||w[168]!=128||w[169]>profile.sourceRows||!w[169]||w[172]!=p.eventArena)return false;}
    else if(!p.ordinal||p.ordinal>4||w[51]!=p.eventArena||w[52]!=p.eventStart||w[53]<w[52]||w[53]>p.eventCapacity||w[54]!=p.eventCapacity)return false;
    return true;
}
std::optional<ValidatedBlockedTailV4> CompletionGate::Blocked(const Profile& profile,const RootPermitV4& p,const Words256& w,const NativeRootEvidence& evidence){
    if(!Common(profile,p,w,evidence)||p.kind!=2||w[28]!=5||Pair(w,18)!=p.sourceCommit||Pair(w,20)!=p.physicsCommit||w[178]!=0||w[179]!=plannerStatuses::BLOCKED)return {};
    // PLANNER_END is checked word-for-word by the GPU root audit (required
    // bit13). Its selected/status are echoed in the frozen completion fields.
    // Native acceptance consumes that single 1024B seal, never an extra
    // control-buffer readback alongside the ordinary root tail.
    return ValidatedBlockedTailV4(ValidatedTailV4(p,w));
}
std::optional<ValidatedSuccessTailV4> CompletionGate::Success(const Profile& profile,const RootPermitV4& p,const Words256& w,const NativeRootEvidence& evidence){
    if(!Common(profile,p,w,evidence)||w[28]!=1)return {};
    uint64_t source,physics;
    if(!CheckedAdd(p.sourceCommit,p.kind==1?1:0,source)||!CheckedAdd(p.physicsCommit,1,physics)||Pair(w,18)!=source||Pair(w,20)!=physics)return {};
    if(p.kind==2&&(w[178]<1||w[178]>3))return {};
    return ValidatedSuccessTailV4(ValidatedTailV4(p,w));
}
}
