#pragma once
#include <array>
#include <cstdint>

namespace grs::g13vk {
enum class D1Kind { Pulse, Source, Reserve, End };
struct D1Action {
  D1Kind kind=D1Kind::Pulse;
  uint32_t id=0, pulse=0;
  uint64_t ns=0;
  double y=0;
};

// T865's fixed PC dry schedule. Time here is a schedule, never a permit clock.
inline constexpr auto M1D1Plan() {
  std::array<D1Action,52> actions{};
  constexpr unsigned groups[]{6,10,8,12,8};
  constexpr D1Kind special[]{D1Kind::Source,D1Kind::Reserve,D1Kind::Source,D1Kind::End};
  unsigned i=0,pulse=0;
  uint64_t time=1000000;
  for (unsigned group=0;group<5;++group) {
    for (unsigned j=0;j<groups[group];++j) {
      time+=16000000;
      actions[i]={D1Kind::Pulse,i+1,++pulse,time};
      ++i;
    }
    if (group<4) {
      time+=16000000;
      actions[i]={special[group],i+1,0,time,group==0 ? 64. : 128.};
      ++i;
      actions[i]={D1Kind::Pulse,i+1,++pulse,time};
      ++i;
    }
  }
  return actions;
}
static_assert(M1D1Plan().back().pulse==48 && M1D1Plan().back().id==52);
}
