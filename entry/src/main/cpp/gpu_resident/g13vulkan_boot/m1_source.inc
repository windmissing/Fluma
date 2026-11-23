constexpr char kM1CandidateSHA[] = "b8f0e3275fc8de0569a6c946097e8f47263d52d24a1a6e6f3573ebe960f8c85c";
constexpr char kM1CommitSHA[] = "caf0614c3b2b56e3ad03d3bb4394347bbea9f2ec5692fce5ee1ddf1fdfd906ce";

constexpr bool SourceDescriptorsReady(VkResult allocated, size_t expected, size_t requested,
                                      size_t nonNull, bool current) {
  return allocated == VK_SUCCESS && requested == expected && nonNull == expected && current;
}

static_assert(SourceDescriptorsReady(VK_SUCCESS,16,16,16,true));
static_assert(!SourceDescriptorsReady(VK_SUCCESS,16,8,8,true));
static_assert(!SourceDescriptorsReady(VK_SUCCESS,16,16,15,true));
static_assert(!SourceDescriptorsReady(VK_ERROR_OUT_OF_POOL_MEMORY,16,16,16,true));
static_assert(!SourceDescriptorsReady(VK_SUCCESS,16,16,16,false));

std::string Boot::ProbeM1Source(const Candidate &candidate) {
  constexpr uint32_t K = 192, Kw = 128, kColumn = 16*(K+Kw);
  constexpr uint32_t headerAt = kColumn+16*128, stateWords = headerAt+256;
  constexpr uint32_t workWords = 96+4096*64+stateWords+4096*16;
  constexpr uint32_t pubWords = 16*K+20*Kw+16*128+256;
  constexpr uint32_t passes = 8;
  constexpr size_t banks[]{kM1Base,kM1Candidate};
  constexpr uint32_t setCount = uint32_t(std::size(banks))*passes;
  constexpr uint32_t storageBindingsPerSet = 4;
  static_assert(setCount == 16);
  constexpr auto storage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
  constexpr auto upload = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  constexpr auto block = 20*sizeof(uint32_t);
  constexpr char sourceSHA[] = "1912af9ed97e30d9eb158f0af2d6eaf5dcbabe6fc32bc04bd57b8226fc4c7162";
  const bool interactive = !attempt.m1QuantumCandidate.empty();
  auto narrow = [](uint64_t value) { return grs::U64{uint32_t(value), uint32_t(value >> 32)}; };
  auto digestWords = [](const char *hex, auto &words) {
    for (size_t i = 0; i < 32; ++i) {
      auto nibble = [](char c) { return uint32_t(c <= '9' ? c-'0' : c-'a'+10); };
      words[i/4] |= (16*nibble(hex[2*i])+nibble(hex[2*i+1])) << (8*(i%4));
    }
  };
  const auto &limits = candidate.properties.limits;
  Require(workWords*4 <= limits.maxStorageBufferRange, "M1_STORAGE_RANGE");
  const auto align = limits.minUniformBufferOffsetAlignment;
  Require(align && block <= limits.maxUniformBufferRange, "M1_UNIFORM_RANGE");
  const auto stride = ((block+align-1)/align)*align;
  std::ifstream input(std::filesystem::path(attempt.m1SourceModule), std::ios::binary | std::ios::ate);
  const auto size = input.tellg();
  Require(size > 0 && size <= 512*1024 && (size % 4) == 0, "M1_MODULE_FILE_SIZE");
  std::vector<uint32_t> module(size_t(size)/4);
  input.seekg(0);
  Require(bool(input.read(reinterpret_cast<char *>(module.data()), size)) &&
          Hex(Hash(module.data(), size_t(size)).data()) == sourceSHA, "M1_MODULE_SHA");
  fields << ",\"m1SourceModuleSHA\":" << Quote(sourceSHA)
         << ",\"inputKind\":" << Quote(interactive ? "WIN32_POINTER_HOLD" : "PC_DIAGNOSTIC_DRY_SOURCE");
  vk.DestroyPipeline(device, pipeline, nullptr);
  vk.DestroyShaderModule(device, shader, nullptr);
  pipeline = VK_NULL_HANDLE;
  shader = VK_NULL_HANDLE;
  VkShaderModuleCreateInfo createModule{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
  createModule.codeSize = size_t(size);
  createModule.pCode = module.data();
  Checkpoint("M1_SOURCE_MODULE");
  Check(vk.CreateShaderModule(device, &createModule, nullptr, &shader), "vkCreateShaderModule.M1_SOURCE");
  VkComputePipelineCreateInfo createPipeline{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
  createPipeline.layout = pipelineLayout;
  createPipeline.stage = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0,
                          VK_SHADER_STAGE_COMPUTE_BIT, shader, "main", nullptr};
  Checkpoint("M1_SOURCE_PIPELINE_ENTER");
  const auto built = vk.CreateComputePipelines(device, VK_NULL_HANDLE, 1, &createPipeline, nullptr, &pipeline);
  fields << ",\"m1PipelineVkResult\":" << int(built);
  Checkpoint("M1_SOURCE_PIPELINE_EXIT");
  Check(built, "vkCreateComputePipelines.M1_SOURCE");
  if (interactive) {
    fields << ",\"m1QuantumCandidateSHA\":" << Quote(kM1CandidateSHA)
           << ",\"m1QuantumCommitSHA\":" << Quote(kM1CommitSHA);
    Checkpoint("M1_QUANTUM_CANDIDATE_PIPELINE");
    m1QuantumPipelines[0] = M1Pipeline(attempt.m1QuantumCandidate, kM1CandidateSHA);
    Checkpoint("M1_QUANTUM_COMMIT_PIPELINE");
    m1QuantumPipelines[1] = M1Pipeline(attempt.m1QuantumCommit, kM1CommitSHA);
  }

  Checkpoint("M1_SOURCE_BUFFERS");
  const auto copyOut = interactive ? VK_BUFFER_USAGE_TRANSFER_SRC_BIT : 0;
  Allocate(kM1Base, stateWords*4, storage | upload | copyOut);
  Allocate(kM1Candidate, stateWords*4, storage | copyOut);
  Allocate(kM1Work, workWords*4, storage | upload | copyOut);
  Allocate(kM1Events, (K+1)*64*4, storage | copyOut);
  Allocate(kM1Pub, pubWords*4, storage);
  Allocate(kM1Tail, 1024, storage | VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
  Allocate(kM1Uniform, stride*passes, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | upload);
  if (interactive) PrepareM1Quantum(candidate);

  vk.DestroyDescriptorPool(device, descriptorPool, nullptr);
  descriptorPool = VK_NULL_HANDLE;
  const VkDescriptorPoolSize counts[] = {{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, setCount*storageBindingsPerSet},
                                         {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, setCount}};
  VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
  pool.maxSets = setCount;
  pool.poolSizeCount = 2;
  pool.pPoolSizes = counts;
  Check(vk.CreateDescriptorPool(device, &pool, nullptr, &descriptorPool), "vkCreateDescriptorPool.M1_SOURCE");
  std::array<VkDescriptorSetLayout, setCount> layouts;
  layouts.fill(descriptorLayout);
  std::array<VkDescriptorSet, setCount> bindings{};
  VkDescriptorSetAllocateInfo setInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
  setInfo.descriptorPool = descriptorPool;
  setInfo.descriptorSetCount = setCount;
  setInfo.pSetLayouts = layouts.data();
  const auto allocated=vk.AllocateDescriptorSets(device,&setInfo,bindings.data());
  const auto firstNull=std::find(bindings.begin(),bindings.end(),VK_NULL_HANDLE);
  const auto nonNull=std::count_if(bindings.begin(),bindings.end(),[](VkDescriptorSet set) {
    return set!=VK_NULL_HANDLE;
  });
  fields << ",\"m1SourceDescriptorAllocation\":{\"expectedSets\":" << setCount
         << ",\"requestedSets\":" << setInfo.descriptorSetCount << ",\"nonNullSets\":" << nonNull
         << ",\"firstNullIndex\":" << (firstNull==bindings.end() ? "null" : std::to_string(firstNull-bindings.begin()))
         << ",\"vkResult\":" << int(allocated) << "}";
  Checkpoint("M1_SOURCE_DESCRIPTOR_ALLOCATED");
  Check(allocated,"vkAllocateDescriptorSets.M1_SOURCE");
  Require(SourceDescriptorsReady(allocated,setCount,setInfo.descriptorSetCount,size_t(nonNull),attempt.current()),
          "M1_SOURCE_DESCRIPTOR_SETS_INCOMPLETE_OR_REVOKED");
  for (size_t base=0; base<std::size(banks); ++base) {
    for (uint32_t pass = 0; pass < passes; ++pass) {
      const size_t output = pass < 5 ? kM1Events : pass < 7 ? kM1Pub : kM1Tail;
      const size_t slots[] = {banks[base], banks[1-base], kM1Work, output, kM1Uniform};
      std::array<VkDescriptorBufferInfo, 5> ranges{};
      std::array<VkWriteDescriptorSet, 5> writes{};
      for (uint32_t binding = 0; binding < 5; ++binding) {
        const auto &b = buffers[slots[binding]];
        ranges[binding] = {b.handle, binding == 4 ? stride*pass : 0, binding == 4 ? block : b.bytes};
        writes[binding] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, bindings[base*passes+pass], binding, 0, 1,
                           binding == 4 ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                           nullptr, &ranges[binding], nullptr};
      }
      vk.UpdateDescriptorSets(device, uint32_t(writes.size()), writes.data(), 0, nullptr);
    }
  }
  fields << ",\"m1SourceDescriptorUpdates\":" << setCount;
  Checkpoint("M1_SOURCE_DESCRIPTOR_UPDATES_COMPLETE");

  auto reset = reinterpret_cast<PFN_vkResetCommandPool>(vk.GetDeviceProcAddr(device, "vkResetCommandPool"));
  auto fill = reinterpret_cast<PFN_vkCmdFillBuffer>(vk.GetDeviceProcAddr(device, "vkCmdFillBuffer"));
  Require(reset && fill, "M1_SOURCE_VULKAN_FUNCTIONS");
  VkFenceCreateInfo fenceInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
  grs::Identity identity{};
  identity.run = narrow(pc::QpcTicks());
  identity.owner = narrow(GetCurrentThreadId());
  identity.context = identity.config = narrow(attempt.ticket);
  identity.surface = narrow(attempt.surface);
  identity.geometry = narrow(attempt.geometry);
  if (interactive) {
    const std::string set = std::string(sourceSHA) + kM1CandidateSHA + kM1CommitSHA;
    digestWords(Hex(Hash(set.data(), set.size()).data()).c_str(), identity.shaderSha);
  } else {
    digestWords(sourceSHA, identity.shaderSha);
  }
  digestWords(kAngleSha256, identity.angleSha);
  digestWords(m1::manifestSha, identity.layoutSha);
  m1::SourceRootOwner root;
  Require(root.Initialize(identity, K, Kw) == ControlCode::OK, "M1_SOURCE_OWNER_INIT");

  m1::ProgressClockV2 clock;
  Require(clock.BeginSession(), "M1_SOURCE_SESSION");
  clock.ready = true;
  const auto setupFields=fields.str();
  PointerSample inputSample{};
  uint64_t pulse = attempt.sourcePulse();
  uint64_t delta = 0;
  if (interactive) {
    stage="M1_POINTER_READY";
    const bool opened=attempt.pointerReady(Receipt("RUNNING",""));
    stage="M1_POINTER_WAIT";
    Require(opened,"ATTEMPT_REVOKED");
    uint64_t anchor = 0, hold = 0, expired = 0;
    while (attempt.current()) {
      const auto next = attempt.pointer();
      const auto ns = attempt.sourcePulse();
      if (next.hold != hold) {
        hold = next.hold;
        anchor = 0;
      }
      if (hold && hold != expired && ns > next.timeNs) {
        if (anchor && ns != anchor) {
          if (ns > anchor && ns-anchor <= m1::ProgressClockV2::GapNs) {
            inputSample = next;
            pulse = ns;
            delta = ns-anchor;
            break;
          }
          expired = hold;
        }
        anchor = ns;
      }
      Sleep(1);
    }
    Require(inputSample.hold != 0, "ATTEMPT_REVOKED");
    fields << ",\"m1Hold\":\"" << inputSample.hold << "\",\"m1PointerSeq\":\"" << inputSample.sequence
           << "\",\"m1PointerNs\":\"" << inputSample.timeNs << "\",\"m1SourceDeltaNs\":\"" << delta << "\"";
  }
  const uint64_t received = pc::TimeNs();
  Require(pulse != 0 && received >= pulse && received-pulse <= m1::ProgressClockV2::GapNs,
          "M1_SOURCE_UI_PULSE_ABSENT");
  clock.Pulse(clock.session, 1, pulse, received);
  const auto refresh = [&]() {
    const auto now = pc::TimeNs();
    const auto ui = attempt.sourcePulse();
    if (ui > clock.uiNs && now >= ui && now-ui <= m1::ProgressClockV2::GapNs)
      clock.Pulse(clock.session, clock.sequence+1, ui, now);
    return attempt.current() && clock.Identity(clock.active.id, clock.session) && clock.Fresh(now);
  };
  m1::PlanGrantV2 permit;
  Require(clock.Claim(pc::TimeNs(),0,interactive ? inputSample.hold : 1,true,permit),"M1_SOURCE_PERMIT");
  uint64_t frameSerial=1;
  uint32_t sourceSerial=0;
  std::array<uint32_t,32> cmd{};
  Words256 tail{};
  AcceptedRoot accepted;
  const auto &staging=buffers[kStaging];
  const auto runSource=[&](const PointerSample &inputSample,uint64_t pulse,uint64_t delta) {
    Require(sourceSerial!=UINT32_MAX,"M1_SOURCE_SEQUENCE_EXHAUSTED");
    Check(reset(device,commandPool,0),"vkResetCommandPool.M1_SOURCE");
    vk.DestroyFence(device,fence,nullptr);
    fence=VK_NULL_HANDLE;
    Check(vk.CreateFence(device,&fenceInfo,nullptr,&fence),"vkCreateFence.M1_SOURCE");
    RootRequest request;
    request.kind = 1;
    request.nonce = narrow(pc::QpcTicks());
    request.control = narrow(permit.id);
    request.frame = narrow(frameSerial);
    request.eventArena = request.frame.lo&1u;
    request.publicationSlot = 1-root.Bank();
    request.eventCapacity = K+1;
    request.dispatchCount = passes;
    request.barrierCount = 9;
    request.uploadedBytes = 128+12*4+3*8+8*4;
    request.carry = uint32_t(permit.carry);
    cmd = {};
    cmd[0] = 1129534023;
    cmd[1] = 2;
    cmd[4] = identity.run.lo; cmd[5] = identity.run.hi;
    cmd[8] = cmd[14] = 1;
    cmd[6] = cmd[15] = ++sourceSerial;
    cmd[10] = interactive ? uint32_t(inputSample.hold) : 1;
    cmd[11] = interactive ? uint32_t(inputSample.hold>>32) : 0;
    cmd[12] = identity.geometry.lo; cmd[13] = identity.geometry.hi;
    cmd[24] = uint32_t(pulse); cmd[25] = uint32_t(pulse >> 32);
    cmd[27] = 1;
    if (interactive) {
      // Original producer: UI ns, sample sequence, diagnostic-position flags.
      cmd[22] = uint32_t(pulse);
      cmd[23] = uint32_t(pulse >> 32);
      cmd[24] = sourceSerial;
      cmd[25] = 0;
      cmd[26] = 1;
    }
    cmd[28] = identity.config.lo; cmd[29] = identity.config.hi;
    const double source[] = {interactive ? inputSample.x : 64.5, interactive ? inputSample.y : 0.0,
                             interactive ? 8.0*double(delta)/1000000000.0 : 0.125};
    std::memcpy(cmd.data()+16, source, sizeof(source));
    Require(root.Begin(request, cmd) == ControlCode::OK, "M1_SOURCE_OWNER_BEGIN");
    const auto deadline = permit.grantedNs+m1::ProgressClockV2::JobNs;
    VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    Check(vk.BeginCommandBuffer(command, &begin), "vkBeginCommandBuffer.M1_SOURCE");
    if (!root.AcceptedCount()) {
      fill(command, buffers[kM1Base].handle, 0, headerAt*4, 0);
      vk.CmdUpdateBuffer(command, buffers[kM1Base].handle, headerAt*4, 1024, root.Header().data());
      fill(command, buffers[kM1Work].handle, 128, VK_WHOLE_SIZE, 0);
    }
    vk.CmdUpdateBuffer(command, buffers[kM1Work].handle, 0, sizeof(cmd), cmd.data());
    for (uint32_t pass = 0; pass < passes; ++pass) {
      const std::array<uint32_t,20> uniforms{
        K,Kw,16*K,kColumn,headerAt,stateWords,pass,1048576,request.nonce.lo,request.nonce.hi,
        request.frame.lo,request.frame.hi,request.control.lo,request.control.hi,root.Bank(),
        request.publicationSlot,request.eventArena,request.carry,request.uploadedBytes,0};
      vk.CmdUpdateBuffer(command, buffers[kM1Uniform].handle, stride*pass, block, uniforms.data());
    }
    Barrier(kTransfer, VK_ACCESS_TRANSFER_WRITE_BIT, kCompute, kShaderRW | VK_ACCESS_UNIFORM_READ_BIT);
    Barrier(kCompute,VK_ACCESS_SHADER_WRITE_BIT,kCompute,kShaderRW);
    vk.CmdBindPipeline(command, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
    for (uint32_t pass = 0; pass < passes; ++pass) {
      vk.CmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1, &bindings[root.Bank()*passes+pass], 0, nullptr);
      vk.CmdDispatch(command, (stateWords+63)/64, 1, 1);
      if (pass+1 < passes) Barrier(kCompute, VK_ACCESS_SHADER_WRITE_BIT, kCompute, kShaderRW);
    }
    Barrier(kCompute, VK_ACCESS_SHADER_WRITE_BIT, kTransfer, VK_ACCESS_TRANSFER_READ_BIT);
    const VkBufferCopy copy{0,0,1024};
    vk.CmdCopyBuffer(command, buffers[kM1Tail].handle, buffers[kStaging].handle, 1, &copy);
    Barrier(kTransfer, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_HOST_BIT, VK_ACCESS_HOST_READ_BIT);
    Check(vk.EndCommandBuffer(command), "vkEndCommandBuffer.M1_SOURCE");
    Checkpoint("M1_SOURCE_SUBMIT");
    const auto sourceCurrent = [&] {
      return refresh() && (!interactive || attempt.pointer().hold == inputSample.hold);
    };
    if (!sourceCurrent() || pc::TimeNs()>=deadline) {
      root.Stop();
      AcceptedRoot discarded;
      root.Poll(FenceState::Signaled,nullptr,0,0,discarded);
      fields << ",\"m1SourceReject\":\"STALE_OR_LATE_BEFORE_SUBMIT\"";
      return false;
    }
    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &command;
    Check(vk.QueueSubmit(queue, 1, &submit, fence), "vkQueueSubmit.M1_SOURCE");
    inFlight = true;
    fields << ",\"m1QueueVkResult\":0,\"m1Nonce\":\"" << ((uint64_t(request.nonce.hi)<<32) | request.nonce.lo)
           << "\",\"m1DeadlineNs\":\"" << deadline << "\",\"m1PulseNs\":\"" << pulse << "\"";
    VkResult waited = VK_NOT_READY;
    uint64_t fenceNs = pc::TimeNs();
    while (fenceNs < deadline && sourceCurrent()) {
      waited = vk.WaitForFences(device, 1, &fence, VK_TRUE, std::min<uint64_t>(deadline-fenceNs, 2000000));
      fenceNs = pc::TimeNs();
      if (waited != VK_TIMEOUT) break;
    }
    if (waited == VK_SUCCESS || waited == VK_ERROR_DEVICE_LOST) inFlight = false;
    const bool fresh = sourceCurrent();
    fields << ",\"m1FenceVkResult\":" << int(waited) << ",\"m1FenceNs\":\"" << fenceNs
           << "\",\"m1PulseCount\":" << clock.pulses << ",\"m1PulseFault\":"
           << (clock.open ? "null" : Quote(clock.fault));
    if (waited != VK_SUCCESS || fenceNs > deadline || !fresh) {
      root.Stop();
      if (inFlight) {
        const auto drained=vk.WaitForFences(device,1,&fence,VK_TRUE,m1::ProgressClockV2::JobNs);
        if (drained==VK_SUCCESS || drained==VK_ERROR_DEVICE_LOST) inFlight=false;
        Check(drained,"vkWaitForFences.M1_SOURCE_RETIRE");
      }
      if (waited==VK_ERROR_DEVICE_LOST) Check(waited,"vkWaitForFences.M1_SOURCE");
      AcceptedRoot discarded;
      root.Poll(FenceState::Signaled,nullptr,0,0,discarded);
      fields << ",\"m1SourceReject\":" << Quote(!fresh ? "PERMIT_REVOKED" : "FENCE_OR_DEADLINE");
      return false;
    }
    Checkpoint("M1_SOURCE_FENCED_READBACK");
    void *mapped = nullptr;
    Check(vk.MapMemory(device, staging.memory, 0, VK_WHOLE_SIZE, 0, &mapped), "vkMapMemory.M1_SOURCE");
    VkResult invalidation = VK_SUCCESS;
    if (!(staging.flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
      VkMappedMemoryRange range{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE, nullptr, staging.memory, 0, VK_WHOLE_SIZE};
      invalidation = vk.InvalidateMappedMemoryRanges(device, 1, &range);
    }
    tail = {};
    if (invalidation == VK_SUCCESS) std::memcpy(tail.data(), mapped, sizeof(tail));
    vk.UnmapMemory(device, staging.memory);
    Check(invalidation, "vkInvalidateMappedMemoryRanges.M1_SOURCE");
    auto result = ControlCode::STALE;
    const auto accept = [&] {
      if (pc::TimeNs() <= deadline)
        result = root.Poll(FenceState::Signaled, tail.data(), tail.size(), 0, accepted);
    };
    if (pc::TimeNs()>deadline || !sourceCurrent() || !attempt.acceptPointer(inputSample.hold,accept)
        || result==ControlCode::STALE) {
      root.Stop();
      AcceptedRoot discarded;
      root.Poll(FenceState::Signaled,nullptr,0,0,discarded);
      fields << ",\"m1SourceReject\":\"STALE_OR_LATE_AT_ACCEPT\"";
      return false;
    }
    fields << ",\"m1Completion\":" << Words(tail) << ",\"m1OwnerCode\":" << int(result);
    Require(result == ControlCode::OK,"M1_SOURCE_COMPLETION_INVALID");
    fields << ",\"m1AdmittedQ\":" << tail[61] << ",\"m1PublicationGeneration\":" << accepted.publicationGeneration.lo;
    return true;
  };
  Require(runSource(inputSample,pulse,delta) && accepted.sourceSuccess && tail[61],
          "M1_SOURCE_NOT_ADMITTED");
  uint32_t ordinal=0;
  if (interactive && !RunM1Quantum(candidate, root, clock, permit, refresh, accepted, frameSerial, ordinal))
    return Receipt("FAILED", "M1_QUANTUM_NOT_ACCEPTED");
  if (interactive && !attempt.m1ColdFile.empty()) CaptureM1Quantum(root, accepted, tail, stateWords, cmd);
  if (!attempt.presentBoot) return Receipt("M1_DRY_SOURCE_ACCEPTED", "");

  Checkpoint("M1_SOURCE_WSI");
  PublicationLease lease;
  Require(root.Acquire(accepted.publication, accepted.publicationGeneration, lease) == ControlCode::OK,
          "M1_SOURCE_PUBLICATION_ACQUIRE");
  bool leased=true;
  bool nextRootFailed=false;
  bool coldPresent=true;
  uint64_t anchor=pulse, anchorHold=inputSample.hold, expiredHold=0;
  const auto publicationBuffer=[&] {
    return buffers[!interactive ? kM1Pub : accepted.publication ? kM1QPubAlt : kM1QPub].handle;
  };
  std::function<VkBuffer()> nextPublication;
  if (interactive && !attempt.m1SinglePresent && attempt.m1ColdFile.empty()) {
    nextPublication=[&]() -> VkBuffer {
      Require(root.Release(accepted.publication,accepted.publicationGeneration,true)==ControlCode::OK,
              "M1_QUANTUM_FRAME_RELEASE");
      leased=false;
      fields.str(setupFields);
      fields.seekp(0,std::ios::end);
      if (coldPresent) {
        // The first graphics pipeline is cold work. Resume with a new
        // anchor instead of turning its elapsed time into physics debt.
        Require(clock.BeginSession(),"M1_PRESENT_SESSION");
        clock.ready=true;
        coldPresent=false;
        fields << ",\"m1ClockReanchoredAfterColdPresent\":true";
      }
      if (!clock.activeRemaining && refresh()) {
        const auto pointer=attempt.pointer();
        const auto ui=attempt.sourcePulse();
        if (pointer.hold!=anchorHold) {
          anchorHold=pointer.hold;
          anchor=0;
        }
        if (pointer.hold && pointer.hold!=expiredHold && ui>pointer.timeNs) {
          if (anchor && ui!=anchor) {
            if (ui<anchor || ui-anchor>m1::ProgressClockV2::GapNs) {
              expiredHold=pointer.hold;
            } else if (clock.Claim(pc::TimeNs(),0,pointer.hold,true,permit)) {
              ++frameSerial;
              ordinal=0;
              fields << ",\"m1Hold\":\"" << pointer.hold << "\",\"m1PointerSeq\":\"" << pointer.sequence
                     << "\",\"m1PointerNs\":\"" << pointer.timeNs << "\",\"m1SourceDeltaNs\":\"" << ui-anchor << "\"";
              if (!runSource(pointer,ui,ui-anchor)) {
                // A revoked SOURCE appended no accepted prefix to this frame.
                // Leave its granted quanta in the plan and start with prefix zero.
                publish(Receipt("M1_SOURCE_REVOKED",""));
              }
            }
          }
          anchor=ui;
        }
      }
      if (!RunM1Quantum(candidate,root,clock,permit,refresh,accepted,frameSerial,ordinal)) {
        nextRootFailed=attempt.current();
        if (nextRootFailed) {
          publish(Receipt("FAILED","M1_QUANTUM_CONTINUATION_FAILED"));
          while (attempt.current()) Sleep(16);
        }
        return VK_NULL_HANDLE;
      }
      Require(root.Acquire(accepted.publication,accepted.publicationGeneration,lease)==ControlCode::OK,
              "M1_QUANTUM_FRAME_ACQUIRE");
      leased=true;
      return publicationBuffer();
    };
  }
  const auto frame = pc::PresentBoot(vk, instance, candidate.physical, device, queue, candidate.family,
      surface, publicationBuffer(), staging.handle, staging.memory, staging.flags, attempt.window,
      attempt.current, [&](const pc::Frame &visible) {
        const auto wide = [](grs::U64 value) { return (uint64_t(value.hi)<<32) | value.lo; };
        fields << ",\"windowFrame\":{\"kind\":" << Quote(interactive ? "M1_QUANTUM_WINDOW" : "M1_DRY_SOURCE_DIAGNOSTIC")
               << ",\"acceptedRoot\":\"" << wide(lease.nonce) << "\",\"publicationSlot\":" << accepted.publication
               << ",\"run\":\"" << wide(lease.identity.run) << "\",\"config\":\"" << wide(lease.identity.config)
               << "\",\"sourceCommit\":\"" << wide(lease.source) << "\",\"physicsCommit\":\"" << wide(lease.physics)
               << "\",\"generation\":\"" << wide(lease.generation)
               << "\",\"imageIndex\":" << visible.imageIndex << ",\"width\":" << visible.width
               << ",\"height\":" << visible.height << ",\"format\":" << visible.format
               << ",\"acquire\":" << int(visible.acquire) << ",\"submit\":" << int(visible.submit)
               << ",\"fence\":" << int(visible.fence) << ",\"present\":" << int(visible.present)
               << ",\"imagePresent\":" << int(visible.imagePresent) << ",\"pixel\":" << visible.pixel
               << ",\"presentEnterQpc\":\"" << visible.presentEnter << "\",\"presentExitQpc\":\""
               << visible.presentExit << "\"}";
        publish(Receipt(interactive ? "M1_QUANTUM_PRESENT_QUEUED" : "M1_DRY_SOURCE_PRESENT_QUEUED", ""));
      }, buffers[kM1Pub].bytes, !interactive, nextPublication);
  const bool usable = frame.retire != VK_ERROR_DEVICE_LOST && frame.error != VK_ERROR_DEVICE_LOST;
  if (usable && leased)
    Require(root.Release(accepted.publication, accepted.publicationGeneration, true) == ControlCode::OK,
            "M1_SOURCE_PUBLICATION_RELEASE");
  else if (!usable)
    root.ContextLost();
  fields << ",\"m1PublicationReleased\":" << (usable ? "true" : "false") << ",\"m1WsiResult\":" << int(frame.error)
         << ",\"m1WsiRetireVkResult\":" << int(frame.retire) << ",\"m1WsiStage\":" << Quote(frame.stage);
  if (nextRootFailed) return Receipt("FAILED","M1_QUANTUM_CONTINUATION_FAILED");
  return frame.error == VK_SUCCESS ? Receipt(interactive ? "M1_QUANTUM_PRESENT_RETIRED" : "M1_DRY_SOURCE_PRESENT_RETIRED", "") :
                                    Receipt("WSI_FAILED", frame.stage);
}
