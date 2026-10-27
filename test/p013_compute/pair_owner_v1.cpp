#include "pair_owner_v1.h"
#include <cmath>
#include <cstring>

namespace p011gpu {
static bool sameIdentity(const RequestIdentity& a,const RequestIdentity& b){
 return a.identityEncodingRevision==b.identityEncodingRevision&&a.runId==b.runId&&a.stateId==b.stateId&&a.configId==b.configId&&
 a.buildManifestSha==b.buildManifestSha&&a.ownerGeneration==b.ownerGeneration&&
 a.contextEpoch==b.contextEpoch&&a.payloadSerial==b.payloadSerial&&
 a.poolMutationSerial==b.poolMutationSerial&&a.attemptSerial==b.attemptSerial&&
 a.substepSerial==b.substepSerial&&a.requestSerial==b.requestSerial&&
 a.candidateRole==b.candidateRole&&a.candidateOrdinal==b.candidateOrdinal&&
 a.slotIndex==b.slotIndex&&a.flags==b.flags&&
 bits64(a.paramsRevision)==bits64(b.paramsRevision)&&
 bits64(a.inputRevision)==bits64(b.inputRevision)&&bits64(a.seed)==bits64(b.seed);
}
template<class T>static bool eq(const T* a,const T* b,size_t n){return a&&b&&std::memcmp(a,b,n*sizeof(T))==0;}
bool sameSnapshotBytes(const SnapshotView& a,const SnapshotView& b){
 if(a.nmax!=b.nmax||a.cols!=b.cols||a.liveCount!=b.liveCount||a.freeCount!=b.freeCount||
  bits64(a.nextId)!=bits64(b.nextId)||a.byteLength!=b.byteLength)return false;
 return eq(a.mass,b.mass,a.nmax)&&eq(a.x,b.x,a.nmax)&&eq(a.y,b.y,a.nmax)&&
  eq(a.vx,b.vx,a.nmax)&&eq(a.vy,b.vy,a.nmax)&&
  eq(a.phase,b.phase,a.nmax)&&eq(a.source,b.source,a.nmax)&&eq(a.state,b.state,a.nmax)&&
  eq(a.id,b.id,a.nmax)&&eq(a.sourceId,b.sourceId,a.nmax)&&eq(a.freeSlots,b.freeSlots,a.nmax)&&
  eq(a.colMassH,b.colMassH,a.cols)&&eq(a.colMassL,b.colMassL,a.cols);
}
static uint32_t expectedDispatches(const PreparedView& p){if(p.n<2)return 0;
 uint32_t count=p.K,reduce=0;do{count=(count+63)/64;reduce++;}while(count>1);
 return p.h+1+reduce;
}
ConsumeStatus verifyAndConsume(const PreparedView& p,const Result& r,const CurrentOwnerState& owner,VerifiedPair& out){
 out=VerifiedPair();if(!p.header||!p.snapshot||!owner.current)return ConsumeStatus::INPUT_INVALID;
 if(!owner.active||owner.poolMutationSerial!=p.mutationAtFreeze||
   !sameIdentity(owner.identity,p.identity)||!sameSnapshotBytes(*p.snapshot,*owner.current))return ConsumeStatus::STALE;
 const auto& h=p.header->w;const auto& w=r.w;
 if(w[0]!=kResultMagic||w[1]!=1||w[3]!=0||w[11]!=0||w[30]!=kDone||w[31]!=0||
   w[10]!=expectedDispatches(p))return ConsumeStatus::PROTOCOL_ERROR;
 for(int i=0;i<4;i++)if(w[12+i]!=h[28+i])return ConsumeStatus::PROTOCOL_ERROR;
 for(int i=0;i<2;i++){
  if(w[16+i]!=h[14+i]||w[18+i]!=h[16+i]||w[20+i]!=h[18+i])return ConsumeStatus::PROTOCOL_ERROR;
 }
 for(int i=0;i<8;i++)if(w[22+i]!=h[48+i])return ConsumeStatus::PROTOCOL_ERROR;
 if(w[2]==kStatusNoPair){for(int i=4;i<=9;i++)if(w[i]!=0)return ConsumeStatus::PROTOCOL_ERROR;
  if(owner.diagnosticOracle){uint32_t a=0,b=0;if(legacyPair(*p.snapshot,p.radius,a,b))return ConsumeStatus::ORACLE_MISMATCH;}
  return ConsumeStatus::NO_PAIR;}
 if(w[2]!=kStatusPair||w[4]==0||w[4]>=w[5]||w[6]>=p.snapshot->nmax||w[7]>=p.snapshot->nmax||
   w[8]>=p.n||w[9]>=p.n)return ConsumeStatus::PROTOCOL_ERROR;
 const auto& a=p.queries[w[8]];const auto& b=p.leaves[w[9]];
 if(a.valid!=1||a.aRank!=w[8]||a.id!=w[4]||a.slot!=w[6]||b.valid!=1||
   b.id!=w[5]||b.slot!=w[7]||w[9]<a.L||w[9]>=a.R||
   !sameGroup(*p.snapshot,w[6],w[7]))return ConsumeStatus::PROTOCOL_ERROR;
 volatile double diff=p.snapshot->y[w[6]]-p.snapshot->y[w[7]];
 if(std::fabs(diff)>p.radius)return ConsumeStatus::PROTOCOL_ERROR;
 if(owner.diagnosticOracle){uint32_t x=0,y=0;
  if(!legacyPair(*p.snapshot,p.radius,x,y)||x!=w[6]||y!=w[7])return ConsumeStatus::ORACLE_MISMATCH;}
 out.found=true;out.idA=w[4];out.idB=w[5];out.slotA=w[6];out.slotB=w[7];out.aRank=w[8];out.bRank=w[9];
 return ConsumeStatus::OK;
}
}
