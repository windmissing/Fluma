#include "source_owner.h"
namespace grs {namespace m1 {namespace {
U64 qr(const uint32_t* p,size_t i){return {p[i],p[i+1]};}bool qm(const uint32_t* p,size_t i,U64 v){return Compare64(qr(p,i),v)==0;}bool qp(U64 a,uint32_t d,U64 b){U64 v;return Add64(a,{d,0},v)&&Compare64(v,b)==0;}
}
ControlCode SourceRootOwner::BeginQuantum(const RootRequest& r,int angle){
 if(state==FlightState::StateLost)return ControlCode::OWNER_STATE_LOST;if(state==FlightState::Quarantined)return ControlCode::QUARANTINED;if(state!=FlightState::Idle)return ControlCode::BUSY;
 uint32_t pad=K==192?256:512;auto l=[](uint32_t n){uint32_t h=0;while(n>1){h++;n/=2;}return h*(h+1)/2;};uint32_t passes=35+l(pad)+2*l(Kw),capacity=4*(48*K+16*Kw+254)+K;
 if(r.kind!=2||r.quantumOrdinal<1||r.quantumOrdinal>4||r.publicationSlot>1||r.eventArena>1||r.eventStart>r.eventCapacity||r.eventCapacity!=capacity||r.dispatchCount!=passes||r.barrierCount!=passes||r.carry>=1000000000||!(r.frame.lo|r.frame.hi)||!(r.control.lo|r.control.hi)||Compare64(r.nonce,lastNonce)<=0||angle< -7680||angle>7680)return ControlCode::INVALID_INPUT;
 if(pubs[r.publicationSlot].leased||pubs[r.publicationSlot].releasePending)return ControlCode::BACKPRESSURE;
 pending=r;quantumAngle=angle;lastNonce=r.nonce;tombstone=false;state=FlightState::Submitted;pubs[r.publicationSlot].available=false;return ControlCode::OK;
}
bool SourceRootOwner::ValidQuantum(const uint32_t* c,size_t n,uint32_t gl)const{
 if(!c||n!=256||gl||c[0]!=1381192263||c[1]!=2||c[2]!=256||c[3]!=3235823838u||c[26]!=pending.quantumOrdinal||c[27]!=2||c[28]!=1||c[29]||c[30]||c[31]||c[97]!=255||c[98]||c[99]!=1023||c[100]||c[101]!=UINT32_MAX||c[102]||c[103]||c[104]||c[105]||c[106]||c[109]||c[113])return false;
 for(size_t i=59;i<=68;i++)if(c[i])return false;for(size_t i=160;i<256;i++)if(c[i])return false;
 for(auto p:{std::array<size_t,2>{6,4},{8,6},{10,8},{12,10},{38,20},{40,22}})if(!qm(c,p[0],qr(header.data(),p[1])))return false;
 if(!qm(c,4,pending.nonce)||!qm(c,22,pending.control)||!qm(c,24,pending.frame)||!qm(c,14,qr(header.data(),14))||!qm(c,18,qr(header.data(),14))||!qm(c,16,qr(header.data(),16))||!qp(qr(header.data(),16),1,qr(c,20)))return false;
 for(size_t i=0;i<9;i++)if(c[42+i]!=header[24+i])return false;for(size_t i=0;i<24;i++)if(c[114+i]!=header[88+i])return false;
 if(c[32]!=bank||c[33]!=1-bank||c[34]!=2||c[35]!=pending.publicationSlot||!qp(qr(header.data(),72),1,qr(c,36))||c[51]!=pending.eventArena||c[52]!=pending.eventStart||c[53]<c[52]||c[53]>c[54]||c[54]!=pending.eventCapacity||c[55]!=header[34]||c[56]!=header[34]||c[57]!=header[33]||c[58]!=header[33])return false;
 for(auto p:{std::array<size_t,2>{69,56},{71,56},{73,58},{75,58},{77,60},{79,62},{138,112},{142,114},{146,116}})if(!qm(c,p[0],qr(header.data(),p[1])))return false;
 U64 before,after;if(!Add64(qr(c,138),qr(c,142),before)||!Add64(qr(c,140),qr(c,144),after)||Compare64(before,after)!=0||Compare64(after,qr(c,75))!=0)return false;
 if(!qp(qr(c,146),c[156],qr(c,148))||!qp(qr(header.data(),118),1,qr(c,150))||Compare64(qr(c,150),qr(c,152))!=0||Compare64(qr(c,140),qr(c,138))<0||Compare64(qr(c,144),qr(c,142))>0||(c[156]==0&&(Compare64(qr(c,140),qr(c,138))!=0||Compare64(qr(c,144),qr(c,142))!=0))||(c[156]>0&&Compare64(qr(c,140),qr(c,138))<=0))return false;
 if(c[82]||c[84]||c[81]>2621440||c[83]!=2621440-c[81]||int32_t(c[85])< -605394||int32_t(c[86])>605394||int32_t(c[85])>int32_t(c[86])||c[87]>268435456||c[88]>K||c[89]!=K-c[88]||c[90]>Kw||c[91]!=Kw-c[90]||c[92]!=header[39]||c[93]!=header[40]||c[96]||int32_t(c[107])< -7680||int32_t(c[107])>7680||c[108]!=pending.carry||c[110]!=pending.dispatchCount||c[111]!=pending.barrierCount||c[112]!=pending.uploadedBytes||c[154]>16*Kw||c[155]>48*K||c[156]>Kw||c[157]>127||c[158]||c[159])return false;
 if(c[94]>48*K+16*Kw||c[95]>127||uint64_t(c[94])+c[95]+c[157]!=uint64_t(c[53])-c[52]||c[154]>c[94]||c[155]>c[94]||c[156]>c[154])return false;
 return true;
}
}}
