#include "kernel_assets.h"
#include "m3a_source_kernel.h"
#include "m3a_quantum_kernel.h"
#include "m3a_root_sha.h"
namespace grs::m3a {
const char* SourceKernelV4(){return kM3aSource;}
const char* SourceKernelShaV4(){return kM3aSourceSha;}
const char* QuantumKernelV4(){return kM3aQuantum;}
const char* QuantumKernelShaV4(){return kM3aQuantumSha;}
const char* RootKernelShaV4(){return kM3aRootSha;}
}
