#include "pass.h"
#include <algorithm>
#include <iterator>

namespace grs::pc::sand {
void PutCommonWord(Common &out, size_t at, uint32_t value) {
  if (at % 4 || at > out.size() - 4) return;
  for (size_t i=0;i<4;++i) out[at+i]=uint8_t(value>>(8*i));
}

std::optional<Command> Plan(Root root,size_t ordinal) {
  const bool source=root==Root::Source;
  const auto *rows=source ? kSource : kQuantum;
  const auto count=source ? std::size(kSource) : std::size(kQuantum);
  if (ordinal>=count) return {};
  const auto &p=rows[ordinal];
  const auto bindingOpcode=p.bindingOpcode==UINT32_MAX ? p.opcode : p.bindingOpcode;
  if (!p.hasModule) return {};
  constexpr uint32_t current=0,candidate=2; // Both parent owners rotate handles in the slots.
  uint32_t read=1,write=candidate,event=6;
#ifdef PC_SAND_G11_R1
  constexpr uint32_t freeze=1, validate=31, final=20, scopeSeal=21, compare=26, audit=38, sourceCompare=39;
  constexpr uint32_t sourceSeal=41, rootSeal=29, selection=22;
#else
  constexpr uint32_t freeze=13, validate=1, final=34, scopeSeal=35, compare=40, audit=8, sourceCompare=9;
  constexpr uint32_t sourceSeal=11, rootSeal=43, selection=36;
#endif
  if (!p.visual && (bindingOpcode==freeze || bindingOpcode==validate)) {
    read=current;
    write=source ? candidate : 1;
  }
  if (p.role==3 || p.role==4) write=4;
  if (!p.visual && bindingOpcode==final && (p.role==2 || p.role==4)) {
    read=7;
    write=6;
    event=1;
  }
  if (!p.visual && bindingOpcode==scopeSeal && p.role==4) { read=candidate; write=4; }
  if (!p.visual && bindingOpcode==compare) { read=3; write=candidate; }
  if (!p.visual && bindingOpcode==audit) { read=current; write=4; }
  if (!p.visual && bindingOpcode==sourceCompare) { read=candidate; write=4; }
  if (!p.visual && (bindingOpcode==sourceSeal || bindingOpcode==rootSeal)) { read=1; write=candidate; }
  if (p.visual && source && p.role==4) read=current;
  if (read==write || read==event || write==event) return {};
  Command result{{uint8_t(read),uint8_t(write),5,uint8_t(event)},
                 {p.opcode,p.role,p.j,uint32_t(p.micro),uint32_t(ordinal)},
                 source && ordinal==0,!p.visual && bindingOpcode==selection};
#ifdef PC_SAND_G11_R1
  result.push[5]=p.phase;
  result.push[6]=result.push[7]=p.role==4 ? 2 : 1;
  result.push[8]=uint32_t(ordinal);
#endif
  return result;
}

bool Record(const DispatchApi &api,VkCommandBuffer cb,const Pass &pass,const Command &command,const BoundPass &bound) {
  if (!api.bind || !api.descriptors || !api.push || !api.dispatch || !api.barrier || !cb ||
      !bound.pipeline || !bound.layout || !bound.descriptors || !pass.hasModule ||
      pass.opcode!=bound.opcode || pass.opcode!=command.push[0] ||
      bound.storageSlots!=command.storageSlots) return false;
#ifdef PC_SAND_G11_R1
  constexpr uint32_t pushBytes=36;
#else
  constexpr uint32_t pushBytes=20;
#endif
  api.bind(cb,VK_PIPELINE_BIND_POINT_COMPUTE,bound.pipeline);
  api.descriptors(cb,VK_PIPELINE_BIND_POINT_COMPUTE,bound.layout,0,1,&bound.descriptors,0,nullptr);
  api.push(cb,bound.layout,VK_SHADER_STAGE_COMPUTE_BIT,0,pushBytes,command.push.data());
  api.dispatch(cb,1,1,1);
  VkMemoryBarrier visible{};
  visible.sType=VK_STRUCTURE_TYPE_MEMORY_BARRIER;
  visible.srcAccessMask=VK_ACCESS_SHADER_WRITE_BIT;
  visible.dstAccessMask=VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_TRANSFER_READ_BIT;
  api.barrier(cb,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
              VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT|VK_PIPELINE_STAGE_TRANSFER_BIT,0,1,&visible,0,nullptr,0,nullptr);
  return true;
}

static bool Valid(const Slice &slice) {
  return slice.buffer && slice.generation && slice.bytes && !(slice.offset%4 || slice.bytes%4) &&
         slice.bytes<=UINT64_MAX-slice.offset;
}

bool WriteDescriptors(PFN_vkUpdateDescriptorSets update,VkDevice device,VkDescriptorSet descriptors,
                      const Command &command,const StorageSlots &storage,const Slice &uniform,
                      const VkPhysicalDeviceLimits &limits,bool initialAudit) {
  if (!update || !device || !descriptors || !Valid(uniform) || uniform.bytes!=kCommonBytes ||
      kCommonBytes>limits.maxUniformBufferRange || !limits.minUniformBufferOffsetAlignment ||
      uniform.offset%limits.minUniformBufferOffsetAlignment || !limits.minStorageBufferOffsetAlignment)
    return false;
  std::array<Slice,5> slices{};
  for (size_t i=0;i<command.storageSlots.size();++i) {
    const auto slot=command.storageSlots[i];
    if (slot>=storage.size()) return false;
    slices[i]=storage[slot];
    if (!Valid(slices[i]) || slices[i].bytes>limits.maxStorageBufferRange ||
        slices[i].offset%limits.minStorageBufferOffsetAlignment) return false;
  }
  slices[4]=uniform;
  std::array<VkDescriptorBufferInfo,5> buffers{};
  std::array<VkWriteDescriptorSet,5> writes{};
  for (uint32_t i=0;i<writes.size();++i) {
    const auto &slice=slices[i];
    for (uint32_t j=0;j<i;++j)
      if (!(initialAudit && i==1 && j==0) && slice.buffer==slices[j].buffer &&
          slice.offset<slices[j].offset+slices[j].bytes && slices[j].offset<slice.offset+slice.bytes)
        return false;
    buffers[i]={slice.buffer,slice.offset,slice.bytes};
    writes[i].sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[i].dstSet=descriptors;
    writes[i].dstBinding=i;
    writes[i].descriptorCount=1;
    writes[i].descriptorType=i==4 ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[i].pBufferInfo=&buffers[i];
  }
  update(device,uint32_t(writes.size()),writes.data(),0,nullptr);
  return true;
}

bool RecordParentCopies(PFN_vkCmdPipelineBarrier barrier,PFN_vkCmdCopyBuffer copy,VkCommandBuffer cb,
                        const Command &command,const StorageSlots &storage,
                        VkDeviceSize stateBytes,VkDeviceSize arenaBytes) {
  if (!command.freezeSourceBase && !command.saveSelection) return true;
  if ((command.freezeSourceBase && command.saveSelection) || !barrier || !copy || !cb) return false;
  const auto fits=[&](size_t from,size_t to,VkDeviceSize bytes) {
    return Valid(storage[from]) && Valid(storage[to]) && bytes && bytes%4==0 &&
           bytes<=storage[from].bytes && bytes<=storage[to].bytes && storage[from].buffer!=storage[to].buffer;
  };
  if (command.freezeSourceBase ? !fits(0,1,stateBytes) : (!fits(2,3,stateBytes) || !fits(6,7,arenaBytes)))
    return false;
  VkMemoryBarrier sync{};
  sync.sType=VK_STRUCTURE_TYPE_MEMORY_BARRIER;
  sync.srcAccessMask=VK_ACCESS_SHADER_WRITE_BIT|VK_ACCESS_TRANSFER_WRITE_BIT;
  sync.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT|VK_ACCESS_TRANSFER_WRITE_BIT;
  barrier(cb,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT|VK_PIPELINE_STAGE_TRANSFER_BIT,
          VK_PIPELINE_STAGE_TRANSFER_BIT,0,1,&sync,0,nullptr,0,nullptr);
  const auto record=[&](size_t from,size_t to,VkDeviceSize bytes) {
    const VkBufferCopy region{storage[from].offset,storage[to].offset,bytes};
    copy(cb,storage[from].buffer,storage[to].buffer,1,&region);
  };
  if (command.freezeSourceBase) record(0,1,stateBytes);
  else {
    record(2,3,stateBytes);
    record(6,7,arenaBytes);
  }
  sync.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;
  sync.dstAccessMask=VK_ACCESS_SHADER_READ_BIT|VK_ACCESS_SHADER_WRITE_BIT;
  barrier(cb,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
          0,1,&sync,0,nullptr,0,nullptr);
  return true;
}

bool RecordReadback(PFN_vkCmdPipelineBarrier barrier,PFN_vkCmdCopyBuffer copy,VkCommandBuffer cb,
                    const Slice &staging,const CopySpan *spans,size_t count) {
  if (!barrier || !copy || !cb || !Valid(staging) || !spans || !count) return false;
  for (size_t i=0;i<count;++i) {
    const auto &span=spans[i];
    if (!Valid(span.source) || span.source.buffer==staging.buffer || span.destination%4 ||
        span.destination>staging.bytes || span.source.bytes>staging.bytes-span.destination) return false;
    for (size_t j=0;j<i;++j)
      if (span.destination<spans[j].destination+spans[j].source.bytes &&
          spans[j].destination<span.destination+span.source.bytes) return false;
  }
  VkMemoryBarrier visible{};
  visible.sType=VK_STRUCTURE_TYPE_MEMORY_BARRIER;
  visible.srcAccessMask=VK_ACCESS_SHADER_WRITE_BIT|VK_ACCESS_TRANSFER_WRITE_BIT;
  visible.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;
  barrier(cb,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT|VK_PIPELINE_STAGE_TRANSFER_BIT,
          VK_PIPELINE_STAGE_TRANSFER_BIT,0,1,&visible,0,nullptr,0,nullptr);
  for (size_t i=0;i<count;++i) {
    const auto &span=spans[i];
    const VkBufferCopy region{span.source.offset,staging.offset+span.destination,span.source.bytes};
    copy(cb,span.source.buffer,staging.buffer,1,&region);
  }
  return true;
}

bool MakeHostVisible(PFN_vkCmdPipelineBarrier barrier,VkCommandBuffer cb,const Slice &staging) {
  if (!barrier || !cb || !Valid(staging)) return false;
  VkBufferMemoryBarrier visible{};
  visible.sType=VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  visible.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;
  visible.dstAccessMask=VK_ACCESS_HOST_READ_BIT;
  visible.srcQueueFamilyIndex=visible.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
  visible.buffer=staging.buffer;
  visible.offset=staging.offset;
  visible.size=staging.bytes;
  barrier(cb,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,0,nullptr,1,&visible,0,nullptr);
  return true;
}

VkResult AwaitReadback(PFN_vkWaitForFences wait,PFN_vkInvalidateMappedMemoryRanges invalidate,VkDevice device,
                       VkFence fence,uint64_t timeoutNs,bool coherent,VkDeviceMemory memory,
                       VkDeviceSize allocationBytes,VkDeviceSize atom,VkDeviceSize begin,VkDeviceSize bytes,
                       VkResult *fenceResult,VkResult *invalidateResult) {
  if (!wait || !device || !fence || !memory || !bytes || begin>allocationBytes ||
      bytes>allocationBytes-begin || (!coherent && (!invalidate || !atom)))
    return VK_ERROR_INITIALIZATION_FAILED;
  const auto result=wait(device,1,&fence,VK_TRUE,timeoutNs);
  if (fenceResult) *fenceResult=result;
  if (result!=VK_SUCCESS || coherent) return result;
  VkMappedMemoryRange range{};
  range.sType=VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
  range.memory=memory;
  range.offset=begin-begin%atom;
  const auto end=begin+bytes;
  const auto padding=(atom-end%atom)%atom;
  range.size=end+std::min(padding,allocationBytes-end)-range.offset;
  const auto invalidated=invalidate(device,1,&range);
  if (invalidateResult) *invalidateResult=invalidated;
  return invalidated;
}
}
