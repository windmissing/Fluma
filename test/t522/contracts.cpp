#include "m3a/work_layout.h"
#include "m3a/buffer_plan.h"
#include "m3a/semantic_schema.h"
#include <cassert>
#include <iostream>
int main(){
 using namespace grs::m3a;
 for(const auto& p:kProfiles){
  assert(p.passCount==5*(p.k==192?(p.kw==64?113:127):(p.kw==64?122:136))-16);
  for(size_t i=0;i<p.passCount;++i)assert(p.passes[i].role<=5);
  for(auto alignment:{16u,64u,256u,4096u}){ResidentBuffersV4 buffers;assert(ResidentBuffersV4::Make(p,alignment,buffers));assert(buffers.total==p.payloadBytes+buffers.padding);WorkLayout l;assert(WorkLayout::Make(p.k,p.kw,alignment,l));assert(l.combinedEventOffset%alignment==0);assert(l.combinedEventOffset>=p.stateBytes);assert(l.combinedBytes-l.combinedEventOffset==size_t(p.scopeRows)*256);assert(l.Supports(1u<<30,4,4,1,64,4096));assert(!l.Supports(4096,4,4,1,64,4096));}
  assert(p.source[3].End()==p.sourceWorkBytes&&p.quantum[2].End()==p.quantumWorkBytes);
 }
 for(size_t word=0;word<256;++word){const bool representation=RepresentationHeaderWord(word);assert(representation==(word==76||word==77||word==78||word==152||word==153));}
 for(size_t word=0;word<64;++word)assert(QuantumSemanticMask(word)==(word==52?~(7u<<22u):UINT32_MAX));
 WorkLayout bad;assert(!WorkLayout::Make(192,64,0,bad));assert(!WorkLayout::Make(128,64,256,bad));
 std::cout<<"T522 formal profiles, four binding pass tables, checked WORK/combined layout PASS\n";
}
