#pragma once
#include <cstdint>
namespace grs::m3b {
// Scope transaction cursor; separate from T619 native rootCursor64.
// Reservations never authorize a reader.
// A failed compound operation keeps its pending range for forensic export.
struct CompoundCursor {
 uint32_t plannedPhysical=0,plannedAux=0,sealedPhysical=0,sealedAux=0;
 uint32_t pendingOrdinal=0,pendingPhysical=0,pendingAux=0,firstFailure=0;
 bool Reserve(uint32_t physical,uint32_t aux,uint32_t maxPhysical,uint32_t maxAux){
  if(firstFailure||pendingOrdinal||plannedPhysical!=sealedPhysical||plannedAux!=sealedAux||
     plannedPhysical>maxPhysical||plannedAux>maxAux||physical>maxPhysical-plannedPhysical||aux>maxAux-plannedAux)return false;
  if(!physical&&!aux)return false;
  if(plannedPhysical==UINT32_MAX)return false;
  pendingOrdinal=plannedPhysical+1;pendingPhysical=physical;pendingAux=aux;
  plannedPhysical+=physical;plannedAux+=aux;return true;
 }
 bool Seal(){
  if(firstFailure||!pendingOrdinal)return false;
  sealedPhysical=plannedPhysical;sealedAux=plannedAux;
  pendingOrdinal=pendingPhysical=pendingAux=0;return true;
 }
 void Fail(uint32_t reason){if(!firstFailure&&reason)firstFailure=reason;}
 bool Complete()const{return !firstFailure&&!pendingOrdinal&&plannedPhysical==sealedPhysical&&plannedAux==sealedAux;}
};
static_assert(sizeof(CompoundCursor)<=256);
}
