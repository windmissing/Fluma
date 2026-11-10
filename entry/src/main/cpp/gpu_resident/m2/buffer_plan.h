#pragma once
#include "layout_generated.h"
#include <array>

namespace grs::m2 {
// T445 retains the M1 isolation buffers and adds three separate SOURCE128
// arenas. A cold two-batch warmup temporarily owns one additional event64 arena.
constexpr std::array<uint32_t, 20> ResidentBufferBytes(const Profile& p){
 const uint32_t state=p.stateWords*4, publication=p.pubWords*4, source64=(p.k+1)*256;
 const uint32_t quantum64=(4*(48*p.k+16*p.kw+254)+p.k)*256;
 return {state,state,state,publication,publication,
         source64,source64,source64,quantum64,quantum64,quantum64,
         1024,1024,1024,1024,p.sourceBytes,p.quantumBytes,p.arenaBytes,p.arenaBytes,p.arenaBytes};
}
constexpr bool ValidResidentBuffers(){
 for(const auto& p:kProfiles){
  uint64_t total=0;
  for(uint32_t bytes:ResidentBufferBytes(p)){
   if(!bytes||bytes>p.coldTransientBytes)return false;
   total+=bytes;
  }
  if(total!=p.payloadBytes)return false;
 }
 return true;
}
static_assert(ValidResidentBuffers(), "T445 simultaneous resident payload mismatch");
}
