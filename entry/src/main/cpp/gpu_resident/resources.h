#pragma once
#include "integer.h"
#include <array>
namespace grs {
struct ProfileLimits {bool measured=false;uint32_t globalBindings=0,computeBlocks=0,workGroupSize=0,workGroupInvocations=0;size_t maxBlockBytes=0,storageAlignment=0,maxTotalPayloadBytes=0;};
enum class ResourceCode {OK,CAPABILITY_REQUIRED,CAPABILITY_UNSUPPORTED,MEMORY_PROFILE_UNSUPPORTED,INVALID_PROFILE};
struct ResourceRow {const char* name=nullptr;size_t payloadBytes=0,allocatedBytes=0;bool host=false;};
struct ResourcePlan {
 uint32_t K=0,N=128,Kw=64,Q=16,D=4096,eventCapacity=0,sortLayers=0,sourceDispatch=0,quantumDispatch=0,tickDispatch=0;
 std::array<ResourceRow,18> rows{};size_t payloadBytes=0,allocatedBytes=0,maxBlockBytes=0;bool deviceValidated=false;
 // This is an unresolved contract, not a hidden runtime expansion.
 bool doublePaddedSortFits=false;size_t paddedSortExtraBytes=0;
};
ResourceCode PlanResources(uint32_t K,const ProfileLimits&,ResourcePlan&);
struct StateSegments {size_t sand=0,water=0,column=0,header=0,total=0;};
bool PlanStateSegments(uint32_t K,size_t alignment,size_t maxBytes,StateSegments&);
}
