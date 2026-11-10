#pragma once
#include "layout_generated.h"
#include "../control.h"
#include <algorithm>
#include <sstream>

namespace grs::m2 {
inline std::string RootTokenV3(const Words256& c){
 std::ostringstream out;out<<"GRS_GPU_ROOT_Q20_M2_V3";
 // Full scope, hashes, publication generation and the source summary are
 // serialized as integers; the token is independent of the scene token.
 for(size_t i=4;i<28;++i)out<<'|'<<c[i];
 for(size_t i=32;i<42;++i)out<<'|'<<c[i];
 for(size_t i=114;i<138;++i)out<<'|'<<c[i];
 for(size_t i=160;i<173;++i)out<<'|'<<c[i];
 return out.str();
}
// This owner validates a sealed tail only. GL calls and deadline/tombstone
// retirement belong to the submitter, outside the short accept transaction.
class RootGate {
 const Profile* profile=nullptr;
 Identity id{};
 Words256 header{};
 RootRequest active{};
 uint32_t bank=0;
 U64 lastNonce{};
 bool pending=false,revoked=false;
 static U64 Pair(const uint32_t* a,size_t i){return {a[i],a[i+1]};}
 static bool Eq(U64 a,U64 b){return Compare64(a,b)==0;}
 static bool Step(U64 before,uint32_t delta,U64 after){U64 sum;return Add64(before,{delta,0},sum)&&Eq(sum,after);}
 bool Match(const uint32_t* c,size_t at,U64 expected)const{return Eq(Pair(c,at),expected);}
public:
 RootGate(const Profile& p,const Identity& identity,const Words256& cold,uint32_t currentBank=0):profile(&p),id(identity),header(cold),bank(currentBank){}
 bool Begin(const RootRequest& root){
  if(pending||root.kind<1||root.kind>2||root.publicationSlot>1||root.eventArena!=(root.frame.lo&1u)||
     Compare64(root.nonce,lastNonce)<=0||!(root.control.lo|root.control.hi)||!(root.frame.lo|root.frame.hi)||root.carry>=1000000000)return false;
  if(root.kind==1&&(root.quantumOrdinal||root.eventStart||root.eventCapacity!=profile->sourceRows||root.dispatchCount!=13||root.barrierCount!=14))return false;
  if(root.kind==2&&(root.quantumOrdinal==0||root.quantumOrdinal>4||root.dispatchCount!=profile->quantumDispatches||root.barrierCount!=profile->quantumDispatches||root.eventStart>root.eventCapacity))return false;
  active=root;lastNonce=root.nonce;pending=true;revoked=false;return true;
 }
 void Revoke(){if(pending)revoked=true;}
 void Discard(){pending=false;revoked=false;}
 uint32_t Bank()const{return bank;}
 const Words256& Header()const{return header;}
 bool Inspect(const uint32_t* c,size_t n,uint32_t gl,AcceptedRoot& out)const{
  out={};if(!pending||revoked||!c||n!=256||gl)return false;
  const auto& r=active;const bool source=r.kind==1;
  if(c[0]!=1381192263||c[1]!=3||c[2]!=256||c[3]!=3235823838u||c[26]!=r.quantumOrdinal||c[27]!=r.kind||
     c[28]<1||c[28]>(source?3u:1u)||c[30]||c[31]||c[32]!=bank||c[33]!=1-bank||c[34]!=2||c[35]!=r.publicationSlot||
     c[97]!=(source?8191u:255u)||c[98]||c[99]!=1023||c[100]||c[101]!=UINT32_MAX||c[102]||c[103]||c[104]||c[105]||c[109]||c[113]||
     c[110]!=r.dispatchCount||c[111]!=r.barrierCount||c[112]!=r.uploadedBytes||c[108]!=r.carry)return false;
  for(size_t i=173;i<256;++i)if(c[i])return false;
  if(!Match(c,4,r.nonce)||!Match(c,6,id.run)||!Match(c,8,id.owner)||!Match(c,10,id.context)||!Match(c,12,id.surface)||
     !Match(c,22,r.control)||!Match(c,24,r.frame)||!Match(c,38,id.geometry)||!Match(c,40,id.config)||
     !Match(c,14,Pair(header.data(),14))||!Match(c,16,Pair(header.data(),16))||!Step(Pair(header.data(),72),1,Pair(c,36)))return false;
  for(size_t i=0;i<9;++i)if(c[42+i]!=header[24+i])return false;
  for(size_t i=0;i<8;++i)if(c[114+i]!=id.shaderSha[i]||c[122+i]!=id.angleSha[i]||c[130+i]!=id.layoutSha[i])return false;
  for(size_t i=0;i<5;++i)if(c[160+i]!=header[132+i])return false;
  const bool admits=source&&c[28]==1;
  const uint32_t delta=admits?1:0;
  if(!Step(Pair(c,14),delta,Pair(c,18))||!Step(Pair(c,16),source?delta:1,Pair(c,20))||
     c[55]!=header[34]||c[56]!=c[55]+delta||c[57]!=header[33]||c[58]>4096||c[56]>c[58]||
     c[58]<c[57]||c[58]-c[57]>(source?1u:0u)||c[88]>profile->k||c[89]!=profile->k-c[88]||
     c[90]>profile->kw||c[91]!=profile->kw-c[90]||c[87]>cfg_CQ)return false;
  if(!Match(c,69,Pair(header.data(),56))||!Match(c,73,Pair(header.data(),58))||
     !Match(c,138,Pair(header.data(),112))||!Match(c,142,Pair(header.data(),114))||!Match(c,146,Pair(header.data(),116))||
     !Match(c,71,Pair(c,77))||!Match(c,75,Pair(c,79)))return false;
  U64 water;if(!Add64(Pair(c,140),Pair(c,144),water)||!Eq(water,Pair(c,75)))return false;
  if(source){
   if(c[94]||c[95]||c[81]||c[82]||c[85]||c[86]||c[155]||c[156]||c[157]||c[158]>2||
      c[106]!=(admits?0u:1u)||c[107]!=header[45]||c[61]>cfg_CQ||c[62]>cfg_CQ||uint64_t(c[61])+c[62]>cfg_CQ||
      !Match(c,83,Pair(header.data(),54))||!Match(c,148,Pair(header.data(),116))||
      !Match(c,150,Pair(header.data(),118))||!Match(c,152,Pair(header.data(),120)))return false;
   if(c[51]||c[52]||c[53]||c[54]||c[168]!=128||c[169]==0||c[169]>profile->sourceRows||c[172]!=r.eventArena||
      c[165]>c[166]||c[166]>profile->k||c[167]>LEDGER_FULL||c[96]||
      !Step(Pair(header.data(),142),c[165]+c[166],Pair(c,170)))return false;
   if(admits){
    const uint32_t phase=c[158]==0?0:1,mode=c[158],births=c[166],before=header[39+phase];
    if((c[63]==1&&(c[62]||c[167]!=TARGET_COMPLETE))||(c[63]==2&&(!c[62]||c[167]<COLUMN_LIMIT||c[167]>SLOT_LIMIT))||
       c[154]!=(phase==1?births:0u)||c[159]!=(phase==1?c[67]:0u))return false;
    if(mode==1){
     if(births||c[165]||c[67]||c[68]!=before||c[92]!=header[39]||c[93]!=header[40]||c[88]!=header[41]||c[90]!=header[42])return false;
    }else{
     if(!before||!births||births!=(phase==0?(c[61]+cfg_package-1)/cfg_package:1u)||
        uint64_t(before)+births-1>UINT32_MAX)return false;
     const auto last=uint64_t(before)+births-1;
     const uint32_t after=last==UINT32_MAX?(phase==0?0u:UINT32_MAX):uint32_t(last+1);
     if(c[67]!=before||c[68]!=after||c[92+phase]!=after||c[92+1-phase]!=header[40-phase]||
        c[88+2*phase]!=header[41+phase]+births-c[165]||c[88+2*(1-phase)]!=header[42-phase])return false;
    }
    if(c[29]||c[65]||c[61]==0||c[61]>cfg_CQ||c[66]!=c[166]||c[63]<1||c[63]>2||
       c[64]!=c[167]||c[169]!=c[165]+c[166]+1u+(c[158]==1?1u:0u)||c[58]!=c[57]+1||c[158]>2)return false;
    if(!Step(Pair(c,69),c[158]==0?c[61]:0,Pair(c,71))||!Step(Pair(c,73),c[158]!=0?c[61]:0,Pair(c,75))||
       !Step(Pair(c,138),c[158]==1?c[61]:0,Pair(c,140))||!Step(Pair(c,142),c[158]==2?c[61]:0,Pair(c,144)))return false;
   }else{
    if(c[165]||c[166]||c[169]!=1||c[88]!=header[41]||c[90]!=header[42]||c[92]!=header[39]||c[93]!=header[40]||
       !Match(c,71,Pair(c,69))||!Match(c,75,Pair(c,73))||!Match(c,140,Pair(c,138))||!Match(c,144,Pair(c,142)))return false;
    if(c[28]==2&&(c[65]||c[61]||c[66]||c[63]!=3||c[64]!=c[29]))return false;
    if(c[28]==3&&(c[65]!=1||c[29]||c[167]!=DEDUP_REPLAY||c[58]!=c[57]))return false;
   }
  }else{
   if(c[29]||c[51]!=r.eventArena||c[52]!=r.eventStart||c[53]<c[52]||c[53]>c[54]||c[54]!=r.eventCapacity||
      c[92]!=header[39]||c[93]!=header[40]||!Match(c,71,Pair(c,69))||!Match(c,75,Pair(c,73))||
      !Step(Pair(header.data(),118),1,Pair(c,150))||!Match(c,152,Pair(c,150)))return false;
   for(size_t i=0;i<13;++i)if(c[160+i]!=header[132+i])return false;
  }
  out.bank=c[33];out.publication=c[35];out.outcome=c[28];out.source=Pair(c,18);out.physics=Pair(c,20);
  out.publicationGeneration=Pair(c,36);out.nonce=r.nonce;out.sourceSuccess=admits;
  std::copy(c,c+256,out.completion.begin());return true;
 }
 // Call Inspect and Accept under the same permit lock. Only a validated tail
 // may flip the authoritative bank and publication generation.
private:
 void Accept(const AcceptedRoot& out){
  const auto& c=out.completion;
  for(const auto& p:std::array<std::array<size_t,2>,15>{{{12,4},{14,18},{16,20},{18,22},{49,24},{72,36},{56,71},
       {58,75},{60,77},{62,79},{112,140},{114,144},{116,148},{118,150},{120,152}}}){
   header[p[0]]=c[p[1]];header[p[0]+1]=c[p[1]+1];
  }
  for(size_t i=0;i<13;++i)header[132+i]=c[160+i];
  header[33]=header[74]=c[58];header[34]=c[56];header[39]=c[92];header[40]=c[93];
  header[41]=c[88];header[42]=c[90];header[43]=c[89];header[44]=c[91];
  bank=out.bank;pending=false;
 }
public:
 bool Commit(const uint32_t* c,size_t n,uint32_t rawGl,AcceptedRoot& out){
  if(!Inspect(c,n,rawGl,out))return false;
  Accept(out);return true;
 }
};
}
