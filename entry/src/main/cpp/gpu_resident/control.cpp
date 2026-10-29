#include "control.h"
#include <algorithm>
namespace grs {
namespace {
U64 Read(const uint32_t* p,size_t i){return U64{p[i],p[i+1]};}
void Write(Words256& p,size_t i,U64 v){p[i]=v.lo;p[i+1]=v.hi;}
bool Equal(U64 a,U64 b){return a.lo==b.lo&&a.hi==b.hi;}
bool PlusOne(U64 a,U64 b){U64 x;return Add64(a,{1,0},x)&&Equal(x,b);}
}
CompletionPolicy ProductCompletionPolicy(){
 CompletionPolicy p;p.productWire=true;p.magic=wire::magicWords::completion256;p.revision=wire::revisionWord;p.seal=wire::sealWord;p.sourceKind=wire::rootKind::SOURCE;p.quantumKind=wire::rootKind::QUANTUM;
 p.successOutcome=wire::outcome::SUCCESS;p.metadataRejectOutcome=wire::outcome::METADATA_REJECT;p.replayOutcome=wire::outcome::DEDUP_REPLAY;p.successReceipt=wire::terminalStatus::SUCCESS;p.partialReceipt=wire::terminalStatus::PARTIAL;p.rejectedReceipt=wire::terminalStatus::REJECTED;
 p.requiredStages={wire::requiredStageMaskLo,wire::requiredStageMaskHi};p.requiredAudit={wire::requiredAuditMaskLo,wire::requiredAuditMaskHi};p.replayEqualSentinel=wire::replayEqualSentinel;return p;
}
ControlCode Fixed120Clock::Plan(uint64_t deltaNs,TickPlan& out){
 out=TickPlan{};out.deltaNs=deltaNs;out.carryBefore=out.carryAfter=carry;
 if(deltaNs==0)return ControlCode::OK; // strict dt0: not even the first-tick latch changes.
 if(deltaNs>33333333u){Pause();return ControlCode::INVALID_INPUT;}
 if(first){first=false;return ControlCode::OK;}
 uint64_t total=uint64_t(carry)+120*deltaNs;uint32_t f=uint32_t(total/1000000000u);
 if(f>4){Pause();return ControlCode::INVALID_INPUT;}
 out.quanta=f;out.carryAfter=uint32_t(total%1000000000u);carry=out.carryAfter;return ControlCode::OK;
}
bool RootOwner::Match(const uint32_t* p,size_t i,U64 value)const{return Equal(Read(p,i),value);}
ControlCode RootOwner::Initialize(const Identity& id,const CompletionPolicy& pol,const Words256& cold,uint32_t features){
 if(features!=1)return ControlCode::UNSUPPORTED_FEATURE;
 if(!pol.magic||!pol.revision||!pol.seal||!pol.sourceKind||!pol.quantumKind||pol.sourceKind==pol.quantumKind||!pol.successOutcome||!pol.metadataRejectOutcome||!pol.replayOutcome||pol.successOutcome==pol.metadataRejectOutcome||pol.successOutcome==pol.replayOutcome||pol.metadataRejectOutcome==pol.replayOutcome||!pol.successReceipt||!pol.partialReceipt||!pol.rejectedReceipt||(pol.requiredStages.lo|pol.requiredStages.hi)==0||(pol.requiredAudit.lo|pol.requiredAudit.hi)==0)return ControlCode::INVALID_INPUT;
 if(state==FlightState::Submitted||state==FlightState::Quarantined)return ControlCode::BUSY;
 for(const auto& p:publications)if(p.leased||p.releasePending)return ControlCode::BACKPRESSURE;
 if((id.run.lo|id.run.hi)==0||(id.owner.lo|id.owner.hi)==0||(id.context.lo|id.context.hi)==0||pol.magic==0||!Match(cold.data(),header256::runLo,id.run)||!Match(cold.data(),header256::ownerLo,id.owner)||!Match(cold.data(),header256::contextEpochLo,id.context)||!Match(cold.data(),header256::surfaceEpochLo,id.surface)||!Match(cold.data(),header256::geometryLo,id.geometry)||!Match(cold.data(),header256::configLo,id.config)||cold[header256::featureBitmapLo]!=1||cold[header256::featureBitmapHi]!=0)return ControlCode::INVALID_INPUT;
 for(size_t i=112;i<256;i++)if(cold[i])return ControlCode::INVALID_INPUT;
 for(size_t i=0;i<8;i++)if(cold[header256::shaderSha_0+i]!=id.shaderSha[i]||cold[header256::angleSha_0+i]!=id.angleSha[i]||cold[header256::layoutSha_0+i]!=id.layoutSha[i])return ControlCode::INVALID_INPUT;
 identity=id;policy=pol;header=cold;acceptedBank=0;candidateBank=1;spareBank=2;state=FlightState::Idle;tombstone=false;lastNonce={};pending={};publications[0]={};publications[1]={};mappedBytes=acceptedCount=0;return ControlCode::OK;
}
ControlCode RootOwner::Begin(const RootRequest& r){
 if(state==FlightState::StateLost)return ControlCode::OWNER_STATE_LOST;
 if(state==FlightState::Quarantined)return ControlCode::QUARANTINED;if(state!=FlightState::Idle)return ControlCode::BUSY;
 if(policy.productWire&&((r.kind==policy.sourceKind&&r.quantumOrdinal!=0)||(r.kind==policy.quantumKind&&r.quantumOrdinal==0)))return ControlCode::INVALID_INPUT;
 if(r.publicationSlot>=2||r.eventArena>=2||r.eventStart>r.eventCapacity||!r.eventCapacity||!r.dispatchCount||r.barrierCount<r.dispatchCount||r.carry>=1000000000u||(r.kind!=policy.sourceKind&&r.kind!=policy.quantumKind)||r.quantumOrdinal>4)return ControlCode::INVALID_INPUT;
 if(Compare64(r.nonce,lastNonce)<=0)return ControlCode::STALE;
 const auto& p=publications[r.publicationSlot];if(p.leased||p.releasePending)return ControlCode::BACKPRESSURE;
 if(header[header256::acceptedSourceCount]>4096||header[header256::acceptedSourceEventCount]>4096)return ControlCode::COMPLETION_INVALID;
 pending=r;lastNonce=r.nonce;tombstone=false;state=FlightState::Submitted;return ControlCode::OK;
}
bool RootOwner::ValidCompletion(const uint32_t* c,size_t count,uint32_t rawGlError)const {
 using namespace completion256;
 if(count!=Words||rawGlError||c[magic]!=policy.magic||c[revision]!=policy.revision||c[wordLength]!=256||c[seal]!=policy.seal||c[firstGlError]||c[firstErrorStage]||c[firstErrorOrdinal]||c[replayMismatchWord]!=policy.replayEqualSentinel||c[replayMismatchExpected]||c[replayMismatchActual])return false;
 for(size_t i=138;i<256;i++)if(c[i])return false;
 if(!Match(c,rootNonceLo,pending.nonce)||!Match(c,runLo,identity.run)||!Match(c,ownerLo,identity.owner)||!Match(c,contextEpochLo,identity.context)||!Match(c,surfaceEpochLo,identity.surface)||!Match(c,geometryLo,identity.geometry)||!Match(c,configLo,identity.config)||!Match(c,controlSerialLo,pending.control)||!Match(c,frameSerialLo,pending.frame))return false;
 const size_t cm[]={numericRev,timeRev,angleRev,mediumRev,layoutRev,algorithmRev,sceneRev,featureBitmapLo,featureBitmapHi};const size_t hm[]={header256::numericRev,header256::timeRev,header256::angleRev,header256::mediumRev,header256::layoutRev,header256::algorithmRev,header256::sceneRev,header256::featureBitmapLo,header256::featureBitmapHi};
 for(size_t i=0;i<9;i++)if(c[cm[i]]!=header[hm[i]])return false;
 for(size_t i=0;i<8;i++)if(c[shaderSha_0+i]!=identity.shaderSha[i]||c[angleSha_0+i]!=identity.angleSha[i]||c[layoutSha_0+i]!=identity.layoutSha[i])return false;
 if(!Match(c,baseSourceCommitLo,Read(header.data(),header256::sourceCommitLo))||!Match(c,basePhysicsCommitLo,Read(header.data(),header256::physicsCommitLo))||c[baseBank]!=acceptedBank||c[completion256::candidateBank]!=this->candidateBank||c[completion256::spareBank]!=this->spareBank||c[publicationSlot]!=pending.publicationSlot||c[quantumOrdinal]!=pending.quantumOrdinal||c[rootKind]!=pending.kind||c[eventArenaIndex]!=pending.eventArena||c[eventStart]!=pending.eventStart||c[eventCapacity]!=pending.eventCapacity||c[eventEnd]<c[eventStart]||c[eventEnd]>c[eventCapacity])return false;
 if(!PlusOne(Read(header.data(),header256::publicationGenerationLo),Read(c,publicationGenerationLo))||!Equal(Read(c,stageCompletedMaskLo),policy.requiredStages)||!Equal(Read(c,auditFlagMaskLo),policy.requiredAudit))return false;
 if(policy.productWire&&(c[fenceStatus]!=0||c[reason]>wire::reason::UNSUPPORTED_INITIAL_STATE||c[receiptReason]>wire::reason::UNSUPPORTED_INITIAL_STATE))return false;
 if(c[actualDispatchCount]!=pending.dispatchCount||c[actualBarrierCount]!=pending.barrierCount||c[actualBytesUploaded]!=pending.uploadedBytes||c[actualBytesMapped]!=0||c[timeCarryHi]!=0||c[timeCarryLo]!=pending.carry)return false;
 if(c[sourceEventPrefixBefore]!=header[header256::acceptedSourceEventCount]||c[dedupPrefixBefore]!=header[header256::acceptedSourceCount]||c[sourceEventPrefixAfter]>4096||c[dedupPrefixAfter]>4096||c[sourceEventPrefixAfter]<c[sourceEventPrefixBefore]||c[dedupPrefixAfter]<c[dedupPrefixBefore]||c[sourceEventPrefixAfter]-c[sourceEventPrefixBefore]>1||c[dedupPrefixAfter]-c[dedupPrefixBefore]>1)return false;
 int64_t angle=c[angleAcceptedIndex]<=0x7fffffffu?int64_t(c[angleAcceptedIndex]):int64_t(c[angleAcceptedIndex])-4294967296LL;
 if(angle<angleMinIndex||angle>angleMaxIndex||c[nextWaterId]!=header[header256::nextWaterId])return false;
 if(c[waterLive]||c[waterFree]!=64||!Match(c,waterBeforeLo,{})||!Match(c,waterAfterLo,{})||!Match(c,I_waterLo,{})||c[sandLive]>512||c[sandFree]>512||c[sandLive]+c[sandFree]!=header[header256::sandLive]+header[header256::sandFree]||c[capacityMax]>Cq)return false;
 if(!Match(c,sandBeforeLo,Read(header.data(),header256::sandMassLo)))return false;
 if(policy.productWire){
  if(c[outcome]==policy.successOutcome){
   if(c[metadataOnlyStockEqual]!=0)return false;
   if(pending.kind==policy.sourceKind){
    const uint32_t oldNext=header[header256::nextSandId],births=c[receiptBirthCount];
    if(oldNext==0||births==0||uint64_t(births-1)>uint64_t(UINT32_MAX)-oldNext)return false;
    const uint64_t lastId=uint64_t(oldNext)+births-1;
    const uint32_t expectedNext=lastId==UINT32_MAX?0:uint32_t(lastId+1);
    if(c[receiptFirstId]!=oldNext||c[receiptNextId]!=expectedNext||c[nextSandId]!=expectedNext||
       c[dedupPrefixAfter]!=c[dedupPrefixBefore]+1||c[sourceEventPrefixAfter]!=c[sourceEventPrefixBefore]+1||
       (c[receiptStatus]==policy.successReceipt&&c[rejectedUnits]!=0)||
       (c[receiptStatus]==policy.partialReceipt&&c[rejectedUnits]==0))return false;
   }
   else {if(c[nextSandId]!=header[header256::nextSandId])return false;const size_t sourceFields[]={sourceId,requestSeq,admittedUnits,rejectedUnits,receiptStatus,receiptReason,receiptReplayFlag,receiptBirthCount,receiptFirstId,receiptNextId,sourceEventCount};for(size_t i:sourceFields)if(c[i])return false;if(c[dedupPrefixAfter]!=c[dedupPrefixBefore]||c[sourceEventPrefixAfter]!=c[sourceEventPrefixBefore])return false;}
  }else if(c[outcome]==policy.metadataRejectOutcome){
   if(c[reason]==0||c[receiptReason]!=c[reason]||c[receiptReplayFlag]||c[sourceEventPrefixAfter]!=c[sourceEventPrefixBefore]||c[nextSandId]!=header[header256::nextSandId]||c[sandLive]!=header[header256::sandLive]||c[sandFree]!=header[header256::sandFree])return false;
   uint32_t expectedPrefix=c[dedupPrefixBefore]+(c[reason]==wire::reason::DUPLICATE_KEY_CONFLICT||c[reason]==wire::reason::SOURCE_LOG_FULL?0:1);if(c[dedupPrefixAfter]!=expectedPrefix)return false;
  }else if(c[outcome]==policy.replayOutcome){if(c[reason]||c[dedupPrefixAfter]!=c[dedupPrefixBefore]||c[sourceEventPrefixAfter]!=c[sourceEventPrefixBefore]||c[receiptStatus]<1||c[receiptStatus]>3||c[nextSandId]!=header[header256::nextSandId]||c[sandLive]!=header[header256::sandLive]||c[sandFree]!=header[header256::sandFree])return false;}
 }
 // Only checking bounded receipts/identities here. Full stock, guard and replay are GPU duties.
 if(c[outcome]==policy.metadataRejectOutcome||c[outcome]==policy.replayOutcome){
  if(pending.kind!=policy.sourceKind||c[metadataOnlyStockEqual]!=1||!Match(c,sandBeforeLo,Read(c,sandAfterLo))||!Match(c,I_sandLo,Read(header.data(),header256::I_sandLo))||!Match(c,proposedSourceCommitLo,Read(c,baseSourceCommitLo))||!Match(c,proposedPhysicsCommitLo,Read(c,basePhysicsCommitLo))||(c[outcome]==policy.metadataRejectOutcome&&(c[receiptStatus]!=policy.rejectedReceipt||c[admittedUnits]||c[receiptBirthCount]))||(c[outcome]==policy.replayOutcome&&c[receiptReplayFlag]!=1))return false;
 }else if(c[outcome]==policy.successOutcome){
  if(c[reason]||c[receiptReplayFlag])return false;
  if(!PlusOne(Read(c,basePhysicsCommitLo),Read(c,proposedPhysicsCommitLo)))return false;
  if(pending.kind==policy.sourceKind){
   if(c[admittedUnits]==0||!PlusOne(Read(c,baseSourceCommitLo),Read(c,proposedSourceCommitLo))||(c[receiptStatus]!=policy.successReceipt&&c[receiptStatus]!=policy.partialReceipt))return false;
   U64 mass,admitted{c[admittedUnits],0};if(!Add64(Read(c,sandBeforeLo),admitted,mass)||!Match(c,sandAfterLo,mass)||!Add64(Read(header.data(),header256::I_sandLo),admitted,mass)||!Match(c,I_sandLo,mass))return false;
  }else if(!Match(c,proposedSourceCommitLo,Read(c,baseSourceCommitLo))||!Match(c,sandAfterLo,Read(c,sandBeforeLo))||!Match(c,I_sandLo,Read(header.data(),header256::I_sandLo)))return false;
 }else return false;
 return true;
}
ControlCode RootOwner::Poll(FenceState fence,const uint32_t* c,size_t count,uint32_t glError,AcceptedRoot& out){
 out=AcceptedRoot{};
 if(state==FlightState::StateLost)return ControlCode::OWNER_STATE_LOST;
 if(state==FlightState::Idle)return ControlCode::STALE;
 if(fence==FenceState::Pending)return state==FlightState::Quarantined?ControlCode::QUARANTINED:ControlCode::BUSY;
 if(fence==FenceState::Failed||fence==FenceState::Timeout){state=FlightState::Quarantined;tombstone=true;return ControlCode::QUARANTINED;}
 if(tombstone||state==FlightState::Quarantined){state=FlightState::Idle;tombstone=false;return ControlCode::STALE;}
 // Successful fence permits at most exactly one bounded completion read by GL transport.
 if(count!=256||c==nullptr){state=FlightState::Idle;return ControlCode::COMPLETION_INVALID;}
 mappedBytes+=1024;
 if(c==nullptr||!ValidCompletion(c,count,glError)){state=FlightState::Idle;return ControlCode::COMPLETION_INVALID;}
 using namespace completion256;
 out.bank=c[completion256::candidateBank];out.publication=c[publicationSlot];out.outcome=c[outcome];out.source=Read(c,proposedSourceCommitLo);out.physics=Read(c,proposedPhysicsCommitLo);out.publicationGeneration=Read(c,publicationGenerationLo);out.nonce=pending.nonce;out.sourceSuccess=pending.kind==policy.sourceKind&&c[outcome]==policy.successOutcome;std::copy(c,c+256,out.completion.begin());
 // Selector and bounded summary update form one non-callback owner operation.
 uint32_t old=acceptedBank;acceptedBank=this->candidateBank;this->candidateBank=old;
 Write(header,header256::rootNonceLo,pending.nonce);Write(header,header256::sourceCommitLo,out.source);Write(header,header256::physicsCommitLo,out.physics);Write(header,header256::controlSerialLo,pending.control);Write(header,header256::frameSerialLo,pending.frame);Write(header,header256::publicationGenerationLo,out.publicationGeneration);Write(header,header256::sandMassLo,Read(c,sandAfterLo));Write(header,header256::I_sandLo,Read(c,I_sandLo));header[header256::acceptedSourceCount]=c[dedupPrefixAfter];if(policy.productWire)Write(header,header256::sourceLedgerRevLo,{c[dedupPrefixAfter],0});header[header256::acceptedSourceEventCount]=c[sourceEventPrefixAfter];header[header256::sandLive]=c[sandLive];header[header256::sandFree]=c[sandFree];header[header256::nextSandId]=c[nextSandId];header[header256::acceptedAngleIndex]=c[angleAcceptedIndex];header[header256::quantumOrdinal]=c[quantumOrdinal];header[header256::timeCarryLo]=c[timeCarryLo];header[header256::timeCarryHi]=c[timeCarryHi];header[header256::acceptedBank]=acceptedBank;header[header256::candidateBank]=this->candidateBank;header[header256::spareBank]=this->spareBank;
 PublicationLease& pub=publications[out.publication];pub.identity=identity;pub.source=out.source;pub.physics=out.physics;pub.control=pending.control;pub.frame=pending.frame;pub.nonce=pending.nonce;pub.angle=c[angleAcceptedIndex];pub.available=true;pub.leased=pub.releasePending=false;pub.generation=out.publicationGeneration;pub.run=identity.run;pub.context=identity.context;pub.surface=identity.surface;pub.bank=acceptedBank;
 acceptedCount++;state=FlightState::Idle;return ControlCode::OK;
}
ControlCode RootOwner::Stop(){if(state==FlightState::StateLost)return ControlCode::OWNER_STATE_LOST;if(state==FlightState::Submitted){tombstone=true;state=FlightState::Quarantined;return ControlCode::QUARANTINED;}return state==FlightState::Quarantined?ControlCode::QUARANTINED:ControlCode::OK;}
void RootOwner::ContextLost(){state=FlightState::StateLost;tombstone=true;for(auto& p:publications)p=PublicationLease{};}
ControlCode RootOwner::ResetWhenRetired(){if(state==FlightState::Submitted||state==FlightState::Quarantined)return ControlCode::QUARANTINED;for(const auto& p:publications)if(p.leased||p.releasePending)return ControlCode::BACKPRESSURE;state=FlightState::StateLost;return ControlCode::OK;}
ControlCode RootOwner::Acquire(uint32_t slot,U64 generation,PublicationLease& out){
 if(state==FlightState::StateLost)return ControlCode::OWNER_STATE_LOST;if(slot>=2)return ControlCode::INVALID_INPUT;auto& p=publications[slot];if(!p.available||!Equal(p.generation,generation))return ControlCode::STALE;if(p.leased||p.releasePending)return ControlCode::BACKPRESSURE;p.leased=true;out=p;return ControlCode::OK;
}
ControlCode RootOwner::Release(uint32_t slot,U64 generation,bool signaled){if(slot>=2)return ControlCode::INVALID_INPUT;auto& p=publications[slot];if(!p.leased||!Equal(p.generation,generation))return ControlCode::STALE;p.leased=false;p.releasePending=!signaled;return ControlCode::OK;}
ControlCode RootOwner::RetireRelease(uint32_t slot,U64 generation,bool signaled){if(slot>=2)return ControlCode::INVALID_INPUT;auto& p=publications[slot];if(!p.releasePending||!Equal(p.generation,generation))return ControlCode::STALE;if(!signaled)return ControlCode::BUSY;p.releasePending=false;return ControlCode::OK;}
}
