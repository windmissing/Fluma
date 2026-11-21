#ifdef GRS_G9_BOOT_SOURCE
#include "source_shader.h"
#elif defined(GRS_G9_BOOT_ONLY)
#include "boot_source.h"
#else
#include "pipeline_source.h"
#endif
#include "transport.h"
namespace grs::g9rx {
#ifdef GRS_G9_BOOT_SOURCE
const char *PipelineSource() { return kSourcePipeline; }
const char *PipelineSourceHash() { return kSourcePipelineSHA; }
const char *SourceRoleSetSHA() { return kSourceRoleSetSHA; }
#elif defined(GRS_G9_BOOT_ONLY)
const char *PipelineSource() { return kBootPipeline; }
const char *PipelineSourceHash() { return kBootPipelineSHA; }
const char *BootRoleSetSHA() { return kBootRoleSetSHA; }
#else
const char *PipelineSource() { return kRxPipeline; }
const char *PipelineSourceHash() { return kRxPipelineSHA; }
#endif
} // namespace grs::g9rx
