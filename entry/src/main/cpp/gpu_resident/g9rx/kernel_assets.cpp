#ifdef GRS_G9_BOOT_ONLY
#include "boot_source.h"
#else
#include "pipeline_source.h"
#endif
#include "transport.h"
namespace grs::g9rx {
#ifdef GRS_G9_BOOT_ONLY
const char *PipelineSource() { return kBootPipeline; }
const char *PipelineSourceHash() { return kBootPipelineSHA; }
const char *BootRoleSetSHA() { return kBootRoleSetSHA; }
#else
const char *PipelineSource() { return kRxPipeline; }
const char *PipelineSourceHash() { return kRxPipelineSHA; }
#endif
} // namespace grs::g9rx
