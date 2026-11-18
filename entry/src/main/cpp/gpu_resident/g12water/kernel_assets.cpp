// T576 (problem T567): G12 kernel re-exports.
#include "transport.h"
#include "g12water_pipeline.h"
namespace grs::g12water {
const char* Transport::PipelineSource(){return kG12WaterPipeline;}
const char* Transport::PipelineSourceHash(){return kG12WaterPipelineHash;}
}
