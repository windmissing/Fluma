#include "contract_generated.h"
#include <pipeline_source.h>
namespace grs::g13 {
bool PipelineTestHooks() { return kG13TestHooks; }
const char *PipelineSource() { return kG13Pipeline; }
const char *PipelineSha() { return kG13PipelineSha; }
} // namespace grs::g13
