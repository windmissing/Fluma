#include "kernel_assets.h"
#include "m3b_source_kernel.h"
#include "m3b_quantum_kernel.h"
#include "m3b_root_sha.h"
namespace grs::m3b {
const char* SourceKernelV5(){return kM3bSource;}
const char* SourceKernelShaV5(){return kM3bSourceSha;}
const char* QuantumKernelV5(){return kM3bQuantum;}
const char* QuantumKernelShaV5(){return kM3bQuantumSha;}
const char* RootKernelShaV5(){return kM3bRootSha;}
}
