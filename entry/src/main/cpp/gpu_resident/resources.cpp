#include "resources.h"
#include <algorithm>
namespace grs {
bool PlanStateSegments(uint32_t K,size_t alignment,size_t maxBytes,StateSegments& out){
 if((K!=192&&K!=512)||alignment==0)return false;StateSegments p;size_t next=64*size_t(K);
 if(!AlignUp(next,alignment,maxBytes,p.water))return false;next=p.water+48*64;
 if(next<p.water||!AlignUp(next,alignment,maxBytes,p.column))return false;next=p.column+64*128;
 if(next<p.column||!AlignUp(next,alignment,maxBytes,p.header)||p.header>maxBytes||maxBytes-p.header<1024)return false;p.total=p.header+1024;out=p;return true;
}
ResourceCode PlanResources(uint32_t K,const ProfileLimits& limits,ResourcePlan& out){
 if(K!=192&&K!=512)return ResourceCode::INVALID_PROFILE;
 ResourcePlan p;p.K=K;p.eventCapacity=133*K+508;p.sortLayers=K==192?36:45;p.sourceDispatch=K==192?32:34;p.quantumDispatch=23+p.sortLayers;p.tickDispatch=p.sourceDispatch+4*p.quantumDispatch;
 size_t n=0;auto row=[&](const char* name,size_t bytes,bool host=false){p.rows[n++]={name,bytes,bytes,host};};
 row("three_state_banks",3*(64*K+48*64+64*128+1024));row("two_publications",2*(64*K+64*64+32*128+1024));row("motion32",128*K);row("two_index_arrays",8*(K==192?256:512));row("scan_scratch",4*(K+64+128));row("edge_proposals",64*128);row("column_audit",16*128);row("two_frame_event64_arenas",2*256*size_t(p.eventCapacity));row("dedup64",256*4096);row("source_event16",64*4096);row("queue32",128*16);row("two_status256",2048);row("two_gpu_completion256",2048);row("two_host_completion256",2048,true);row("source_receipts64",256*16,true);row("angle_table",245776);row("fixed_stats",1024);row("fixed_control",1024);
 uint32_t pad=K==192?256:512;p.doublePaddedSortFits=true;p.paddedSortExtraBytes=0;
 for(size_t i=0;i<n;i++){p.payloadBytes+=p.rows[i].payloadBytes;size_t one=p.rows[i].payloadBytes;if(i==0)one/=3;else if(i==1||i==7||i==11||i==12||i==13)one/=2;p.maxBlockBytes=std::max(p.maxBlockBytes,one);}
 out=p;
 if(!limits.measured)return ResourceCode::CAPABILITY_REQUIRED;
 if(limits.globalBindings<4||limits.computeBlocks<4||limits.workGroupSize<64||limits.workGroupInvocations<64||limits.storageAlignment==0)return ResourceCode::CAPABILITY_UNSUPPORTED;
 if(limits.maxBlockBytes<p.maxBlockBytes||limits.maxTotalPayloadBytes<p.payloadBytes)return ResourceCode::MEMORY_PROFILE_UNSUPPORTED;
 p.allocatedBytes=0;
 for(size_t i=0;i<n;i++){size_t count=i==0?3:(i==1||i==7||i==11||i==12||i==13)?2:1,one=p.rows[i].payloadBytes/count,aligned=0;
  if(!AlignUp(one,limits.storageAlignment,limits.maxBlockBytes,aligned)||aligned>SIZE_MAX/count)return ResourceCode::MEMORY_PROFILE_UNSUPPORTED;
  if(i==0){StateSegments state;if(!PlanStateSegments(K,limits.storageAlignment,limits.maxBlockBytes,state))return ResourceCode::MEMORY_PROFILE_UNSUPPORTED;aligned=state.total;}
  p.rows[i].allocatedBytes=aligned*count;if(p.rows[i].allocatedBytes>SIZE_MAX-p.allocatedBytes)return ResourceCode::MEMORY_PROFILE_UNSUPPORTED;p.allocatedBytes+=p.rows[i].allocatedBytes;
 }
 if(p.allocatedBytes>limits.maxTotalPayloadBytes)return ResourceCode::MEMORY_PROFILE_UNSUPPORTED;
 // measured here means caller supplied limits; only actual GL cold allocation can validate resources.
 p.deviceValidated=false;out=p;return ResourceCode::OK;
}
}
