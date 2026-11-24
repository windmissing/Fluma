#include "host_device.h"
#include "../wsi.h"
#include <iostream>

namespace grs::pc::sand {
bool PresentG15(HostDevice &gpu,const VulkanOwner::AcceptedProof &proof,const VulkanOwner &owner,
                const Slice &state,const Slice &work,const Slice &witness,const VulkanOwner::Readback &pixel,
                VkDeviceSize offset,bool last) {
  if (!proof.ProducedBy(&owner) || !state.buffer || !work.buffer || !witness.buffer || witness.bytes!=8192 ||
      offset>work.bytes || witness.bytes>work.bytes-offset || !gpu.Current()) return false;
  g13vk::VulkanApi vk{};
  vk.GetInstanceProcAddr=gpu.instanceProc;
#define LOAD_I(name) vk.name=reinterpret_cast<PFN_vk##name>(gpu.instanceProc(gpu.instance,"vk" #name));
  T742_INSTANCE_API(LOAD_I)
#undef LOAD_I
#define LOAD_D(name) vk.name=reinterpret_cast<PFN_vk##name>(gpu.deviceProc(gpu.device,"vk" #name));
  T742_DEVICE_API(LOAD_D)
#undef LOAD_D
  pc::PresentObservations observations{};
  observations.g15Work=work.buffer;
  observations.g15Witness=witness.buffer;
  observations.g15WitnessOffset=work.offset+offset;
  observations.timing=true;
  std::function<VkBuffer()> next;
  if (!last) next=[] { return VkBuffer{}; };
  const auto &permit=proof.Permission();
  const auto frame=pc::PresentBoot(vk,gpu.instance,gpu.physical,gpu.device,gpu.queue,gpu.family,gpu.surface,
      state.buffer,pixel.buffer.buffer,pixel.memory,gpu.properties.limits.nonCoherentAtomSize ?
      (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|(pixel.coherent?VK_MEMORY_PROPERTY_HOST_COHERENT_BIT:0)) : 0,
      gpu.window,[&]{return gpu.Current();},[&](const pc::Frame &ready) {
        std::cout << "{\"stage\":\"G15_SCENE_PRESENT\",\"nonce\":\"" << permit.nonce
                  << "\",\"run\":\"" << permit.identity.run << "\",\"geometry\":\"" << permit.geometry
                  << "\",\"stateGeneration\":\"" << state.generation << "\",\"arenaGeneration\":\"" << permit.arenaGeneration
                  << "\",\"imageIndex\":" << ready.imageIndex << ",\"graphicsFence\":\"" << ready.graphicsFence
                  << "\",\"fenceExitQpc\":\"" << ready.graphicsFenceExit << "\",\"presentExitQpc\":\""
                  << ready.presentExit << "\"}\n";
      },state.bytes,false,next,false,{},observations);
  std::cout << "{\"stage\":\"G15_SCENE_RETIRE\",\"nonce\":\"" << permit.nonce
            << "\",\"vkResult\":" << int(frame.error) << ",\"retireVkResult\":" << int(frame.retire) << "}\n";
  return frame.error==VK_SUCCESS && frame.retire==VK_SUCCESS;
}
}
