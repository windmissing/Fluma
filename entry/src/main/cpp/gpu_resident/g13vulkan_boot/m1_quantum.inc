VkPipeline Boot::M1Pipeline(const std::wstring &path, const char *sha) {
  std::ifstream input(std::filesystem::path(path), std::ios::binary | std::ios::ate);
  const auto size = input.tellg();
  Require(size > 0 && size <= 512*1024 && size % 4 == 0, "M1_QUANTUM_MODULE_SIZE");
  std::vector<uint32_t> words(size_t(size)/4);
  input.seekg(0);
  Require(bool(input.read(reinterpret_cast<char *>(words.data()), size)) &&
          Hex(Hash(words.data(), size_t(size)).data()) == sha, "M1_QUANTUM_MODULE_SHA");
  VkShaderModuleCreateInfo module{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
  module.codeSize = size_t(size);
  module.pCode = words.data();
  VkShaderModule handle = VK_NULL_HANDLE;
  Check(vk.CreateShaderModule(device, &module, nullptr, &handle), "vkCreateShaderModule.M1_QUANTUM");
  VkComputePipelineCreateInfo info{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
  info.layout = pipelineLayout;
  info.stage = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0,
                VK_SHADER_STAGE_COMPUTE_BIT, handle, "main", nullptr};
  VkPipeline built = VK_NULL_HANDLE;
  const auto result = vk.CreateComputePipelines(device, VK_NULL_HANDLE, 1, &info, nullptr, &built);
  vk.DestroyShaderModule(device, handle, nullptr);
  Check(result, "vkCreateComputePipelines.M1_QUANTUM");
  return built;
}

namespace m1pc {
constexpr size_t banks[]{kM1Base,kM1Candidate};
constexpr size_t pubs[]{kM1QPub,kM1QPubAlt};
constexpr uint32_t k = 192, kw = 128, pad = 256, water = 16*k, column = water+16*kw;
constexpr uint32_t header = column+2048, stateWords = header+256;
constexpr uint32_t sandIndex=32, waterIndex=sandIndex+pad, arrival=waterIndex+kw, edges=arrival+kw;
constexpr uint32_t frozen=edges+127*16, rows=frozen+512, angles=rows+k+kw;
constexpr uint32_t sourceEvents=angles+61444, spare=sourceEvents+4096*16, replay=spare+stateWords;
constexpr uint32_t capacity=4*(48*k+16*kw+254)+k;
constexpr uint32_t block=192;
constexpr auto storage=VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
constexpr auto transferDst=VK_BUFFER_USAGE_TRANSFER_DST_BIT;
}

void Boot::PrepareM1Quantum(const Candidate &candidate) {
  using namespace m1pc;
  auto &passes=m1QuantumPasses;
  m1::EachQuantumPass(stateWords, pad, kw, [&](m1::QuantumPassOp p) { passes.push_back(p); });
  const auto &limits = candidate.properties.limits;
  Require(capacity*256ull <= limits.maxStorageBufferRange && block <= limits.maxUniformBufferRange &&
          limits.minUniformBufferOffsetAlignment, "M1_QUANTUM_RESOURCE_LIMIT");
  const auto align = limits.minUniformBufferOffsetAlignment;
  const auto stride = (block+align-1)/align*align;
  const auto pubBytes = buffers[kM1Pub].bytes;
  const auto copyOut = attempt.m1ColdFile.empty() ? 0 : VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  Allocate(kM1QScratch, stateWords*4, storage);
  Allocate(kM1QWork, (replay+256)*4, storage | transferDst);
  Allocate(kM1QEvents, capacity*256ull, storage | transferDst | copyOut);
  Allocate(kM1QUniform, stride*passes.size(), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | transferDst);
  Allocate(kM1QPub, pubBytes, storage | copyOut);
  Allocate(kM1QPubAlt, pubBytes, storage | copyOut);

  const uint32_t count = uint32_t(passes.size());
  const VkDescriptorPoolSize sizes[] = {{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,count*8},
                                        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,count*2}};
  VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
  pool.maxSets = count*2;
  pool.poolSizeCount = 2;
  pool.pPoolSizes = sizes;
  Check(vk.CreateDescriptorPool(device, &pool, nullptr, &m1QuantumDescriptorPool), "vkCreateDescriptorPool.M1_QUANTUM");
  std::vector<VkDescriptorSetLayout> layouts(count*2, descriptorLayout);
  auto &sets=m1QuantumSets;
  sets.resize(count*2);
  VkDescriptorSetAllocateInfo setInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
  setInfo.descriptorPool = m1QuantumDescriptorPool;
  setInfo.descriptorSetCount = count*2;
  setInfo.pSetLayouts = layouts.data();
  Check(vk.AllocateDescriptorSets(device, &setInfo, sets.data()), "vkAllocateDescriptorSets.M1_QUANTUM");
  for (size_t base=0;base<2;++base) {
  const size_t outputs[] = {kM1QEvents,banks[1-base],pubs[1-base],kM1Tail};
  uint32_t channel=0;
  for (uint32_t i=0;i<count;++i) {
    if (passes[i].bind3) channel=passes[i].bind3;
    const size_t slots[] = {banks[base],kM1QScratch,kM1QWork,outputs[channel],kM1QUniform};
    std::array<VkDescriptorBufferInfo,5> ranges{};
    std::array<VkWriteDescriptorSet,5> writes{};
    for (uint32_t b=0;b<5;++b) {
      const auto &buffer = buffers[slots[b]];
      ranges[b] = {buffer.handle,b==4 ? stride*i : 0,b==4 ? block : buffer.bytes};
      writes[b] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,nullptr,sets[base*count+i],b,0,1,
                   b==4 ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                   nullptr,&ranges[b],nullptr};
    }
    vk.UpdateDescriptorSets(device,uint32_t(writes.size()),writes.data(),0,nullptr);
  }
  }
  Checkpoint("M1_QUANTUM_COLD_ANGLES");
  auto reset=reinterpret_cast<PFN_vkResetCommandPool>(vk.GetDeviceProcAddr(device,"vkResetCommandPool"));
  Require(reset!=nullptr,"M1_QUANTUM_RESET_FUNCTION");
  Check(reset(device,commandPool,0),"vkResetCommandPool.M1_ANGLES");
  vk.DestroyFence(device,fence,nullptr);
  fence=VK_NULL_HANDLE;
  VkFenceCreateInfo coldFence{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
  Check(vk.CreateFence(device,&coldFence,nullptr,&fence),"vkCreateFence.M1_ANGLES");
  VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  begin.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  Check(vk.BeginCommandBuffer(command,&begin),"vkBeginCommandBuffer.M1_ANGLES");
  for (size_t at=0;at<sizeof(kAngleWords);at+=65536)
    vk.CmdUpdateBuffer(command,buffers[kM1QWork].handle,angles*4+at,
                       std::min<size_t>(65536,sizeof(kAngleWords)-at),
                       reinterpret_cast<const uint8_t *>(kAngleWords)+at);
  Barrier(kTransfer,VK_ACCESS_TRANSFER_WRITE_BIT,kCompute,kShaderRW);
  Check(vk.EndCommandBuffer(command),"vkEndCommandBuffer.M1_ANGLES");
  VkSubmitInfo upload{VK_STRUCTURE_TYPE_SUBMIT_INFO};
  upload.commandBufferCount=1;
  upload.pCommandBuffers=&command;
  Check(vk.QueueSubmit(queue,1,&upload,fence),"vkQueueSubmit.M1_ANGLES");
  inFlight=true;
  const auto waited=vk.WaitForFences(device,1,&fence,VK_TRUE,UINT64_MAX);
  if (waited==VK_SUCCESS || waited==VK_ERROR_DEVICE_LOST) inFlight=false;
  if (attempt.hotCapture) attempt.hotCapture->StopBusy();
  Check(waited,"vkWaitForFences.M1_ANGLES");
  fields << ",\"m1ColdAngleUploadBytes\":" << sizeof(kAngleWords)
         << ",\"m1ColdAngleFenceVkResult\":" << int(waited);
  Checkpoint("M1_QUANTUM_COLD_ANGLES_COMPLETE");
}

