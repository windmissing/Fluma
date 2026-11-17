// T564 (problem T560 §2): layered capability queries + cold record JSON.
// Driver queries happen only here (cold create / capability export), never
// per-frame; each glGet* result keeps its own status so an UNSUPPORTED_QUERY
// is reported instead of being treated as a hardware zero.
#include "stage_capability.h"
#include <cstdio>
#include <inttypes.h>
namespace grs::g9dm {
std::string StageCapabilityV2::Renderer()const{
 std::string out="{\"revision\":\"G9_STAGE_CAPABILITY_V2\"";
 out+=",\"context\":"+std::to_string(uintptr_t(context));
 out+=",\"route\":"+std::to_string(route)+",\"k\":"+std::to_string(k);
 out+=",\"build\":"+std::to_string(buildGeneration);
 out+=",\"computeEligible\":";out+=(computeEligible?"true":"false");
 if(computeFailure&&*computeFailure)out+=",\"computeFailure\":\""+std::string(computeFailure)+"\"";
 out+=",\"sceneEligible\":";out+=(sceneEligible?"true":"false");
 if(sceneFailure&&*sceneFailure)out+=",\"sceneFailure\":\""+std::string(sceneFailure)+"\"";
 char item[192];
 for(uint32_t i=0;i<queryCount;++i){const auto& q=queries[i];
  std::snprintf(item,sizeof(item),",\"q%" PRIu32 "\":[%" PRIu32 ",%" PRId64 ",%" PRId32 ",%" PRId32 ",\"%s\"]",i,q.pname,q.value,q.queryStatus,q.required,q.domain);
  out+=item;}
 out+="}";
 return out;
}
}
