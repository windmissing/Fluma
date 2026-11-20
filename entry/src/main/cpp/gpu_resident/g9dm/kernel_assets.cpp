#include "transport.h"
#include "candidate_v2.h"
#ifdef GRS_G9_BOOT_ONLY
#include "boot_source.h"
#else
#include "g9dm_kernel.h"
#endif
namespace grs::g9dm {
#ifdef GRS_G9_BOOT_ONLY
const char* PipelineSource(){return kBootPipeline;}
const char* PipelineSourceHash(){return kBootPipelineSHA;}
const char* BootRoleSetSHA(){return kBootRoleSetSHA;}
#else
const char* PipelineSource(){return kG9DmPipeline;}
const char* PipelineSourceHash(){return kG9DmPipelineHash;}
#endif
}
