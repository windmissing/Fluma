#include "source_owner.h"
#include <algorithm>
namespace grs {namespace m1 {namespace {
U64 read(const uint32_t* p,size_t i){return {p[i],p[i+1]};}bool eq(U64 a,U64 b){return Compare64(a,b)==0;}bool match(const uint32_t* p,size_t i,U64 v){return eq(read(p,i),v);}void put(Words256& p,size_t i,U64 v){p[i]=v.lo;p[i+1]=v.hi;}bool plus(U64 a,uint32_t q,U64 b){U64 v;return Add64(a,{q,0},v)&&eq(v,b);}
}
ControlCode SourceRootOwner::Initialize(const Identity& id,uint32_t k,uint32_t kw){
 if((k!=192&&k!=512)||(kw!=64&&kw!=128))return ControlCode::UNSUPPORTED_FEATURE;
 if(state==FlightState::Submitted||state==FlightState::Quarantined)return ControlCode::BUSY;for(const auto& p:pubs)if(p.leased||p.releasePending)return ControlCode::BACKPRESSURE;
 for(U64 v:{id.run,id.owner,id.context,id.surface,id.geometry,id.config})if(!(v.lo|v.hi))return ControlCode::INVALID_INPUT;
 identity=id;K=k;Kw=kw;fullWaterContract=false;header={};header[0]=1213420103;header[1]=2;header[2]=256;put(header,4,id.run);put(header,6,id.owner);put(header,8,id.context);put(header,10,id.surface);put(header,20,id.geometry);put(header,22,id.config);
 for(size_t i=24;i<=26;i++)header[i]=1;for(size_t i=27;i<=30;i++)header[i]=2;header[31]=3;header[39]=header[40]=1;header[43]=K;header[44]=Kw;header[77]=1;header[78]=2;header[84]=1;header[122]=header[124]=header[125]=1;header[123]=65536;
 for(size_t i=0;i<8;i++){header[88+i]=id.shaderSha[i];header[96+i]=id.angleSha[i];header[104+i]=id.layoutSha[i];}
 bank=0;lastNonce={};pubs[0]={};pubs[1]={};tombstone=false;mapped=accepted=0;state=FlightState::Idle;return ControlCode::OK;
}
ControlCode SourceRootOwner::Begin(const RootRequest& r,const std::array<uint32_t,32>& c){
 if(state==FlightState::StateLost)return ControlCode::OWNER_STATE_LOST;if(state==FlightState::Quarantined)return ControlCode::QUARANTINED;if(state!=FlightState::Idle)return ControlCode::BUSY;
 if(r.kind!=1||r.quantumOrdinal||r.publicationSlot>1||r.eventArena>1||r.eventStart||r.eventCapacity!=K+1||r.dispatchCount!=8||r.barrierCount!=9||r.carry>=1000000000||!(r.frame.lo|r.frame.hi)||!(r.control.lo|r.control.hi)||Compare64(r.nonce,lastNonce)<=0)return ControlCode::INVALID_INPUT;
 const auto& p=pubs[r.publicationSlot];if(p.leased||p.releasePending)return ControlCode::BACKPRESSURE;
 pending=r;command=c;lastNonce=r.nonce;tombstone=false;state=FlightState::Submitted;return ControlCode::OK;
}
bool SourceRootOwner::Valid(const uint32_t* c,size_t words,uint32_t error)const{
 if(!c||words!=256||error||c[0]!=1381192263||c[1]!=2||c[2]!=256||c[3]!=3235823838u||c[26]||c[27]!=1||c[28]<1||c[28]>3||c[29]>22||c[30]||c[31]||c[97]!=255||c[98]||c[99]!=1023||c[100]||c[101]!=UINT32_MAX||c[102]||c[103]||c[104]||c[105]||c[109]||c[113])return false;
 for(size_t i=160;i<256;i++)if(c[i])return false;
 if(!match(c,4,pending.nonce)||!match(c,6,identity.run)||!match(c,8,identity.owner)||!match(c,10,identity.context)||!match(c,12,identity.surface)||!match(c,22,pending.control)||!match(c,24,pending.frame)||!match(c,38,identity.geometry)||!match(c,40,identity.config))return false;
 for(size_t i=0;i<9;i++)if(c[42+i]!=header[24+i])return false;for(size_t i=0;i<24;i++)if(c[114+i]!=header[88+i])return false;
 if(!match(c,14,read(header.data(),14))||!match(c,16,read(header.data(),16))||c[32]!=bank||c[33]!=1-bank||c[34]!=2||c[35]!=pending.publicationSlot||!plus(read(header.data(),72),1,read(c,36))||c[51]!=pending.eventArena||c[52]||c[53]>c[54]||c[54]!=K+1||c[55]!=header[34]||c[57]!=header[33]||c[56]>4096||c[58]>4096||c[56]<c[55]||c[56]-c[55]>1||c[58]<c[57]||c[58]-c[57]>1)return false;
 if(c[59]!=command[14]||c[60]!=command[15]||c[88]>K||c[89]!=K-c[88]||c[90]>Kw||c[91]!=Kw-c[90]||c[87]>268435456||c[94]||c[95]||c[96]!=c[53]||c[107]!=header[45]||c[108]!=pending.carry||c[110]!=8||c[111]!=9||c[112]!=pending.uploadedBytes||c[155]||c[156]||c[157]||c[158]>2||c[61]>268435456||c[62]>268435456||uint64_t(c[61])+c[62]>268435456)return false;
 for(auto p:std::array<std::array<size_t,2>,6>{{{{69,56}},{{73,58}},{{138,112}},{{142,114}},{{146,116}},{{150,118}}}})if(!match(c,p[0],read(header.data(),p[1])))return false;
 if(!match(c,148,read(header.data(),116))||!match(c,152,read(header.data(),120))||!match(c,83,read(header.data(),54))||c[81]||c[82]||c[85]||c[86])return false;
 if(c[28]==1){
  uint32_t phase=command[2],mode=c[158],births=c[66],q=c[61];if(phase>1||q==0||c[29]||c[64]||c[65]||c[106]||c[63]<1||c[63]>2||(c[63]==1&&c[62])||(c[63]==2&&!c[62])||c[56]!=c[55]+1||c[58]!=c[57]+1||!plus(read(c,14),1,read(c,18))||!plus(read(c,16),1,read(c,20)))return false;
  if((phase==0&&mode!=0)||(phase==1&&(mode<1||mode>2))||c[53]!=(mode==1?1:births)||c[154]!=(phase==1?c[53]:0)||c[159]!=(phase==1?c[67]:0))return false;
  for(uint32_t k=0;k<2;k++){uint32_t add=k==phase?q:0;if(!plus(read(header.data(),56+2*k),add,read(c,71+4*k))||!plus(read(header.data(),60+2*k),add,read(c,77+2*k)))return false;}
  if(!plus(read(header.data(),112),mode==1?q:0,read(c,140))||!plus(read(header.data(),114),mode==2?q:0,read(c,144)))return false;
  uint32_t before=header[39+phase];if(mode==1){if(births||c[67]||c[68]!=before||c[92]!=header[39]||c[93]!=header[40]||c[88]!=header[41]||c[90]!=header[42])return false;}
  else {if(!before||!births||births!=(phase==0?(q+131071)/131072:1)||uint64_t(births-1)>uint64_t(UINT32_MAX)-before)return false;uint64_t last=uint64_t(before)+births-1;uint32_t after=last==UINT32_MAX?(fullWaterContract&&phase==1?UINT32_MAX:0):uint32_t(last+1);if(fullWaterContract&&phase==1&&header[127])return false;if(c[67]!=before||c[68]!=after||c[92+phase]!=after||c[92+1-phase]!=header[40-phase]||c[88+2*phase]!=header[41+phase]+births||c[88+2*(1-phase)]!=header[42-phase])return false;}
 }else{
  if(!match(c,18,read(c,14))||!match(c,20,read(c,16))||c[106]!=1||c[53]||c[56]!=c[55]||c[88]!=header[41]||c[90]!=header[42]||c[92]!=header[39]||c[93]!=header[40])return false;
  for(auto p:std::array<std::array<size_t,2>,6>{{{{71,56}},{{75,58}},{{77,60}},{{79,62}},{{140,112}},{{144,114}}}})if(!match(c,p[0],read(header.data(),p[1])))return false;
  if(c[28]==2){if(c[63]!=3||c[61]||c[66]||c[65]||c[64]!=c[29]||c[58]-c[57]>1)return false;}
  else if(c[65]!=1||c[58]!=c[57]||c[29]||c[63]<1||c[63]>3||c[64]>22||(command[2]==0&&c[158]!=0)||(command[2]==1&&c[63]!=3&&c[158]==0)||(c[63]==3&&(c[61]||c[66]))||c[154]||(command[2]==1&&c[159]!=c[67]))return false;
 }
 return true;
}
ControlCode SourceRootOwner::Poll(FenceState fence,const uint32_t* c,size_t n,uint32_t error,AcceptedRoot& out){
 out={};if(state==FlightState::StateLost)return ControlCode::OWNER_STATE_LOST;if(state==FlightState::Idle)return ControlCode::STALE;if(fence==FenceState::Pending)return tombstone?ControlCode::QUARANTINED:ControlCode::BUSY;if(fence==FenceState::Failed||fence==FenceState::Timeout){tombstone=true;state=FlightState::Quarantined;return ControlCode::QUARANTINED;}if(tombstone){tombstone=false;state=FlightState::Idle;return ControlCode::STALE;}state=FlightState::Idle;if(n!=256||!c)return ControlCode::COMPLETION_INVALID;mapped+=1024;if(!(pending.kind==2?ValidQuantum(c,n,error):Valid(c,n,error)))return ControlCode::COMPLETION_INVALID;
 out.bank=c[33];out.publication=c[35];out.outcome=c[28];out.source=read(c,18);out.physics=read(c,20);out.publicationGeneration=read(c,36);out.nonce=pending.nonce;out.sourceSuccess=pending.kind==1&&c[28]==1;std::copy(c,c+256,out.completion.begin());bank=out.bank;
 for(auto p:std::array<std::array<size_t,2>,15>{{{{12,4}},{{14,18}},{{16,20}},{{18,22}},{{49,24}},{{72,36}},{{56,71}},{{58,75}},{{60,77}},{{62,79}},{{112,140}},{{114,144}},{{116,148}},{{118,150}},{{120,152}}}})put(header,p[0],read(c,p[1]));
 header[33]=c[58];header[34]=c[56];header[38]=c[53];header[39]=c[92];header[40]=c[93];header[41]=c[88];header[42]=c[90];header[43]=c[89];header[44]=c[91];header[47]=c[108];header[74]=c[58];header[76]=bank;header[77]=1-bank;header[79]=1;header[80]=c[28];header[81]=2;header[126]=c[92]==0;header[127]=fullWaterContract?(header[127]||(pending.kind==1&&c[28]==1&&command[2]==1&&c[158]==2&&c[67]==UINT32_MAX)):c[93]==0;
 if(pending.kind==2){header[38]=c[53];header[45]=c[107];header[46]=uint32_t(quantumAngle);header[51]=pending.quantumOrdinal;header[52]=2621440;header[53]=0;header[54]=c[83];header[55]=0;header[79]=2;for(size_t i=0;i<4;i++)header[128+i]=c[154+i];}
 auto& pub=pubs[out.publication];pub={};pub.identity=identity;pub.run=identity.run;pub.context=identity.context;pub.surface=identity.surface;pub.source=out.source;pub.physics=out.physics;pub.control=pending.control;pub.frame=pending.frame;pub.nonce=pending.nonce;pub.angle=c[107];pub.available=true;pub.generation=out.publicationGeneration;pub.bank=bank;accepted++;return ControlCode::OK;
}
ControlCode SourceRootOwner::Stop(){if(state==FlightState::StateLost)return ControlCode::OWNER_STATE_LOST;if(state==FlightState::Submitted){tombstone=true;state=FlightState::Quarantined;}return state==FlightState::Quarantined?ControlCode::QUARANTINED:ControlCode::OK;}
void SourceRootOwner::ContextLost(){state=FlightState::StateLost;tombstone=true;pubs[0]={};pubs[1]={};}
ControlCode SourceRootOwner::Acquire(uint32_t slot,U64 gen,PublicationLease& out){if(state==FlightState::StateLost)return ControlCode::OWNER_STATE_LOST;if(slot>1)return ControlCode::INVALID_INPUT;auto& p=pubs[slot];if(!p.available||!eq(p.generation,gen))return ControlCode::STALE;if(p.leased||p.releasePending)return ControlCode::BACKPRESSURE;p.leased=true;out=p;return ControlCode::OK;}
ControlCode SourceRootOwner::Release(uint32_t slot,U64 gen,bool signaled){if(slot>1)return ControlCode::INVALID_INPUT;auto& p=pubs[slot];if(!eq(p.generation,gen)||(!p.leased&&!p.releasePending))return ControlCode::STALE;p.leased=false;p.releasePending=!signaled;return signaled?ControlCode::OK:ControlCode::QUARANTINED;}
}}
#include "quantum_owner.inc"
