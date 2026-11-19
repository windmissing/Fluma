#pragma once
#include "generated_contract.h"
namespace grs::g11vis {
struct SeedSegment{uint32_t source,packed,bytes,kind;};
inline std::array<SeedSegment,4> SeedSegments(const VisualResources& r){return {{{uint32_t(r.visual[kSegControl]+0u),0u,512u,1u},{uint32_t(r.visual[kSegMarkerCurrent]+0u),(0u+512u),128u*r.v,2u},{uint32_t(r.visual[kSegControl]+1536u),((0u+512u)+128u*r.v),8192u,3u},{uint32_t(r.visual[kSegControl]+60544u),(((0u+512u)+128u*r.v)+8192u),256u,4u},}};}
struct ColdResources{uint32_t parent,k,v;size_t rootBytes,fullBytes,twoFullBytes,witnessBytes,fullGpuBytes;};
inline constexpr std::array<ColdResources,8> kColdResources={{
{0u,192u,2048u,28192768u,98504704u,197009408u,3149824u,70311936u},
{0u,192u,8192u,30748672u,101060608u,202121216u,4722688u,70311936u},
{0u,512u,2048u,49491968u,183209984u,366419968u,3149824u,133718016u},
{0u,512u,8192u,52047872u,185765888u,371531776u,4722688u,133718016u},
{1u,192u,2048u,30289920u,106893312u,213786624u,3149824u,76603392u},
{1u,192u,8192u,32845824u,109449216u,218898432u,4722688u,76603392u},
{1u,512u,2048u,51589120u,191598592u,383197184u,3149824u,140009472u},
{1u,512u,8192u,54145024u,194154496u,388308992u,4722688u,140009472u},
}};
inline const ColdResources* SelectCold(uint32_t parent,uint32_t k,uint32_t v){for(const auto& r:kColdResources)if(r.parent==parent&&r.k==k&&r.v==v)return &r;return nullptr;}
}
