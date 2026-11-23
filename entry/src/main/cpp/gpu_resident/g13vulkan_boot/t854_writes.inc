void Boot::ObserveT854Writes(const Profile &profile) {
  constexpr std::array<const char *,3> roles{"OP17_PRIVATE_MOTION","OP17_PRIVATE_MMS","OP17_PRIVATE_LEDGER"};
  for(size_t i=0;i<roles.size();++i){
    QualifySteady(roles[i]);
    t854Pipelines[i]=std::exchange(pipeline,VK_NULL_HANDLE);
  }
  size_t ordinal=0;
  for(;ordinal<std::size(k_quantum)&&k_quantum[ordinal].op!=17;++ordinal){}
  pc::RootPlanPass resolved{};
  Require(ordinal<std::size(k_quantum)&&pc::PlanRootPass(false,ordinal,k_quantum[ordinal],resolved)&&
          resolved.baseSlot==0&&resolved.targetSlot==1,"T854_OP17_PLAN");
  const auto &push=resolved.push;
  Checkpoint("T854_PRIVATE_OP17");

  constexpr std::array<size_t,4> slots{0,1,kWork,kControl};
  std::array<uint32_t,4> sizes{};
  uint64_t snapshotBytes=0;
  for(size_t i=0;i<slots.size();++i){
    Require(buffers[slots[i]].bytes<=4*1024*1024,"T854_PRIVATE_RANGE");
    sizes[i]=uint32_t(buffers[slots[i]].bytes);
    snapshotBytes+=sizes[i];
  }
  const uint32_t privateWord=profile.ww;
  Require(privateWord%4==0 && sizes[2]==uint64_t(privateWord)*4+128 &&
          profile.work[0].offset+1<privateWord,"T854_WORK_SEGMENT");
  constexpr uint32_t caseCount=3, samplesPerCase=4;
  const auto bytes=snapshotBytes*(1+caseCount*samplesPerCase);
  Require(bytes<=32*1024*1024,"T854_STAGING_LIMIT");
  Allocate(kM1ColdStaging,bytes,VK_BUFFER_USAGE_TRANSFER_DST_BIT|VK_BUFFER_USAGE_TRANSFER_SRC_BIT,true);
  const auto &staging=buffers[kM1ColdStaging];
  const auto reset=reinterpret_cast<PFN_vkResetCommandPool>(vk.GetDeviceProcAddr(device,"vkResetCommandPool"));
  Require(reset!=nullptr,"T854_RESET_FUNCTION");
  Check(reset(device,commandPool,0),"vkResetCommandPool.T854");
  vk.DestroyFence(device,fence,nullptr);
  fence=VK_NULL_HANDLE;
  VkFenceCreateInfo fenceInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
  Check(vk.CreateFence(device,&fenceInfo,nullptr,&fence),"vkCreateFence.T854");
  VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  begin.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  Check(vk.BeginCommandBuffer(command,&begin),"vkBeginCommandBuffer.T854");
  auto write=[&](size_t slot,uint32_t word,const auto &values){
    vk.CmdUpdateBuffer(command,buffers[slot].handle,word*4,sizeof(values),&values);
  };
  auto capture=[&](uint32_t sample){
    VkDeviceSize at=snapshotBytes*sample;
    for(size_t i=0;i<slots.size();++i){
      const VkBufferCopy copy{0,at,sizes[i]};
      vk.CmdCopyBuffer(command,buffers[slots[i]].handle,staging.handle,1,&copy);
      at+=sizes[i];
    }
  };
  Barrier(kCompute|kTransfer,VK_ACCESS_SHADER_WRITE_BIT|VK_ACCESS_TRANSFER_WRITE_BIT,
          kTransfer,VK_ACCESS_TRANSFER_READ_BIT|VK_ACCESS_TRANSFER_WRITE_BIT);
  const VkBufferCopy candidateSeed{0,0,sizes[1]};
  vk.CmdCopyBuffer(command,buffers[0].handle,buffers[1].handle,1,&candidateSeed);
  const std::array<uint32_t,32> clear{};
  write(kWork,privateWord,clear);
  write(kLayout,kLayoutBytes/4,privateWord);
  Barrier(kTransfer,VK_ACCESS_TRANSFER_WRITE_BIT,kTransfer,VK_ACCESS_TRANSFER_READ_BIT);
  capture(0);
  vk.CmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_COMPUTE,pipelineLayout,0,1,&sets[1],0,nullptr);
  const auto firstNonce=pc::QpcTicks();
  Require(firstNonce && firstNonce<UINT64_MAX-caseCount,"T854_NONCE");
  for(uint32_t c=0;c<caseCount;++c){
    if(c){
      Barrier(kCompute|kTransfer,VK_ACCESS_SHADER_WRITE_BIT|VK_ACCESS_TRANSFER_READ_BIT|VK_ACCESS_TRANSFER_WRITE_BIT,
              kTransfer,VK_ACCESS_TRANSFER_READ_BIT|VK_ACCESS_TRANSFER_WRITE_BIT);
      VkDeviceSize at=sizes[0];
      for(size_t i=1;i<slots.size();++i){
        const VkBufferCopy copy{at,0,sizes[i]};
        vk.CmdCopyBuffer(command,staging.handle,buffers[slots[i]].handle,1,&copy);
        at+=sizes[i];
      }
      Barrier(kTransfer,VK_ACCESS_TRANSFER_WRITE_BIT,kTransfer,VK_ACCESS_TRANSFER_WRITE_BIT);
    }
    const auto nonce=firstNonce+c;
    write(kControl,136,std::array<uint32_t,2>{uint32_t(nonce),uint32_t(nonce>>32)});
    write(kControl,520,uint32_t(0));
    write(kControl,518+push[1],c==1 ? push[2] : 0u);
    write(kControl,642,std::array<uint32_t,2>{0x7fc00042,0x7fc00043});
    write(kWork,profile.work[0].offset+1,uint32_t(0x3e800000));
    // Keep epoch <= tick even for the huge elapsed-time MMS domain fixture.
    if(c==2)write(0,18,std::array<uint32_t,2>{UINT32_MAX-1,UINT32_MAX});
    write(1,24,uint32_t(0));
    write(1,26,std::array<uint32_t,5>{0x3fc00000,0x3f800000,0x40000000,0,c==2 ? 0u : 3u});
    write(1,198,std::array<uint32_t,8>{0,0,c==2 ? UINT32_MAX : 2u,c==2 ? UINT32_MAX : 0u,0,0,3,0x3e800000});
    write(1,206,c==2 ? 0u : 3u);
    Barrier(kTransfer,VK_ACCESS_TRANSFER_WRITE_BIT,kTransfer,VK_ACCESS_TRANSFER_READ_BIT);
    const uint32_t start=1+c*samplesPerCase;
    capture(start);
    vk.CmdPushConstants(command,pipelineLayout,VK_SHADER_STAGE_COMPUTE_BIT,0,sizeof(push),push.data());
    for(size_t i=0;i<roles.size();++i){
      Barrier(kTransfer|kCompute,VK_ACCESS_TRANSFER_READ_BIT|VK_ACCESS_TRANSFER_WRITE_BIT|VK_ACCESS_SHADER_WRITE_BIT,
              kCompute,kShaderRW|VK_ACCESS_UNIFORM_READ_BIT);
      vk.CmdBindPipeline(command,VK_PIPELINE_BIND_POINT_COMPUTE,t854Pipelines[i]);
      vk.CmdDispatch(command,1,1,1);
      Barrier(kCompute,VK_ACCESS_SHADER_WRITE_BIT,kTransfer,VK_ACCESS_TRANSFER_READ_BIT);
      capture(start+uint32_t(i)+1);
    }
  }
  Barrier(kTransfer,VK_ACCESS_TRANSFER_WRITE_BIT,VK_PIPELINE_STAGE_HOST_BIT,VK_ACCESS_HOST_READ_BIT);
  Check(vk.EndCommandBuffer(command),"vkEndCommandBuffer.T854");
  Require(attempt.current(),"ATTEMPT_REVOKED");
  VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
  submit.commandBufferCount=1;
  submit.pCommandBuffers=&command;
  const auto queued=vk.QueueSubmit(queue,1,&submit,fence);
  Check(queued,"vkQueueSubmit.T854");
  inFlight=true;
  const auto waited=vk.WaitForFences(device,1,&fence,VK_TRUE,250000000u);
  if(waited==VK_SUCCESS||waited==VK_ERROR_DEVICE_LOST)inFlight=false;
  if(inFlight){
    const auto retire=vk.DeviceWaitIdle(device);
    if(retire==VK_SUCCESS||retire==VK_ERROR_DEVICE_LOST)inFlight=false;
    Check(retire,"vkDeviceWaitIdle.T854");
  }
  Check(waited,"vkWaitForFences.T854");
  std::vector<uint8_t> raw(size_t(bytes),0);
  void *mapped=nullptr;
  Check(vk.MapMemory(device,staging.memory,0,VK_WHOLE_SIZE,0,&mapped),"vkMapMemory.T854");
  VkResult invalidation=VK_SUCCESS;
  if(!(staging.flags&VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)){
    VkMappedMemoryRange range{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,nullptr,staging.memory,0,VK_WHOLE_SIZE};
    invalidation=vk.InvalidateMappedMemoryRanges(device,1,&range);
  }
  if(invalidation==VK_SUCCESS)std::memcpy(raw.data(),mapped,raw.size());
  vk.UnmapMemory(device,staging.memory);
  Check(invalidation,"vkInvalidateMappedMemoryRanges.T854");
  for(uint32_t c=0;c<caseCount;++c){
    const auto nonce=firstNonce+c;
    for(uint32_t phase=1;phase<=3;++phase){
      const auto at=(1+c*samplesPerCase+phase)*snapshotBytes+sizes[0]+sizes[1]+uint64_t(privateWord)*4;
      std::array<uint32_t,32> seen{};
      std::memcpy(seen.data(),raw.data()+at,sizeof(seen));
      Require(seen[0]==0x54383534&&seen[1]==phase&&seen[2]==uint32_t(nonce)&&seen[3]==uint32_t(nonce>>32)&&
              std::equal(push.begin(),push.end(),seen.begin()+4)&&seen[10]==uint32_t(c!=1)&&
              seen[11]==0&&seen[30]==0&&seen[31]==0,"T854_PRIVATE_LATCH");
    }
  }
  Require(attempt.current(),"ATTEMPT_REVOKED");
  const auto &path=attempt.t849WriteFile;
  const HANDLE output=CreateFileW(path.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
  Require(output!=INVALID_HANDLE_VALUE,"T854_PRIVATE_CREATE");
  DWORD done=0;
  const bool wrote=WriteFile(output,raw.data(),DWORD(raw.size()),&done,nullptr)&&
                   done==raw.size()&&FlushFileBuffers(output);
  CloseHandle(output);
  if(!wrote)DeleteFileW(path.c_str());
  Require(wrote,"T854_PRIVATE_WRITE");
  fields << ",\"t854PrivateWrites\":{\"format\":\"G13_T854_PRIVATE_V1\",\"fileBytes\":" << raw.size()
         << ",\"sha256\":" << Quote(Hex(Hash(raw.data(),raw.size()).data())) << ",\"segmentsBytes\":" << Words(sizes)
         << ",\"segments\":[\"BASE\",\"CANDIDATE\",\"WORK\",\"CTRL\"]"
         << ",\"initialSnapshots\":1,\"stages\":[\"BEFORE\",\"MOTION\",\"MMS\",\"LEDGER\"]"
         << ",\"cases\":[\"ACTIVE\",\"ENTRY_NOOP\",\"MMS_FAULT\"]"
         << ",\"firstNonce\":\"" << firstNonce << "\",\"ownerTid\":" << GetCurrentThreadId()
         << ",\"push\":" << Words(push) << ",\"layoutWords\":" << Words(BootLayout(profile))
         << ",\"descriptorObjects\":[\"" << reinterpret_cast<uintptr_t>(buffers[0].handle) << "\",\""
         << reinterpret_cast<uintptr_t>(buffers[1].handle) << "\",\"" << reinterpret_cast<uintptr_t>(buffers[kWork].handle)
         << "\",\"" << reinterpret_cast<uintptr_t>(buffers[kControl].handle) << "\"]"
         << ",\"privateWord\":" << privateWord << ",\"queueSubmit\":" << int(queued)
         << ",\"fence\":" << int(waited) << ",\"acceptedRoot\":null,\"productionEquivalent\":false}";
}