bool Boot::RunM1Quantum(const Candidate &candidate, m1::SourceRootOwner &root,
                        m1::ProgressClockV2 &clock, m1::PlanGrantV2 &permit,
                        const std::function<bool()> &refresh, grs::AcceptedRoot &accepted,
                        uint64_t &frameSerial, uint32_t &ordinal) {
  using namespace m1pc;
  const m1::QuantumParameters p;
  const auto &passes=m1QuantumPasses;
  const auto &sets=m1QuantumSets;
  const uint32_t count=uint32_t(passes.size());
  const auto align=candidate.properties.limits.minUniformBufferOffsetAlignment;
  const auto stride=(block+align-1)/align*align;
  const auto baseBank=root.Bank();
  const auto previous=accepted.completion;
  auto reset = reinterpret_cast<PFN_vkResetCommandPool>(vk.GetDeviceProcAddr(device,"vkResetCommandPool"));
  Require(reset != nullptr,"M1_QUANTUM_RESET_FUNCTION");
  Check(reset(device,commandPool,0),"vkResetCommandPool.M1_QUANTUM");
  vk.DestroyFence(device,fence,nullptr);
  fence=VK_NULL_HANDLE;
  VkFenceCreateInfo fenceInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
  Check(vk.CreateFence(device,&fenceInfo,nullptr,&fence),"vkCreateFence.M1_QUANTUM");

  // A grant with no physics slot can authorize SOURCE alone.
  // Only same-frame roots can carry the SOURCE event prefix.
  const bool sameFrame = clock.activeRemaining != 0;
  while (!sameFrame) {
    if (!refresh()) {
      fields << ",\"m1QuantumReject\":\"CLOCK_OR_WINDOW_REVOKED_BEFORE_GRANT\"";
      return false;
    }
    if (clock.Claim(pc::TimeNs(),p.requestedAngle,0,false,permit)) {
      ++frameSerial;
      ordinal=0;
      break;
    }
    Sleep(1);
  }
  RootRequest r;
  r.kind=2;
  r.quantumOrdinal=++ordinal;
  const auto nonce=pc::QpcTicks();
  r.nonce={uint32_t(nonce),uint32_t(nonce>>32)};
  r.control={uint32_t(permit.id),uint32_t(permit.id>>32)};
  r.frame={uint32_t(frameSerial),uint32_t(frameSerial>>32)};
  r.eventArena=r.frame.lo&1u;
  r.publicationSlot=1-baseBank;
  const uint64_t previousFrame=uint64_t(previous[24])|(uint64_t(previous[25])<<32);
  r.eventStart=sameFrame && frameSerial==previousFrame ? previous[53] : 0;
  r.eventCapacity=capacity;
  r.dispatchCount=r.barrierCount=count;
  r.carry=uint32_t(permit.carry);
  // Match the original scalar-uniform accounting in the native Completion ABI.
  r.uploadedBytes=26*4+28+8*4+4;
  for (const auto &op : passes)
    r.uploadedBytes+=4+(op.color!=UINT32_MAX ? 4 : 0)+(op.sortK ? 8 : 0);
  Require(root.BeginQuantum(r,p.requestedAngle)==ControlCode::OK,"M1_QUANTUM_OWNER_BEGIN");
  const uint64_t deadline=permit.grantedNs+m1::ProgressClockV2::JobNs;
  VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  begin.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  Check(vk.BeginCommandBuffer(command,&begin),"vkBeginCommandBuffer.M1_QUANTUM");
  const auto work=buffers[kM1QWork].handle;
  Barrier(kCompute,VK_ACCESS_SHADER_WRITE_BIT,kTransfer,VK_ACCESS_TRANSFER_READ_BIT);
  const VkBufferCopy ledger{(96+4096*64+stateWords)*4,sourceEvents*4,4096*64};
  vk.CmdCopyBuffer(command,buffers[kM1Work].handle,work,1,&ledger);
  if (r.eventStart && previous[27]==1) {
    const VkBufferCopy prefix{0,0,r.eventStart*256ull};
    vk.CmdCopyBuffer(command,buffers[kM1Events].handle,buffers[kM1QEvents].handle,1,&prefix);
  }
  for (uint32_t i=0;i<count;++i) {
    const auto &op=passes[i];
    const std::array<uint32_t,48> u{
      k,kw,pad,water,column,header,stateWords,op.stage,op.color,op.sortK,op.sortJ,sandIndex,waterIndex,
      arrival,edges,frozen,rows,angles,sourceEvents,r.eventStart,capacity,p.gravity,p.settle,p.kAir,p.kFlight,
      p.kWet,p.kappa,p.soft,p.flux,r.quantumOrdinal,uint32_t(p.requestedAngle),0,r.nonce.lo,r.nonce.hi,
      r.frame.lo,r.frame.hi,r.control.lo,r.control.hi,spare,replay,baseBank,r.publicationSlot,r.eventArena,
      r.carry,r.uploadedBytes,count,0,0};
    vk.CmdUpdateBuffer(command,buffers[kM1QUniform].handle,stride*i,block,u.data());
  }
  Barrier(kTransfer,VK_ACCESS_TRANSFER_WRITE_BIT,kCompute,kShaderRW|VK_ACCESS_UNIFORM_READ_BIT);
  Barrier(kCompute,VK_ACCESS_SHADER_WRITE_BIT,kCompute,kShaderRW);
  const VkCommandBuffer cmd=command;
  VkPipeline bound=VK_NULL_HANDLE;
  for (uint32_t i=0;i<count;++i) {
    const auto next=m1QuantumPipelines[passes[i].stage<=20 ? 0 : 1];
    if (next!=bound) {
      vk.CmdBindPipeline(cmd,VK_PIPELINE_BIND_POINT_COMPUTE,next);
      bound=next;
    }
    vk.CmdBindDescriptorSets(cmd,VK_PIPELINE_BIND_POINT_COMPUTE,pipelineLayout,0,1,&sets[baseBank*count+i],0,nullptr);
    vk.CmdDispatch(cmd,(passes[i].n+63)/64,1,1);
    if (i+1<count) Barrier(kCompute,VK_ACCESS_SHADER_WRITE_BIT,kCompute,kShaderRW);
  }
  Barrier(kCompute,VK_ACCESS_SHADER_WRITE_BIT,kTransfer,VK_ACCESS_TRANSFER_READ_BIT);
  const VkBufferCopy tailCopy{0,0,1024};
  const auto &staging=buffers[kStaging];
  vk.CmdCopyBuffer(cmd,buffers[kM1Tail].handle,staging.handle,1,&tailCopy);
  Barrier(kTransfer,VK_ACCESS_TRANSFER_WRITE_BIT,VK_PIPELINE_STAGE_HOST_BIT,VK_ACCESS_HOST_READ_BIT);
  Check(vk.EndCommandBuffer(cmd),"vkEndCommandBuffer.M1_QUANTUM");
  stage="M1_QUANTUM_SUBMIT";
  fields << ",\"m1QuantumNonce\":\"" << nonce << "\",\"m1QuantumDispatches\":" << count
         << ",\"m1QuantumEventStart\":" << r.eventStart << ",\"m1QuantumDeadlineNs\":\"" << deadline << "\"";
  const auto current = [&] { return refresh() && clock.Identity(permit.id,permit.session); };
  if (!current() || pc::TimeNs()>=deadline) {
    fields << ",\"m1QuantumReject\":\"STALE_OR_LATE_BEFORE_SUBMIT\"";
    root.Stop();
    return false;
  }
  VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
  submit.commandBufferCount=1;
  submit.pCommandBuffers=&cmd;
  Check(vk.QueueSubmit(queue,1,&submit,fence),"vkQueueSubmit.M1_QUANTUM");
  inFlight=true;
  M1BusyFence busy{attempt.hotCapture,M1HotCapture::Phase::QuantumFence,nonce};
  VkResult waited=VK_NOT_READY;
  uint64_t now=pc::TimeNs();
  while (now<deadline && current()) {
    waited=vk.WaitForFences(device,1,&fence,VK_TRUE,std::min<uint64_t>(deadline-now,2000000));
    now=pc::TimeNs();
    if (waited!=VK_TIMEOUT) break;
  }
  if (waited==VK_SUCCESS || waited==VK_ERROR_DEVICE_LOST) inFlight=false;
  fields << ",\"m1QuantumFenceVkResult\":" << int(waited)
         << ",\"m1QuantumFenceNs\":\"" << now << "\"";
  if (waited!=VK_SUCCESS || now>deadline || !current()) {
    fields << ",\"m1QuantumReject\":\"FENCE_OR_PERMIT_FAILED\"";
    root.Stop();
    return false;
  }
  void *mapped=nullptr;
  Check(vk.MapMemory(device,staging.memory,0,VK_WHOLE_SIZE,0,&mapped),"vkMapMemory.M1_QUANTUM");
  VkResult invalidation=VK_SUCCESS;
  if (!(staging.flags&VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
    VkMappedMemoryRange range{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,nullptr,staging.memory,0,VK_WHOLE_SIZE};
    invalidation=vk.InvalidateMappedMemoryRanges(device,1,&range);
  }
  Words256 tail{};
  if (invalidation==VK_SUCCESS) std::memcpy(tail.data(),mapped,sizeof(tail));
  vk.UnmapMemory(device,staging.memory);
  Check(invalidation,"vkInvalidateMappedMemoryRanges.M1_QUANTUM");
  ControlCode result=ControlCode::STALE;
  if (current() && clock.activeRemaining && pc::TimeNs()<=deadline) {
    attempt.acceptPointer(0,[&] {
      if (pc::TimeNs()>deadline) return;
      result=root.Poll(FenceState::Signaled,tail.data(),tail.size(),0,accepted);
      if (result==ControlCode::OK) clock.AcceptQuantum(permit.id);
      busy.Complete(result==ControlCode::OK ? &tail : nullptr);
    });
  }
  fields << ",\"m1QuantumCompletion\":" << Words(tail) << ",\"m1QuantumOwnerCode\":" << int(result);
  if (result!=ControlCode::OK) {
    fields << ",\"m1QuantumReject\":\"OWNER_REJECTED_OR_REVOKED\"";
    root.Stop();
    return false;
  }
  return true;
}

std::future<M1ColdResult> Boot::CaptureM1Quantum(m1::SourceRootOwner &root, const grs::AcceptedRoot &accepted,
                            const grs::Words256 &sourceCompletion, uint32_t stateWords,
                            const std::array<uint32_t,32> &sourceCommand,std::optional<M1HotCapture::Page> ticket) {
  Checkpoint("M1_QUANTUM_COLD_CAPTURE");
  const uint32_t eventCount=accepted.completion[53];
  const auto &stored=accepted.completion;
  const std::array<uint32_t, 8> sizes{1024,stateWords*4,eventCount*256,uint32_t(buffers[kM1QPub].bytes),
                                    stateWords*4,sourceCompletion[53]*256,1024,128};
  VkDeviceSize payload=0;
  for (auto bytes : sizes) payload+=bytes;
  Require(eventCount<=stored[54] && sourceCompletion[53]<=sourceCompletion[54] &&
          payload<=16*1024*1024,"M1_COLD_RANGE");
  const auto gpuBytes=payload-sizes[6]-sizes[7];
  PublicationLease lease;
  Require(root.Acquire(accepted.publication,accepted.publicationGeneration,lease)==ControlCode::OK,
          "M1_COLD_LEASE");
  Allocate(kM1ColdStaging,gpuBytes,VK_BUFFER_USAGE_TRANSFER_DST_BIT,true);
  const auto &staging=buffers[kM1ColdStaging];
  auto reset=reinterpret_cast<PFN_vkResetCommandPool>(vk.GetDeviceProcAddr(device,"vkResetCommandPool"));
  Require(reset!=nullptr,"M1_COLD_RESET_FUNCTION");
  Check(reset(device,commandPool,0),"vkResetCommandPool.M1_COLD");
  vk.DestroyFence(device,fence,nullptr);
  fence=VK_NULL_HANDLE;
  VkFenceCreateInfo fenceInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
  Check(vk.CreateFence(device,&fenceInfo,nullptr,&fence),"vkCreateFence.M1_COLD");
  VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  begin.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  Check(vk.BeginCommandBuffer(command,&begin),"vkBeginCommandBuffer.M1_COLD");
  Barrier(kCompute,VK_ACCESS_SHADER_WRITE_BIT,kTransfer,VK_ACCESS_TRANSFER_READ_BIT);
  const size_t slots[]={kM1Tail,kM1Base,kM1QEvents,kM1QPub,kM1Candidate,kM1Events};
  VkDeviceSize at=0;
  for (size_t i=0;i<std::size(slots);++i) {
    if (!sizes[i]) continue;
    const VkBufferCopy copy{0,at,sizes[i]};
    vk.CmdCopyBuffer(command,buffers[slots[i]].handle,staging.handle,1,&copy);
    at+=sizes[i];
  }
  Barrier(kTransfer,VK_ACCESS_TRANSFER_WRITE_BIT,VK_PIPELINE_STAGE_HOST_BIT,VK_ACCESS_HOST_READ_BIT);
  Check(vk.EndCommandBuffer(command),"vkEndCommandBuffer.M1_COLD");
  VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
  submit.commandBufferCount=1;
  submit.pCommandBuffers=&command;
  Check(vk.QueueSubmit(queue,1,&submit,fence),"vkQueueSubmit.M1_COLD");
  inFlight=true;
  const auto wait=vk.WaitForFences(device,1,&fence,VK_TRUE,m1::ProgressClockV2::JobNs);
  if (wait==VK_SUCCESS || wait==VK_ERROR_DEVICE_LOST) inFlight=false;
  Check(wait,"vkWaitForFences.M1_COLD");
  std::vector<uint8_t> raw(size_t(payload),0);
  void *mapped=nullptr;
  Check(vk.MapMemory(device,staging.memory,0,VK_WHOLE_SIZE,0,&mapped),"vkMapMemory.M1_COLD");
  VkResult invalidation=VK_SUCCESS;
  if (!(staging.flags&VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
    VkMappedMemoryRange range{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,nullptr,staging.memory,0,VK_WHOLE_SIZE};
    invalidation=vk.InvalidateMappedMemoryRanges(device,1,&range);
  }
  if (invalidation==VK_SUCCESS) std::memcpy(raw.data(),mapped,size_t(gpuBytes));
  vk.UnmapMemory(device,staging.memory);
  Check(invalidation,"vkInvalidateMappedMemoryRanges.M1_COLD");
  Require(root.Release(accepted.publication,accepted.publicationGeneration,true)==ControlCode::OK,
          "M1_COLD_RELEASE");
  Require(std::memcmp(raw.data(),stored.data(),1024)==0,"M1_COLD_COMPLETION_CHANGED");
  std::memcpy(raw.data()+gpuBytes,sourceCompletion.data(),sizes[6]);
  std::memcpy(raw.data()+gpuBytes+sizes[6],sourceCommand.data(),sizes[7]);
  // The worker owns only sealed CPU bytes and the destination path.
  return std::async(std::launch::async,[path=attempt.m1ColdFile,sizes,raw=std::move(raw),
                                       capture=attempt.hotCapture,ticket]() mutable {
    try {
    const auto digest=Hash(raw.data(),raw.size());
    const auto temp=path+L".pending-"+std::to_wstring(GetCurrentProcessId());
    const HANDLE out=CreateFileW(temp.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
    Require(out!=INVALID_HANDLE_VALUE,"M1_COLD_CREATE");
    const auto write=[&](const void *src,size_t bytes) {
      DWORD n=0;
      return WriteFile(out,src,DWORD(bytes),&n,nullptr) ? n : 0;
    };
    const bool wrote=WriteM1Cold(sizes,digest,raw.data(),raw.size(),write) && FlushFileBuffers(out);
    CloseHandle(out);
    const bool finished=wrote && MoveFileExW(temp.c_str(),path.c_str(),MOVEFILE_WRITE_THROUGH);
    if (!finished) DeleteFileW(temp.c_str());
    Require(finished,"M1_COLD_WRITE_OR_DESTINATION_EXISTS");
    M1ColdResult result{Hex(digest.data()),raw.size()+kM1ColdEnvelopeBytes};
    if (ticket && !capture->SealCold(*ticket,raw,result.sha,pc::TimeNs()))
      return M1ColdResult{};
    return result;
    } catch (...) {
      if (ticket) capture->FailCold(*ticket);
      throw;
    }
  });
}
