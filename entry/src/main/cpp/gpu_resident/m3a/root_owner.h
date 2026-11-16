#pragma once
#include "planning_control.h"
namespace grs::m3a {
// All methods use the bridge owner's single control mutex. No GL or callback
// occurs in an acceptance transaction; transport retires the real fence first.
class RootOwnerV4 {
    ProgressClockV3& clock_;
    RunControl& run_;
    PlanningControl planning_;
    const Profile* profile_=nullptr;
    RootPermitV4 pending_;
    uint64_t geometry_=0,nextNonce_=0,lastRetiredNonce_=0;
    std::array<uint32_t,8> shaderSha_{},angleSha_{},layoutSha_{};
    bool pendingLive_=false;
    ControlResult Check(const RootPermitV4&,uint64_t now)const;
    ControlResult AcceptSource(const RootPermitV4&,const Words256&,uint64_t now);
    void Publish(const RootPermitV4&,const Words256&);
public:
    uint32_t currentBank=0,actualBank=1;
    uint64_t sourceCommit=0,physicsCommit=0,publicationGeneration=0;
    uint32_t sourcePrefix=0,dedupPrefix=0,eventEnd=0,publicationSlot=0;
    uint64_t eventFrame=0;
    bool lastGood=false,stale=false;
    RootOwnerV4(ProgressClockV3& clock,RunControl& run):clock_(clock),run_(run),planning_(clock,run){}
    ControlResult ConfigureLocked(const Profile&,uint64_t geometry,const std::array<uint32_t,8>& shader,
                                  const std::array<uint32_t,8>& angle,const std::array<uint32_t,8>& layout);
    ControlResult BeginRootLocked(uint64_t now,uint32_t kind,uint32_t ordinal,uint64_t frame,uint64_t control,
                                  uint32_t eventArena,uint32_t eventStart,uint32_t publication,uint32_t uploaded,RootPermitV4& out);
    bool AuthorizeLocked(const RootPermitV4& permit,uint64_t now)const{return Check(permit,now)==ControlResult::OK;}
    ControlResult AcceptSourceLocked(const ValidatedSuccessTailV4& tail,uint64_t now){return AcceptSource(tail.Permit(),tail.Words(),now);}
    ControlResult AcceptSourceLocked(const ValidatedMetadataTailV4& tail,uint64_t now){return AcceptSource(tail.Permit(),tail.Words(),now);}
    ControlResult AcceptQuantumLocked(const ValidatedSuccessTailV4&,uint64_t now);
    ControlResult BlockPlanningLocked(const ValidatedBlockedTailV4&,uint64_t now);
    ControlResult RevokeLocked(ControlFault);
    bool HasRootLocked()const{return pendingLive_;}
    const RootPermitV4& PermitLocked()const{return pending_;}
};
}
