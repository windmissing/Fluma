#pragma once
#include "research_config.h"
#include "dm_seed_generated.h"
#include "touch_adapter.h"
namespace grs::g9dm::candidate548 {
inline constexpr auto kConfig=kConfig512;
inline bool Decode(const std::array<uint32_t,64>& words,Config& out){return DecodeConfig(words,out);}
inline bool FillSeed(uint32_t* state,size_t words,uint32_t k=512){return FillDmTemplate(state,words,k);}
}
