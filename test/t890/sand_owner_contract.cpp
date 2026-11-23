#include "../../pc/vulkan/sand/owner.h"
#include <cassert>
#include <cstring>
#include <iostream>
#include <iterator>

int main() {
  using namespace grs::pc::sand;
  VulkanOwner owner;
  assert(!owner.Eligible() && !owner.HasPendingWork());
  assert(owner.Open({}, {})==VK_ERROR_INITIALIZATION_FAILED);
  assert(!owner.Prepare({},{}));
  assert(!owner.AcceptCurrent({},{}));
  assert(std::strlen(owner.RoleSetSHA())==64);
  assert(std::strlen(owner.RootModuleSHA(Root::Source))==64);
  assert(std::strlen(owner.RootModuleSHA(Root::Quantum))==64);
  for (size_t i=0;i<std::size(kSource);++i) assert(Plan(Root::Source,i));
  for (size_t i=0;i<std::size(kQuantum);++i) assert(Plan(Root::Quantum,i));

#ifdef PC_SAND_G11_R1
  const Slice work{reinterpret_cast<VkBuffer>(1),0,4825728,1};
  const auto actual=PlanR1Witness(Root::Quantum,{0,3,2},work,1);
  const auto replay=PlanR1Witness(Root::Quantum,{0,3,4},work,2);
  assert(actual && actual->beginOrdinal==699 && actual->applyOrdinal==702);
  assert(replay && replay->beginOrdinal==1965 && replay->applyOrdinal==1968);
  assert(!PlanR1Witness(Root::Quantum,{0,3,1},work,1));
#else
  const auto handle=reinterpret_cast<VkBuffer>(1);
  DmColdSources resources{};
  resources.base=resources.final={handle,0,591872,1};
  resources.selected=resources.authority={handle,0,12767232,1};
  resources.work={handle,0,1311360,1};
  assert(PlanDmCold(resources,false)->bytes==kDmColdRootBytes);
  assert(!PlanDmCold(resources,true));
  resources.discovery.fill(resources.selected);
  assert(PlanDmCold(resources,true)->bytes==kDmColdFullBytes);
#endif
  std::cout << "T890: owner fails closed before qualification, generated tables and capture ranges verified\n";
}
