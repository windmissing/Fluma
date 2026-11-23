// Cold observation of the already qualified diagnostic arms. This path
// exposes raw write sets. It cannot issue a production root or publish.
void Boot::ObserveT849Writes() {
  constexpr std::array<const char *,4> roles{"OP7_FIELD_ONLY","OP7_MOTION_ONLY","OP7_MMS_FORCE_ONLY","OP8"};
  struct Pipelines {
    VulkanApi &vk;
    VkDevice device;
    std::array<VkPipeline,4> handles{};
    ~Pipelines() { for (auto h : handles) if (h) vk.DestroyPipeline(device,h,nullptr); }
  } qualified{vk,device};
  for (size_t i=0;i<roles.size();++i) {
    QualifySteady(roles[i]);
    qualified.handles[i]=pipeline;
    pipeline=VK_NULL_HANDLE;
  }
  Checkpoint("T849_PRIVATE_WRITE_SET");
  constexpr std::array<size_t,4> slots{0,1,kWork,kControl};
  std::array<uint32_t,4> sizes{};
  uint64_t sampleBytes=0;
  for (size_t i=0;i<slots.size();++i) {
    Require(buffers[slots[i]].bytes<=4*1024*1024,"T849_PRIVATE_RANGE");
    sizes[i]=uint32_t(buffers[slots[i]].bytes);
    sampleBytes+=sizes[i];
  }
  constexpr uint32_t samples=5;
  const auto bytes=sampleBytes*samples;
  Require(bytes<=32*1024*1024,"T849_PRIVATE_STAGING_LIMIT");
  Allocate(kM1ColdStaging,bytes,VK_BUFFER_USAGE_TRANSFER_DST_BIT,true);
  const auto &staging=buffers[kM1ColdStaging];
  auto reset=reinterpret_cast<PFN_vkResetCommandPool>(vk.GetDeviceProcAddr(device,"vkResetCommandPool"));
  Require(reset!=nullptr,"T849_PRIVATE_RESET");
  Check(reset(device,commandPool,0),"vkResetCommandPool.T849_PRIVATE");
  vk.DestroyFence(device,fence,nullptr);
  fence=VK_NULL_HANDLE;
  VkFenceCreateInfo fenceInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
  Check(vk.CreateFence(device,&fenceInfo,nullptr,&fence),"vkCreateFence.T849_PRIVATE");
  VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  begin.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  Check(vk.BeginCommandBuffer(command,&begin),"vkBeginCommandBuffer.T849_PRIVATE");
  // The same GPU-initialized 64x64 BOOT buffers stay bound throughout.
  // Use fixed rotation with a live MMS force window. No fixture word is an
  // accepted root or a native permit.
  const std::array<uint32_t,5> motion{0x3fc00000,0x3f800000,0x40000000,0,3};
  const std::array<uint32_t,8> force{0,0,2,0,0,0,3,0x3e800000};
  constexpr uint32_t motionMetadata=3;
  const uint32_t scopeOp=0;
  const std::array<uint32_t,2> mmsSentinel{0x7fc00042,0x7fc00043};
  Barrier(kCompute,VK_ACCESS_SHADER_WRITE_BIT,kTransfer,VK_ACCESS_TRANSFER_WRITE_BIT);
  vk.CmdUpdateBuffer(command,buffers[1].handle,26*4,sizeof(motion),motion.data());
  vk.CmdUpdateBuffer(command,buffers[1].handle,198*4,sizeof(force),force.data());
  vk.CmdUpdateBuffer(command,buffers[1].handle,206*4,sizeof(motionMetadata),&motionMetadata);
  vk.CmdUpdateBuffer(command,buffers[kControl].handle,520*4,sizeof(scopeOp),&scopeOp);
  vk.CmdUpdateBuffer(command,buffers[kControl].handle,642*4,sizeof(mmsSentinel),mmsSentinel.data());
  Barrier(kCompute|kTransfer,VK_ACCESS_SHADER_WRITE_BIT|VK_ACCESS_TRANSFER_WRITE_BIT,kTransfer,VK_ACCESS_TRANSFER_READ_BIT);
  auto capture=[&](size_t sample) {
    VkDeviceSize at=sampleBytes*sample;
    for (size_t i=0;i<slots.size();++i) {
      const VkBufferCopy copy{0,at,sizes[i]};
      vk.CmdCopyBuffer(command,buffers[slots[i]].handle,staging.handle,1,&copy);
      at+=sizes[i];
    }
  };
  capture(0);
  constexpr std::array<uint32_t,6> op7{7,0,0,0,0,815};
  constexpr std::array<uint32_t,6> op8{8,0,0,0,0,816};
  vk.CmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_COMPUTE,pipelineLayout,0,1,&sets[1],0,nullptr);
  for (size_t i=0;i<roles.size();++i) {
    Barrier(kTransfer|kCompute,VK_ACCESS_TRANSFER_READ_BIT|VK_ACCESS_TRANSFER_WRITE_BIT|VK_ACCESS_SHADER_WRITE_BIT,
            kCompute,kShaderRW);
    const auto &push=i<3 ? op7 : op8;
    vk.CmdBindPipeline(command,VK_PIPELINE_BIND_POINT_COMPUTE,qualified.handles[i]);
    vk.CmdPushConstants(command,pipelineLayout,VK_SHADER_STAGE_COMPUTE_BIT,0,sizeof(push),push.data());
    vk.CmdDispatch(command,1,1,1);
    Barrier(kCompute,VK_ACCESS_SHADER_WRITE_BIT,kTransfer,VK_ACCESS_TRANSFER_READ_BIT);
    capture(i+1);
  }
  Barrier(kTransfer,VK_ACCESS_TRANSFER_WRITE_BIT,VK_PIPELINE_STAGE_HOST_BIT,VK_ACCESS_HOST_READ_BIT);
  Check(vk.EndCommandBuffer(command),"vkEndCommandBuffer.T849_PRIVATE");
  VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
  submit.commandBufferCount=1;
  submit.pCommandBuffers=&command;
  const auto queued=vk.QueueSubmit(queue,1,&submit,fence);
  Check(queued,"vkQueueSubmit.T849_PRIVATE");
  inFlight=true;
  const auto waited=vk.WaitForFences(device,1,&fence,VK_TRUE,250000000u);
  if (waited==VK_SUCCESS || waited==VK_ERROR_DEVICE_LOST) inFlight=false;
  // Pipeline lifetimes belong to the enclosing private attempt, even on a late fence.
  if (inFlight) {
    const auto retire=vk.DeviceWaitIdle(device);
    if (retire==VK_SUCCESS || retire==VK_ERROR_DEVICE_LOST) inFlight=false;
    Check(retire,"vkDeviceWaitIdle.T849_PRIVATE");
  }
  Check(waited,"vkWaitForFences.T849_PRIVATE");
  std::vector<uint8_t> raw(size_t(bytes),0);
  void *mapped=nullptr;
  Check(vk.MapMemory(device,staging.memory,0,VK_WHOLE_SIZE,0,&mapped),"vkMapMemory.T849_PRIVATE");
  VkResult invalidation=VK_SUCCESS;
  if (!(staging.flags&VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
    VkMappedMemoryRange range{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,nullptr,staging.memory,0,VK_WHOLE_SIZE};
    invalidation=vk.InvalidateMappedMemoryRanges(device,1,&range);
  }
  if (invalidation==VK_SUCCESS) std::memcpy(raw.data(),mapped,raw.size());
  vk.UnmapMemory(device,staging.memory);
  Check(invalidation,"vkInvalidateMappedMemoryRanges.T849_PRIVATE");
  Require(attempt.current(),"ATTEMPT_REVOKED");
  const auto &path=attempt.t849WriteFile;
  const HANDLE output=CreateFileW(path.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
  Require(output!=INVALID_HANDLE_VALUE,"T849_PRIVATE_CREATE");
  DWORD done=0;
  const bool wrote=WriteFile(output,raw.data(),DWORD(raw.size()),&done,nullptr) &&
                   done==raw.size() && FlushFileBuffers(output);
  CloseHandle(output);
  if (!wrote) DeleteFileW(path.c_str());
  Require(wrote,"T849_PRIVATE_WRITE");
  fields << ",\"t849PrivateWrites\":{\"format\":\"G13_T849_PRIVATE_V1\",\"fileBytes\":" << raw.size()
         << ",\"sha256\":" << Quote(Hex(Hash(raw.data(),raw.size()).data())) << ",\"segmentsBytes\":" << Words(sizes)
         << ",\"segments\":[\"BASE\",\"CANDIDATE\",\"WORK\",\"CTRL\"]"
         << ",\"stages\":[\"BEFORE\",\"FIELD\",\"MOTION\",\"MMS\",\"OP8\"]"
         << ",\"case\":\"FIXED_ROTATION_MMS_INSIDE_MATCHED_MODE\",\"queueSubmit\":" << int(queued)
         << ",\"fence\":" << int(waited) << ",\"acceptedRoot\":null,\"productionEquivalent\":false}";
}
