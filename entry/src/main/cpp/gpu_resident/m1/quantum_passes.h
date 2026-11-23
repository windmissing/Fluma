#pragma once
#include <cstdint>

namespace grs::m1 {
// T237 candidate values pending calibration.
struct QuantumParameters {
 uint32_t gravity=512u*1048576u,settle=8u*1048576u,kAir=0,kFlight=0,kWet=24,kappa=24,soft=557537,flux=24u*1048576u;
 int32_t requestedAngle=0;
};
struct QuantumPassOp {uint32_t stage,n,color,sortK,sortJ,bind3,out;};

// Original FullM1 dispatch order, shared by GLES and Vulkan.
template<class Emit>
void EachQuantumPass(uint32_t words,uint32_t pad,uint32_t kw,Emit emit){
 auto pass=[&](uint32_t stage,uint32_t count,uint32_t color=UINT32_MAX){
  emit({stage,count,color,0,0,0,0});
 };
 auto sort=[&](uint32_t stage,uint32_t count){
  for(uint32_t k=2;k<=count;k*=2)
   for(uint32_t j=k/2;j;j/=2)emit({stage,count,UINT32_MAX,k,j,0,0});
 };
 pass(0,words);pass(1,1);
 for(uint32_t c=0;c<2;++c){pass(2,127,c);pass(3,1,c);pass(4,127,c);}
 pass(5,128);pass(6,pad);sort(7,pad);pass(8,kw);sort(9,kw);pass(10,1);pass(11,1);
 for(uint32_t c=0;c<3;++c){pass(12,127,c);pass(13,1,c);pass(14,127,c);}
 pass(15,1);pass(16,kw);sort(17,kw);pass(18,1);pass(19,1);pass(20,words);
 pass(21,1);pass(22,words);pass(23,1);pass(24,1);
 emit({28,words,UINT32_MAX,0,0,1,1});
 emit({25,1,UINT32_MAX,0,0,2,2});pass(26,1);
 emit({27,1,UINT32_MAX,0,0,3,3});
}
}
