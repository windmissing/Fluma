#include "pair_prepare_v1.h"
#include "pair_identity_v2.h"
#include "pair_sha256_v1.h"
#include <algorithm>
#include <cmath>
#include <cfenv>
#include <cstring>
#include <limits>

namespace p011gpu {
static uint32_t ceilPow2(uint32_t n){uint32_t k=1;while(k<n&&k<=0x40000000u)k*=2;return k;}
CpuSlot::CpuSlot(uint32_t cap):nmax(cap),K(ceilPow2(std::max(1u,cap))){
 while((1u<<h)<K)h++;order.resize(cap);idOrder.resize(cap);leaves.resize(K);queries.resize(K);
 fMass.resize(cap);fX.resize(cap);fY.resize(cap);fVx.resize(cap);fVy.resize(cap);
 fPhase.resize(cap);fSource.resize(cap);fState.resize(cap);fId.resize(cap);fSourceId.resize(cap);fFree.resize(cap);
 freeSeen.resize(cap);
 fColH.resize(128);fColL.resize(128);
}
bool sameGroup(const SnapshotView& p,uint32_t a,uint32_t b){return p.phase[a]==p.phase[b]&&
 p.source[a]==p.source[b]&&p.sourceId[a]==p.sourceId[b]&&p.state[a]==p.state[b]&&
 std::floor(p.x[a])==std::floor(p.x[b]);}
static bool groupLess(const SnapshotView& p,uint32_t a,uint32_t b){
 if(p.phase[a]!=p.phase[b])return p.phase[a]<p.phase[b];
 if(p.source[a]!=p.source[b])return p.source[a]<p.source[b];
 if(p.sourceId[a]!=p.sourceId[b])return p.sourceId[a]<p.sourceId[b];
 if(p.state[a]!=p.state[b])return p.state[a]<p.state[b];
 double x=std::floor(p.x[a]),y=std::floor(p.x[b]);return x<y;
}
static bool within(double a,double b,double radius){volatile double d=a-b;return std::fabs(d)<=radius;}
bool legacyPair(const SnapshotView& p,double radius,uint32_t& slotA,uint32_t& slotB){
 slotA=slotB=0;if(!std::isfinite(radius)||radius<0)return false;uint32_t bestA=0,bestB=0;bool found=false;
 for(uint32_t a=0;a<p.nmax;a++)if(p.state[a])for(uint32_t b=0;b<p.nmax;b++)if(p.state[b]&&
  p.id[a]<p.id[b]&&sameGroup(p,a,b)&&within(p.y[a],p.y[b],radius)){
   if(!found||p.id[a]<bestA||(p.id[a]==bestA&&p.id[b]<bestB)){
    bestA=p.id[a];bestB=p.id[b];slotA=a;slotB=b;found=true;}}
 return found;
}
static bool environment(){const double d=std::numeric_limits<double>::denorm_min();
 return littleEndian()&&std::numeric_limits<double>::is_iec559&&std::fegetround()==FE_TONEAREST&&
   d>0&&d*2==std::numeric_limits<double>::min()*std::pow(2.0,-51);}
static void addArray(Sha256& sha,uint32_t id,const void* ptr,size_t bytes){sha.u32(id);sha.u32(uint32_t(bytes));sha.update(ptr,bytes);}
static void addString(Sha256& sha,const std::string& s){sha.u32(uint32_t(s.size()));sha.update(s.data(),s.size());}
static std::array<uint8_t,32> snapshotDigest(const SnapshotView& p){Sha256 sha;
 sha.u32(p.nmax);sha.u32(p.cols);sha.u32(p.liveCount);sha.u32(p.freeCount);sha.u64(bits64(p.nextId));
 addArray(sha,1,p.mass,size_t(p.nmax)*8);addArray(sha,2,p.x,size_t(p.nmax)*8);
 addArray(sha,3,p.y,size_t(p.nmax)*8);addArray(sha,4,p.vx,size_t(p.nmax)*8);
 addArray(sha,5,p.vy,size_t(p.nmax)*8);addArray(sha,6,p.phase,p.nmax);
 addArray(sha,7,p.source,p.nmax);addArray(sha,8,p.state,p.nmax);
 addArray(sha,9,p.id,size_t(p.nmax)*4);addArray(sha,10,p.sourceId,size_t(p.nmax)*4);
 addArray(sha,11,p.freeSlots,size_t(p.nmax)*4);addArray(sha,12,p.colMassH,size_t(p.cols)*8);
 addArray(sha,13,p.colMassL,size_t(p.cols)*8);return sha.finish();}
static void copyView(const SnapshotView& p,CpuSlot& slot){
 auto doubles=[&](std::vector<double>& to,const double* from,uint32_t n){std::copy(from,from+n,to.begin());};
 doubles(slot.fMass,p.mass,p.nmax);doubles(slot.fX,p.x,p.nmax);doubles(slot.fY,p.y,p.nmax);
 doubles(slot.fVx,p.vx,p.nmax);doubles(slot.fVy,p.vy,p.nmax);
 doubles(slot.fColH,p.colMassH,p.cols);doubles(slot.fColL,p.colMassL,p.cols);
 std::copy(p.phase,p.phase+p.nmax,slot.fPhase.begin());std::copy(p.source,p.source+p.nmax,slot.fSource.begin());
 std::copy(p.state,p.state+p.nmax,slot.fState.begin());std::copy(p.id,p.id+p.nmax,slot.fId.begin());
 std::copy(p.sourceId,p.sourceId+p.nmax,slot.fSourceId.begin());
 std::copy(p.freeSlots,p.freeSlots+p.nmax,slot.fFree.begin());
 slot.frozen=p;slot.frozen.mass=slot.fMass.data();slot.frozen.x=slot.fX.data();slot.frozen.y=slot.fY.data();
 slot.frozen.vx=slot.fVx.data();slot.frozen.vy=slot.fVy.data();slot.frozen.phase=slot.fPhase.data();
 slot.frozen.source=slot.fSource.data();slot.frozen.state=slot.fState.data();slot.frozen.id=slot.fId.data();
 slot.frozen.sourceId=slot.fSourceId.data();slot.frozen.freeSlots=slot.fFree.data();
 slot.frozen.colMassH=slot.fColH.data();slot.frozen.colMassL=slot.fColL.data();
}
PrepareStatus prepareExactRanges(const SnapshotView& p,double radius,const RequestIdentity& id,CpuSlot& slot,PreparedView& out){
 slot.prepared=false;out=PreparedView();
 if(!environment())return PrepareStatus::ENV_UNSUPPORTED;
 if(!std::isfinite(radius)||radius<0||p.nmax==0||p.nmax!=slot.nmax||p.cols==0||p.cols>128||
  !p.mass||!p.x||!p.y||!p.vx||!p.vy||!p.phase||!p.source||!p.state||!p.id||!p.sourceId||
  !p.freeSlots||!p.colMassH||!p.colMassL||p.liveCount>p.nmax||p.freeCount>p.nmax||
  p.freeCount!=p.nmax-p.liveCount||!std::isfinite(p.nextId)||p.nextId<1||
  p.nextId>9007199254740991.0||std::floor(p.nextId)!=p.nextId||
  id.runId.empty()||id.stateId.empty()||id.configId.empty()||
  id.buildManifestSha.empty()||id.ownerGeneration==0||id.contextEpoch==0||id.payloadSerial==0||
  id.requestSerial==0||id.candidateRole<1||id.candidateRole>5||id.slotIndex>1||id.flags>1||
  !std::isfinite(id.paramsRevision)||!std::isfinite(id.inputRevision)||!std::isfinite(id.seed))return PrepareStatus::INPUT_INVALID;
 const size_t need[13]={size_t(p.nmax)*8,size_t(p.nmax)*8,size_t(p.nmax)*8,
  size_t(p.nmax)*8,size_t(p.nmax)*8,p.nmax,p.nmax,p.nmax,size_t(p.nmax)*4,
  size_t(p.nmax)*4,size_t(p.nmax)*4,size_t(p.cols)*8,size_t(p.cols)*8};
 const void* fields[13]={p.mass,p.x,p.y,p.vx,p.vy,p.phase,p.source,p.state,p.id,p.sourceId,
  p.freeSlots,p.colMassH,p.colMassL};
 for(int a=0;a<13;a++){
  if(p.byteLength[a]!=need[a])return PrepareStatus::INPUT_INVALID;
  uintptr_t begin=reinterpret_cast<uintptr_t>(fields[a]);
  if(need[a]>std::numeric_limits<uintptr_t>::max()-begin)return PrepareStatus::INPUT_INVALID;
  for(int b=0;b<a;b++){
   uintptr_t other=reinterpret_cast<uintptr_t>(fields[b]);
   if(begin<other+need[b]&&other<begin+need[a])return PrepareStatus::INPUT_INVALID;
  }
 }
 std::fill(slot.freeSeen.begin(),slot.freeSeen.end(),0);
 for(uint32_t j=0;j<p.freeCount;j++){int32_t freeSlot=p.freeSlots[j];
  if(freeSlot<0||uint32_t(freeSlot)>=p.nmax||p.state[freeSlot]!=0||slot.freeSeen[freeSlot])return PrepareStatus::INPUT_INVALID;
  slot.freeSeen[freeSlot]=1;
 }
 for(uint32_t c=0;c<p.cols;c++)if(!std::isfinite(p.colMassH[c])||!std::isfinite(p.colMassL[c]))return PrepareStatus::INPUT_INVALID;
 uint32_t n=0;for(uint32_t i=0;i<p.nmax;i++){
  if(!std::isfinite(p.mass[i])||!std::isfinite(p.x[i])||!std::isfinite(p.y[i])||
     !std::isfinite(p.vx[i])||!std::isfinite(p.vy[i]))return PrepareStatus::INPUT_INVALID;
  if(p.phase[i]>1||p.source[i]>1||p.state[i]>2||
    (p.state[i]==0)!=bool(slot.freeSeen[i]))return PrepareStatus::INPUT_INVALID;
  if(p.state[i]){if(p.id[i]==0||p.mass[i]<=0)return PrepareStatus::INPUT_INVALID;
    slot.order[n]=i;slot.idOrder[n]=i;n++;}
 }
 if(n!=p.liveCount)return PrepareStatus::INPUT_INVALID;
 copyView(p,slot);const SnapshotView& f=slot.frozen;slot.n=n;
 std::sort(slot.order.begin(),slot.order.begin()+n,[&](uint32_t a,uint32_t b){
  if(groupLess(f,a,b))return true;if(groupLess(f,b,a))return false;
  if(f.y[a]!=f.y[b])return f.y[a]<f.y[b];if(f.id[a]!=f.id[b])return f.id[a]<f.id[b];return a<b;});
 std::sort(slot.idOrder.begin(),slot.idOrder.begin()+n,[&](uint32_t a,uint32_t b){
  if(f.id[a]!=f.id[b])return f.id[a]<f.id[b];return a<b;});
 for(uint32_t i=1;i<n;i++)if(f.id[slot.idOrder[i]]==f.id[slot.idOrder[i-1]])return PrepareStatus::INPUT_INVALID;
 std::fill(slot.queries.begin(),slot.queries.end(),Query());
 for(uint32_t i=0;i<slot.K;i++)slot.leaves[i]=i<n?Leaf{f.id[slot.order[i]],slot.order[i],1,i}:Leaf{0,0,0,i};
 uint32_t G0=0;
 while(G0<n){uint32_t G1=G0+1;while(G1<n&&sameGroup(f,slot.order[G0],slot.order[G1]))G1++;
  for(uint32_t a=G0;a<G1;a++){
   const double ya=f.y[slot.order[a]];uint32_t l=G0,r=G1;
   while(l<r){uint32_t mid=l+(r-l)/2;if(f.y[slot.order[mid]]<=ya)l=mid+1;else r=mid;}uint32_t split=l;
   l=G0;r=split;while(l<r){uint32_t mid=l+(r-l)/2;
    if(within(ya,f.y[slot.order[mid]],radius))r=mid;else l=mid+1;}uint32_t L=l;
   l=split;r=G1;while(l<r){uint32_t mid=l+(r-l)/2;
    if(within(ya,f.y[slot.order[mid]],radius))l=mid+1;else r=mid;}uint32_t R=l;
   if(!(G0<=L&&L<=a&&a<R&&R<=G1))return PrepareStatus::INPUT_INVALID;
   slot.queries[a]=Query{f.id[slot.order[a]],slot.order[a],a,L,R,G0,G1,1};
  }G0=G1;
 }
 Header& head=slot.header;head=Header();auto& w=head.w;
 w[0]=kHeaderMagic;w[1]=1;w[2]=256;w[3]=128;w[4]=w[5]=w[6]=1;w[7]=id.flags;
 w[8]=p.nmax;w[9]=n;w[10]=slot.K;w[11]=slot.h;
 split64(id.ownerGeneration,w[12],w[13]);split64(id.contextEpoch,w[14],w[15]);
 split64(id.payloadSerial,w[16],w[17]);split64(id.poolMutationSerial,w[18],w[19]);
 split64(id.attemptSerial,w[20],w[21]);split64(id.substepSerial,w[22],w[23]);
 w[24]=id.candidateRole;w[25]=id.candidateOrdinal;w[26]=id.slotIndex;w[27]=64;
 w[28]=w[12];w[29]=w[13];split64(id.requestSerial,w[30],w[31]);
 const auto snapshotSha=snapshotDigest(f);shaWords(snapshotSha,w.data()+32);
 Sha256 payload;payload.update(slot.leaves.data(),size_t(slot.K)*sizeof(Leaf));
 payload.update(slot.queries.data(),size_t(slot.K)*sizeof(Query));const auto payloadSha=payload.finish();shaWords(payloadSha,w.data()+40);
 slot.identityBytes.clear();
 if(id.identityEncodingRevision==kIdentityEncodingV2){
  if(!canonicalIdentityV2(head,id,radius,snapshotSha,payloadSha,slot.identityBytes))return PrepareStatus::INPUT_INVALID;
  Sha256 identity;identity.update(slot.identityBytes.data(),slot.identityBytes.size());shaWords(identity.finish(),w.data()+48);
 }else if(id.identityEncodingRevision=="T141_LEGACY_IDENTITY_ORDER_V1"){
 Sha256 identity;for(const char* revision:{"GPU_SEGMENTED_ID_PAIR_V1","CPU_RN64_MERGE_RANGE_V1",
   "P011_GPU_PAIR_WIRE_V1","P011_GPU_PAIR_PROTOTYPE_V1"})addString(identity,revision);
 addString(identity,id.runId);addString(identity,id.stateId);addString(identity,id.configId);
 identity.u64(bits64(id.paramsRevision));identity.u64(bits64(id.inputRevision));identity.u64(bits64(id.seed));
 for(uint32_t k=12;k<32;k++)identity.u32(w[k]);
 identity.u32(p.nmax);identity.u32(n);identity.u32(slot.K);identity.u32(slot.h);identity.u32(64);identity.u32(id.flags);
 identity.u64(bits64(radius));identity.update(snapshotSha.data(),32);identity.update(payloadSha.data(),32);
 addString(identity,id.buildManifestSha);shaWords(identity.finish(),w.data()+48);
 }else return PrepareStatus::INPUT_INVALID;
 slot.result=Result();slot.result.w[0]=kResultMagic;slot.result.w[1]=1;
 out.leaves=slot.leaves.data();out.queries=slot.queries.data();out.header=&slot.header;
 out.n=n;out.K=slot.K;out.h=slot.h;out.radius=radius;out.identity=id;out.snapshot=&slot.frozen;
 out.mutationAtFreeze=id.poolMutationSerial;slot.prepared=true;
 return n<2?PrepareStatus::NO_PAIR:PrepareStatus::OK;
}
}
