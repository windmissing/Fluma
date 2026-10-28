#include "integer.h"
#include "control.h"
#include "resources.h"
#include "angles_generated.h"
#include <fstream>
#include <iostream>
#include <cassert>
#include <cstring>
#include <climits>
using namespace grs;
int32_t Signed(uint32_t v){return v<=INT32_MAX?int32_t(v):int32_t(int64_t(v)-4294967296LL);}
int numeric(const char* input,const char* output){
 std::ifstream in(input,std::ios::binary);std::ofstream out(output,std::ios::binary);if(!in||!out)return 1;
 uint32_t w[12];while(in.read((char*)w,sizeof(w))){uint32_t r[8]{};r[0]=1;U64 v{0x12345678,0x87654321};U128 z{};uint32_t rem=0;int32_t value=0x12345678;
  switch(w[0]){
   case 1:r[0]=Add64({w[1],w[2]},{w[3],w[4]},v);r[1]=v.lo;r[2]=v.hi;break;
   case 2:r[0]=Sub64({w[1],w[2]},{w[3],w[4]},v);r[1]=v.lo;r[2]=v.hi;break;
   case 3:v=Mul32(w[1],w[2]);r[1]=v.lo;r[2]=v.hi;break;
   case 4:z=Mul64({w[1],w[2]},{w[3],w[4]});for(int i=0;i<4;i++)r[1+i]=z.w[i];break;
   case 5:r[0]=Divide128(U128{{w[1],w[2],w[3],w[4]}},w[5],z,rem);for(int i=0;i<4;i++)r[1+i]=z.w[i];r[5]=rem;break;
   case 6:r[0]=MulDivRNE(Signed(w[1]),Signed(w[2]),w[3],value);r[1]=uint32_t(value);break;
   case 7:r[1]=0x12345678;r[0]=FloorBinary64Q20(w[1],w[2],w[3],r[1]);break;
   case 8:r[1]=Q20FloatBits(w[1]);break;
   case 9:r[1]=ColumnPreservingFloatBits(w[1]);break;
   case 10:r[0]=FloorDiv(Signed(w[1]),Signed(w[2]),value);r[1]=uint32_t(value);break;
   case 11:r[0]=CeilDiv(Signed(w[1]),Signed(w[2]),value);r[1]=uint32_t(value);break;
   case 12:r[1]=uint32_t(Compare128(U128{{w[1],w[2],w[3],w[4]}},U128{{w[5],w[6],w[7],w[8]}}));break;
   default:return 2;
  }
  out.write((char*)r,sizeof(r));
 }
 return in.eof()?0:3;
}
Words256 cold(Identity& id){
 Words256 h{};h[header256::magic]=0x48535247;h[header256::revision]=1;h[header256::wordLength]=256;
 id.run={1,0};id.owner={2,0};id.context={3,0};id.surface={4,0};id.geometry={5,0};id.config={6,0};
 h[header256::runLo]=1;h[header256::ownerLo]=2;h[header256::contextEpochLo]=3;h[header256::surfaceEpochLo]=4;h[header256::geometryLo]=5;h[header256::configLo]=6;
 h[header256::featureBitmapLo]=1;for(size_t i=header256::numericRev;i<=header256::sceneRev;i++)h[i]=1;
 h[header256::nextSandId]=h[header256::nextWaterId]=1;h[header256::sandFree]=192;h[header256::waterFree]=64;h[header256::sourceLedgerRevLo]=1;
 for(size_t i=0;i<8;i++){id.shaderSha[i]=uint32_t(i+10);id.angleSha[i]=uint32_t(i+20);id.layoutSha[i]=uint32_t(i+30);h[88+i]=id.shaderSha[i];h[96+i]=id.angleSha[i];h[104+i]=id.layoutSha[i];}return h;
}
CompletionPolicy pol(){CompletionPolicy p;p.magic=0x43535247;p.revision=p.seal=1;p.successOutcome=1;p.metadataRejectOutcome=2;p.replayOutcome=3;p.sourceKind=1;p.quantumKind=2;p.successReceipt=1;p.partialReceipt=2;p.rejectedReceipt=3;p.replayEqualSentinel=0xffffffffu;p.requiredStages={0xffffff,0};p.requiredAudit={0xffff,0};return p;}
RootRequest request(uint32_t nonce=1){RootRequest r;r.kind=1;r.nonce={nonce,0};r.control={1,0};r.frame={1,0};r.publicationSlot=0;r.eventCapacity=26044;r.dispatchCount=32;r.barrierCount=32;r.uploadedBytes=128;return r;}
Words256 result(const RootOwner& owner,const RootRequest& r,const Identity& id){
 using namespace completion256;Words256 c{};auto p=pol();const auto& h=owner.Header();c[magic]=p.magic;c[revision]=1;c[wordLength]=256;c[seal]=1;c[rootNonceLo]=r.nonce.lo;c[rootNonceHi]=r.nonce.hi;c[runLo]=id.run.lo;c[ownerLo]=id.owner.lo;c[contextEpochLo]=id.context.lo;c[surfaceEpochLo]=id.surface.lo;c[geometryLo]=id.geometry.lo;c[configLo]=id.config.lo;c[controlSerialLo]=r.control.lo;c[frameSerialLo]=r.frame.lo;c[rootKind]=1;c[outcome]=1;c[candidateBank]=owner.Bank()==0?1:0;c[baseBank]=owner.Bank();c[spareBank]=2;c[publicationSlot]=r.publicationSlot;c[publicationGenerationLo]=h[header256::publicationGenerationLo]+1;
 c[baseSourceCommitLo]=h[header256::sourceCommitLo];c[basePhysicsCommitLo]=h[header256::physicsCommitLo];c[proposedSourceCommitLo]=c[baseSourceCommitLo]+1;c[proposedPhysicsCommitLo]=c[basePhysicsCommitLo]+1;
 for(size_t i=0;i<7;i++)c[numericRev+i]=h[header256::numericRev+i];c[featureBitmapLo]=1;c[eventArenaIndex]=r.eventArena;c[eventStart]=r.eventStart;c[eventEnd]=r.eventStart+1;c[eventCapacity]=r.eventCapacity;c[actualDispatchCount]=r.dispatchCount;c[actualBarrierCount]=r.barrierCount;c[actualBytesUploaded]=r.uploadedBytes;c[timeCarryLo]=r.carry;c[stageCompletedMaskLo]=p.requiredStages.lo;c[auditFlagMaskLo]=p.requiredAudit.lo;c[replayMismatchWord]=0xffffffff;
 c[sourceEventPrefixBefore]=h[header256::acceptedSourceEventCount];c[sourceEventPrefixAfter]=c[sourceEventPrefixBefore]+1;c[dedupPrefixBefore]=h[header256::acceptedSourceCount];c[dedupPrefixAfter]=c[dedupPrefixBefore]+1;
 c[sourceId]=c[requestSeq]=1;c[admittedUnits]=1;c[receiptStatus]=1;c[receiptBirthCount]=1;c[receiptFirstId]=h[header256::nextSandId];c[receiptNextId]=c[receiptFirstId]+1;
 c[sandBeforeLo]=h[header256::sandMassLo];c[sandBeforeHi]=h[header256::sandMassHi];c[sandAfterLo]=c[sandBeforeLo]+1;c[I_sandLo]=h[header256::I_sandLo]+1;c[sandLive]=h[header256::sandLive]+1;c[sandFree]=192-c[sandLive];c[waterFree]=64;c[nextSandId]=c[receiptNextId];c[nextWaterId]=1;
 for(size_t i=0;i<8;i++){c[shaderSha_0+i]=id.shaderSha[i];c[angleSha_0+i]=id.angleSha[i];c[layoutSha_0+i]=id.layoutSha[i];}return c;
}
int main(int argc,char** argv){
 if(argc==3)return numeric(argv[1],argv[2]);
 int passed=0;Identity id;auto h=cold(id);auto policy=pol();
 {RootOwner o;assert(o.Initialize(id,policy,h,2)==ControlCode::UNSUPPORTED_FEATURE);assert(o.State()==FlightState::StateLost);passed++;}
 {RootOwner o;assert(o.Initialize(id,policy,h,1)==ControlCode::OK);auto r=request();assert(o.Begin(r)==ControlCode::OK);AcceptedRoot accepted;auto c=result(o,r,id);auto before=o.Header();assert(o.Poll(FenceState::Pending,nullptr,0,0,accepted)==ControlCode::BUSY);assert(o.Header()==before);assert(o.Poll(FenceState::Signaled,c.data(),256,0,accepted)==ControlCode::OK);assert(accepted.sourceSuccess&&accepted.bank==1&&o.Bank()==1&&o.AcceptedCount()==1&&o.MappedBytes()==1024);assert(o.Header()[header256::I_sandLo]==1);passed++;}
 // Each immutable identity/schema/first-error/ledger corruption rejects acceptance.
 const size_t mutations[]={0,1,2,3,4,6,8,10,12,14,16,18,20,22,24,27,30,31,32,33,34,35,36,38,40,42,43,44,45,46,47,48,49,50,51,52,54,55,57,69,71,73,75,77,79,88,89,90,91,97,99,101,104,108,110,111,112,113,114,122,130,138,255};
 for(size_t i:mutations){RootOwner o;assert(o.Initialize(id,policy,h,1)==ControlCode::OK);auto r=request();assert(o.Begin(r)==ControlCode::OK);auto c=result(o,r,id);c[i]^=0x80000000;AcceptedRoot accepted;auto before=o.Header();auto rejection=o.Poll(FenceState::Signaled,c.data(),256,0,accepted);if(rejection!=ControlCode::COMPLETION_INVALID)std::cerr<<"accepted mutation word "<<i<<std::endl;assert(rejection==ControlCode::COMPLETION_INVALID);assert(o.Header()==before&&o.Bank()==0&&!accepted.sourceSuccess);passed++;}
 {RootOwner o;o.Initialize(id,policy,h,1);auto r=request();o.Begin(r);auto c=result(o,r,id);AcceptedRoot accepted;assert(o.Stop()==ControlCode::QUARANTINED);assert(o.Begin(request(2))==ControlCode::QUARANTINED);assert(o.ResetWhenRetired()==ControlCode::QUARANTINED);assert(o.Poll(FenceState::Pending,nullptr,0,0,accepted)==ControlCode::QUARANTINED);assert(o.Poll(FenceState::Signaled,c.data(),256,0,accepted)==ControlCode::STALE);assert(o.AcceptedCount()==0&&o.Bank()==0&&o.MappedBytes()==0);assert(o.Begin(r)==ControlCode::STALE);assert(o.Begin(request(2))==ControlCode::OK);passed++;}
 for(auto failure:{FenceState::Failed,FenceState::Timeout}){RootOwner o;o.Initialize(id,policy,h,1);auto r=request();o.Begin(r);auto c=result(o,r,id);AcceptedRoot accepted;assert(o.Poll(failure,nullptr,0,0,accepted)==ControlCode::QUARANTINED);assert(o.Poll(FenceState::Signaled,c.data(),256,0,accepted)==ControlCode::STALE);assert(o.AcceptedCount()==0);passed++;}
 {RootOwner o;o.Initialize(id,policy,h,1);auto r=request();o.Begin(r);auto c=result(o,r,id);AcceptedRoot accepted;o.ContextLost();assert(o.Poll(FenceState::Signaled,c.data(),256,0,accepted)==ControlCode::OWNER_STATE_LOST);assert(o.Begin(request(2))==ControlCode::OWNER_STATE_LOST);PublicationLease lease;assert(o.Acquire(0,{1,0},lease)==ControlCode::OWNER_STATE_LOST);passed++;}
 {RootOwner o;o.Initialize(id,policy,h,1);auto r=request();o.Begin(r);auto c=result(o,r,id);AcceptedRoot accepted;o.Poll(FenceState::Signaled,c.data(),256,0,accepted);PublicationLease lease;assert(o.Acquire(0,{1,0},lease)==ControlCode::OK);assert(o.Begin(request(2))==ControlCode::BACKPRESSURE);assert(o.Release(0,{0,0},true)==ControlCode::STALE);assert(o.Release(0,{1,0},false)==ControlCode::OK);assert(o.Begin(request(2))==ControlCode::BACKPRESSURE);assert(o.RetireRelease(0,{1,0},false)==ControlCode::BUSY);assert(o.RetireRelease(0,{1,0},true)==ControlCode::OK);assert(o.Begin(request(2))==ControlCode::OK);passed++;}
 {RootOwner o;o.Initialize(id,policy,h,1);auto r=request();o.Begin(r);auto c=result(o,r,id);AcceptedRoot accepted;o.Poll(FenceState::Signaled,c.data(),256,0,accepted);auto saved=o.Header();auto r2=request(2);r2.kind=2;r2.publicationSlot=1;r2.quantumOrdinal=1;o.Begin(r2);auto bad=result(o,r2,id);assert(o.Poll(FenceState::Signaled,bad.data(),256,0,accepted)==ControlCode::COMPLETION_INVALID);assert(o.Header()==saved&&o.AcceptedCount()==1);passed++;}
 {Fixed120Clock clock;TickPlan p;assert(clock.Plan(0,p)==ControlCode::OK&&p.quanta==0);assert(clock.Plan(8333333,p)==ControlCode::OK&&p.quanta==0);assert(clock.Plan(8333333,p)==ControlCode::OK&&p.quanta==0&&p.carryAfter==999999960);assert(clock.Plan(1,p)==ControlCode::OK&&p.quanta==1&&p.carryAfter==80);assert(clock.Plan(33333334,p)==ControlCode::INVALID_INPUT&&clock.Carry()==0);assert(clock.Plan(33333333,p)==ControlCode::OK&&p.quanta==0);assert(clock.Plan(33333333,p)==ControlCode::OK&&p.quanta==3&&p.carryAfter==999999960);passed++;}
 for(uint32_t K:{192u,512u}){ResourcePlan plan;ProfileLimits limits;assert(PlanResources(K,limits,plan)==ResourceCode::CAPABILITY_REQUIRED);assert(plan.payloadBytes==(K==192?15059984:36997904));assert(plan.paddedSortExtraBytes==(K==192?512:0));assert(plan.tickDispatch==(K==192?268:306));limits.measured=true;limits.globalBindings=limits.computeBlocks=4;limits.workGroupInvocations=limits.workGroupSize=64;limits.maxBlockBytes=1u<<30;limits.maxTotalPayloadBytes=64u<<20;limits.storageAlignment=256;assert(PlanResources(K,limits,plan)==ResourceCode::OK&&!plan.deviceValidated);assert(plan.allocatedBytes>=plan.payloadBytes);limits.maxBlockBytes=1024;assert(PlanResources(K,limits,plan)==ResourceCode::MEMORY_PROFILE_UNSUPPORTED);passed++;}
 {StateSegments p;assert(PlanStateSegments(192,256,1u<<20,p));assert(p.sand==0&&p.water%256==0&&p.column%256==0&&p.header%256==0&&p.total==24576);assert(!PlanStateSegments(192,0,1u<<20,p));size_t out=17;assert(!CheckedBytes(SIZE_MAX,SIZE_MAX,out)&&out==17);assert(!AlignUp(SIZE_MAX,256,SIZE_MAX,out)&&out==17);passed++;}
 {int32_t last=-999999999;for(int i=0;i<15361;i++){int32_t t=Signed(kAngleWords[4*i]);assert(t>=last);last=t;assert(kAngleWords[4*i+3]==0);assert(Signed(kAngleWords[4*(15360-i)])==-t);}assert(Signed(kAngleWords[4*7680])==0);assert(Signed(kAngleWords[4*7680+2])==1048576);passed++;}
 std::cout<<"{\"scope\":\"T236_FOUNDATION_ONLY_NOT_M0_PRODUCT\",\"passed\":"<<passed<<",\"failed\":0,\"GPU\":\"NOT_EXECUTED\"}"<<std::endl;return 0;
}
