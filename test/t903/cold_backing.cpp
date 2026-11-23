#include "../../entry/src/main/cpp/gpu_resident/g13vulkan_boot/m1_cold_format.h"
#include <cassert>

int main() {
  using namespace grs::g13vk;
  std::array<uint32_t,256> tail{};
  const M1ColdAnchor anchor{1,2,3,4,3,1,1};
  constexpr size_t eventBytes=8*256;
  assert(!M1ColdBackingMatches(tail,{},true,eventBytes));  // Empty preview.
  tail[4]=3; tail[6]=1; tail[27]=1; tail[32]=0; tail[33]=1;
  tail[35]=1; tail[36]=4; tail[38]=2; tail[51]=1; tail[52]=2; tail[53]=5; tail[54]=8;
  assert(M1ColdBackingMatches(tail,anchor,true,eventBytes));
  assert(!M1ColdBackingMatches(tail,anchor,false,eventBytes));
  tail[27]=2;
  assert(M1ColdBackingMatches(tail,anchor,false,eventBytes));
  for (const size_t word:{4,6,33,35,36,38,51,52,53,54}) {
    const auto original=tail[word];
    tail[word]=9;
    assert(!M1ColdBackingMatches(tail,anchor,true,eventBytes));
    tail[word]=original;
  }
  auto foreignFence=anchor;
  ++foreignFence.fenceNonce;
  assert(!M1ColdBackingMatches(tail,foreignFence,true,eventBytes));
}
