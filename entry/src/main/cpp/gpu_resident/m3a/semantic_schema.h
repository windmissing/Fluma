#pragma once
#include "protocol_generated.h"
namespace grs::m3a {
// These are representation fields, never inventory/budget/ID/angle exclusions.
constexpr bool RepresentationHeaderWord(size_t word){
    return word==header256::acceptedBank||word==header256::candidateBank||word==header256::spareBank||word==header256::branchEpochLo||word==header256::branchEpochHi;
}
constexpr uint32_t QuantumSemanticMask(size_t word){return word==52?~(7u<<22u):UINT32_MAX;}
// AUTHORITATIVE writes into the frame arena, while saved preview/consistency
// scopes start at zero. Preserve the ordinal by subtracting its frozen start;
// neither this header count nor event7 is excluded from comparison.
constexpr bool RootRelativeHeaderWord(size_t word){return word==header256::eventUsed;}
constexpr bool RootRelativeEventWord(size_t word){return word==7;}
}
