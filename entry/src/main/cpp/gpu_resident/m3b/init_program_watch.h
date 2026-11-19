#pragma once
#include "init_diagnostics.h"
#include "../program_cache/program_observer.h"
namespace grs::m3b::init650 {
class ProgramScope {
 ProgramWatch watch_;programcache::ProgramObservationHook previous_;
public:
 ProgramScope(Binding b,Code c,uint32_t role,const Digest& source):watch_{b,c,role,source},previous_(programcache::route5Hook){
  if(!b.store)return;
  programcache::route5Hook={&watch_,[](void* p,const char*,const char*,uint32_t,int64_t error,const char* sha){auto& w=*static_cast<ProgramWatch*>(p);auto digest=Hex(sha);if(digest!=Digest{})w.source=digest;w.binding.store->Emit(w.binding.ticket,w.code,"SAMPLE",-1,Life::NONE,&w.source,w.role,error>0?int32_t(error):-1);},[](void* p,uint32_t slot,const char* raw,size_t n,int64_t reported){auto& w=*static_cast<ProgramWatch*>(p);w.binding.store->LogBytes(w.binding.ticket,w.role,slot,raw,n,reported,w.source);}};
 }
 ~ProgramScope(){programcache::route5Hook=previous_;}
};
}
