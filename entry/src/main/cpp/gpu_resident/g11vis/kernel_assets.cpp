// T566 (problem T563): composite pipeline re-exports + kernels.cmake glue.
#include "visual_transport.h"
#include "g11vis_pipeline.h"
namespace grs::g11vis {
const char* VisualTransport::PipelineSourceR1(){return kG11R1Pipeline;}
const char* VisualTransport::PipelineSourceHashR1(){return kG11R1PipelineHash;}
const char* VisualTransport::PipelineSourceDm(){return kG11DmPipeline;}
const char* VisualTransport::PipelineSourceHashDm(){return kG11DmPipelineHash;}
}
