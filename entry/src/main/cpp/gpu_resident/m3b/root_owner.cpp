#include "root_owner.h"
#include <utility>
namespace grs::m3b {
namespace {
uint64_t Pair(const Words256& w,size_t at){return uint64_t(w[at])|(uint64_t(w[at+1])<<32);}
bool Same(const RootPermitV5& a,const RootPermitV5& b){
 return a.identity==b.identity&&a.plan==b.plan&&a.nonce==b.nonce&&a.control==b.control&&a.frame==b.frame&&a.geometry==b.geometry&&
 a.sourceCommit==b.sourceCommit&&a.physicsCommit==b.physicsCommit&&a.publicationGeneration==b.publicationGeneration&&a.deadline==b.deadline&&
 a.kind==b.kind&&a.ordinal==b.ordinal&&a.baseBank==b.baseBank&&a.candidateBank==b.candidateBank&&a.spareBank==b.spareBank&&
 a.eventArena==b.eventArena&&a.eventStart==b.eventStart&&a.eventCapacity==b.eventCapacity&&a.publicationSlot==b.publicationSlot&&a.uploadedBytes==b.uploadedBytes&&
 a.carry==b.carry&&a.shaderSha==b.shaderSha&&a.angleSha==b.angleSha&&a.layoutSha==b.layoutSha;
}
}
ControlResult RootOwnerV5::ConfigureLocked(const Profile& profile,uint64_t geometry,const std::array<uint32_t,8>& shader,
                                         const std::array<uint32_t,8>& angle,const std::array<uint32_t,8>& layout){
 if(pendingLive_||run_.active||run_.jobLive||!run_.workspaceRetired)return ControlResult::WOULD_BLOCK;
 if(!geometry||!clock_.open)return ControlResult::INVALID_ARGUMENT;
 profile_=&profile;geometry_=geometry;shaderSha_=shader;angleSha_=angle;layoutSha_=layout;
 currentBank=0;actualBank=1;sourceCommit=physicsCommit=publicationGeneration=0;sourcePrefix=dedupPrefix=eventEnd=0;eventFrame=0;lastGood=stale=false;
 return ControlResult::OK;
}
ControlResult RootOwnerV5::Check(const RootPermitV5& p,uint64_t now)const{
 if(!pendingLive_||!Same(p,pending_)||p.identity!=clock_.identity||p.baseBank!=currentBank||p.candidateBank!=actualBank||p.sourceCommit!=sourceCommit||p.physicsCommit!=physicsCommit||p.publicationGeneration!=publicationGeneration)return ControlResult::STALE_REJECTED;
 if(now>p.deadline)return ControlResult::DEADLINE;
 if(!clock_.Fresh(now)||!clock_.ready||!clock_.baseline||clock_.mode!=PlanningMode::RUNNABLE||!clock_.grant.id||p.plan!=clock_.grant.id||!run_.active||run_.plan!=p.plan||run_.nonce!=p.nonce||!clock_.jobLive||!run_.jobLive)return ControlResult::STALE_REJECTED;
 return ControlResult::OK;
}
ControlResult RootOwnerV5::BeginRootLocked(uint64_t now,uint32_t kind,uint32_t ordinal,uint64_t frame,uint64_t control,
                                         uint32_t arena,uint32_t start,uint32_t publication,uint32_t uploaded,RootPermitV5& out){
 if(!profile_||!frame||!control||arena>1||publication>1||(kind!=1&&kind!=2))return ControlResult::INVALID_ARGUMENT;
 if(pendingLive_||clock_.jobLive||run_.jobLive||!run_.workspaceRetired)return ControlResult::WOULD_BLOCK;
 if(!clock_.Fresh(now)||!clock_.open||!clock_.ready||!clock_.baseline||clock_.mode!=PlanningMode::RUNNABLE||!clock_.grant.id||!run_.active||run_.plan!=clock_.grant.id)return ControlResult::STALE_REJECTED;
 if(kind==1){if(!run_.sourcePhase||ordinal||uploaded<2176||start)return ControlResult::INVALID_ARGUMENT;}
 else if(run_.sourcePhase||clock_.activeRemaining==0||ordinal!=run_.quantumOrdinal+1||ordinal>4||start>profile_->quantumFrameRows)return ControlResult::INVALID_ARGUMENT;
 if(eventFrame&&frame<eventFrame)return ControlResult::SEQUENCE_REJECTED;
 if(kind==2&&start!=(frame==eventFrame?eventEnd:0u))return ControlResult::INVALID_ARGUMENT;
 uint64_t nonce,deadline;if(!CheckedAdd(nextNonce_,1,nonce)||!CheckedAdd(now,ProgressClockV3::RootNs,deadline))return ControlResult::QUOTA_OVERFLOW;
 RootPermitV5 p;p.identity=clock_.identity;p.plan=clock_.grant.id;p.nonce=nonce;p.control=control;p.frame=frame;p.geometry=geometry_;
 p.sourceCommit=sourceCommit;p.physicsCommit=physicsCommit;p.publicationGeneration=publicationGeneration;p.deadline=deadline;p.kind=kind;p.ordinal=ordinal;
 p.baseBank=currentBank;p.candidateBank=actualBank;p.spareBank=2;p.eventArena=arena;p.eventStart=start;p.eventCapacity=kind==1?profile_->sourceRows:profile_->quantumFrameRows;p.publicationSlot=publication;p.uploadedBytes=uploaded;
 p.carry=uint32_t(clock_.grant.carry);p.shaderSha=shaderSha_;p.angleSha=angleSha_;p.layoutSha=layoutSha_;
 pending_=p;pendingLive_=true;nextNonce_=nonce;clock_.jobLive=run_.jobLive=true;clock_.activeRootKind=kind;run_.workspaceRetired=false;run_.nonce=nonce;out=p;return ControlResult::OK;
}
void RootOwnerV5::Publish(const RootPermitV5& p,const Words256& w){
 sourceCommit=Pair(w,18);physicsCommit=Pair(w,20);publicationGeneration=Pair(w,36);sourcePrefix=w[56];dedupPrefix=w[58];publicationSlot=p.publicationSlot;
 if(p.kind==2){eventFrame=p.frame;eventEnd=w[53];}
 std::swap(currentBank,actualBank);lastGood=true;stale=false;clock_.lastGoodStale=false;lastRetiredNonce_=p.nonce;pendingLive_=false;run_.workspaceRetired=true;
}
ControlResult RootOwnerV5::AcceptSource(const RootPermitV5& p,const Words256& w,uint64_t now){
 if(!pendingLive_&&p.nonce==lastRetiredNonce_)return ControlResult::ALREADY_RETIRED;
 const auto check=Check(p,now);if(check!=ControlResult::OK)return check;
 if(p.kind!=1||!run_.sourcePhase||w[57]!=dedupPrefix)return ControlResult::STALE_REJECTED;
 Publish(p,w);clock_.jobLive=run_.jobLive=false;clock_.activeRootKind=0;run_.sourcePhase=false;
 if(clock_.activeRemaining==0){run_.active=false;run_.plan=run_.nonce=0;clock_.grant={};}
 return ControlResult::OK;
}
ControlResult RootOwnerV5::AcceptQuantumLocked(const ValidatedSuccessTailV5& tail,uint64_t now){
 const auto& p=tail.Permit();if(!pendingLive_&&p.nonce==lastRetiredNonce_)return ControlResult::ALREADY_RETIRED;
 const auto check=Check(p,now);if(check!=ControlResult::OK)return check;
 if(p.kind!=2||run_.sourcePhase||p.ordinal!=run_.quantumOrdinal+1||tail.Words()[57]!=dedupPrefix)return ControlResult::STALE_REJECTED;
 const auto accepted=planning_.AcceptQuantumLocked(tail,now);if(accepted!=ControlResult::OK)return accepted;
 // From here only fixed assignments occur, under the same owner mutex.
 Publish(p,tail.Words());if(run_.active)run_.quantumOrdinal=p.ordinal;return ControlResult::OK;
}
ControlResult RootOwnerV5::BlockPlanningLocked(const ValidatedBlockedTailV5& tail,uint64_t now){
 const auto& p=tail.Permit();
 if(!pendingLive_&&p.nonce==lastRetiredNonce_)return ControlResult::ALREADY_RETIRED;
 const auto check=Check(p,now);if(check!=ControlResult::OK)return check;
 const auto blocked=planning_.BlockPlanningControlLocked(tail,now);if(blocked!=ControlResult::OK)return blocked;
 pendingLive_=false;lastRetiredNonce_=p.nonce;run_.workspaceRetired=true;stale=lastGood;return ControlResult::OK;
}
ControlResult RootOwnerV5::RevokeLocked(ControlFault fault){
 const auto result=clock_.RevokeLocked(fault);if(result==ControlResult::QUOTA_OVERFLOW)return result;
 // Keep workspaceRetired=false until transport's old fence/context is retired.
 pendingLive_=false;run_.active=run_.sourcePhase=run_.jobLive=false;run_.plan=run_.nonce=0;run_.quantumOrdinal=run_.continuationStage=0;
 stale=lastGood;clock_.lastGoodStale=lastGood;return result;
}
}
