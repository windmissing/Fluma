#pragma once
#include "layout_generated.h"
namespace grs { namespace m1 {
const char* SourceCoreKernel();const char* SourceCoreSha();
enum class ConfigureCode {Unsupported,InvalidRange,ProtocolReadyShaderNotImplemented};
struct ProfilePlan {const Profile* profile=nullptr;uint64_t stateStride=0,publicationStride=0,eventStride=0;};
// Full M1 shape/range planning remains unavailable for quantum submission.
// The distinct SourceCoreTransport supports only new empty SOURCE-only runs.
ConfigureCode Plan(uint32_t K,uint32_t Kw,uint32_t feature,uint32_t wireRevision,
                   uint64_t alignment,uint64_t maxBlock,ProfilePlan& out);
}}
