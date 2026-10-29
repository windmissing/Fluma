#include "foundation.h"
#include "layout_generated.h"
#include "angles_generated.h"
#include "grs_primitive_source.h"
#include "grs_source_kernel.h"
namespace grs {
FoundationInfo GetFoundationInfo(){return {"GRS_Q20_V1","GRS_FIXED120_TIME_V1","GRS_GPU_SCENE_Q20_V1",kLayoutManifestSha256,kAngleSha256,kGrsPrimitiveSha,0,true,false};}
const uint32_t* ColdAngleWords(size_t& count){count=61444;return kAngleWords;}
const char* SourceKernelSource(){return kGrsSourceKernel;}
const char* SourceKernelSha(){return kGrsSourceSha;}
const char* PrimitiveLibrarySource(){return kGrsPrimitiveSource;}
}
