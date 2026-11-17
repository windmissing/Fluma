#pragma once
#include "research_config.h"
#include "touch_adapter.h"
namespace grs::g9dm::candidate548 {
// T548 seed R2 (problem T552 G9 seed manifest v2): column9=40Q now fixed
// (canonicalAssignments.columnEvery32Words[9]=41943040), parameterRevision 3,
// seedRevision 2; all 64 config words and the seed template come verbatim from
// docs/architecture-T552-g9-seed-manifest-v2.json (G9_SEED_S40_ORIGIN1_R2).
inline constexpr std::array<uint32_t,64> kConfig={843266375,2,826691667,512,536870912,32768,24576,262144,262144,65536,576,3,7,2,53401,1,2016024897,2515985490,924096203,2197582556,1973154206,2196991564,284700942,637129275,1782254967,1574685276,2266489603,81830087,3718613522,2935158059,4102980319,2640608833,1905024894,2013720932,1728381989,791044997,2696668310,891730736,3810132939,2702886615,1781737610,301268279,828469598,2799693599,3935563064,3182723658,1165815609,2951438695,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
inline bool Decode(const std::array<uint32_t,64>& raw,Config& out){
 if(raw!=kConfig)return false;
 Config c;c.k=raw[3];c.g=raw[4];c.muStart=raw[5];c.muStop=raw[6];c.band=raw[7];c.entrainRate=raw[8];c.shear=raw[9];c.settleTicks=raw[10];c.revision=raw[11];c.research=c.synthetic=true;c.parameterSha=HashWords(raw.data(),raw.size());if(!c.Valid())return false;out=c;return true;
}
inline bool FillSeed(uint32_t* state,size_t words){
 const auto* p=SelectProfile(512);if(!state||!p||words!=673792/4||words!=p->stateBytes/4)return false;std::memset(state,0,words*4);
state[0]=827537735u;
state[1]=1u;
state[2]=826691667u;
state[18]=1u;
state[19]=1u;
state[21]=536870912u;
state[23]=536870912u;
state[24]=512u;
state[25]=128u;
state[26]=64u;
state[27]=64u;
state[40]=1073741824u;
state[41]=1u;
state[98]=32768u;
state[99]=24576u;
state[100]=262144u;
state[101]=262144u;
state[102]=65536u;
state[103]=576u;
for(size_t col=0;col<128;++col){
state[p->state[3].begin/4+col*32+0]=41943040u;
state[p->state[3].begin/4+col*32+9]=41943040u;
state[p->state[3].begin/4+col*32+6]=1u;
state[p->state[4].begin/4+col*64*4+0]=1u;
state[p->state[4].begin/4+col*64*4+1]=0u;
state[p->state[4].begin/4+col*64*4+2]=1u;
state[p->state[4].begin/4+col*64*4+3]=41943040u;
}
state[p->state[7].begin/4+0]=1u;
state[p->state[7].begin/4+1]=0u;
state[p->state[7].begin/4+2]=1u;
state[p->state[7].begin/4+3]=0u;
state[p->state[7].begin/4+4]=1073741824u;
state[p->state[7].begin/4+5]=1u;
state[p->state[7].begin/4+6]=0u;
state[p->state[7].begin/4+7]=0u;
state[p->state[7].begin/4+8]=0u;
state[p->state[7].begin/4+9]=0u;
state[p->state[7].begin/4+10]=0u;
state[p->state[7].begin/4+11]=0u;
state[p->state[7].begin/4+12]=0u;
state[p->state[7].begin/4+13]=0u;
state[p->state[7].begin/4+14]=1u;
state[p->state[7].begin/4+15]=0u;
return true;
}
}
