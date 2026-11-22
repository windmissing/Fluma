// Explicit dry SOURCE diagnostic on the same PC device.
std::string Boot::ProbeM1Source(const Candidate &candidate) {
  constexpr uint32_t K = 192, Kw = 128, kColumn = 16*(K+Kw);
  constexpr uint32_t headerAt = kColumn+16*128, stateWords = headerAt+256;
  constexpr uint32_t workWords = 96+4096*64+stateWords+4096*16;
  constexpr uint32_t pubWords = 16*K+20*Kw+16*128+256;
  constexpr uint32_t passes = 8;
  constexpr auto storage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
  constexpr auto upload = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  constexpr auto block = 20*sizeof(uint32_t);
  constexpr char sourceSHA[] = "1912af9ed97e30d9eb158f0af2d6eaf5dcbabe6fc32bc04bd57b8226fc4c7162";
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
         << ",\"inputKind\":\"PC_DIAGNOSTIC_DRY_SOURCE\"";
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

  Checkpoint("M1_SOURCE_BUFFERS");
  Allocate(kM1Base, stateWords*4, storage | upload);
  Allocate(kM1Candidate, stateWords*4, storage);
  Allocate(kM1Work, workWords*4, storage | upload);
  Allocate(kM1Events, (K+1)*64*4, storage);
  Allocate(kM1Pub, pubWords*4, storage);
  Allocate(kM1Tail, 1024, storage | VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
  Allocate(kM1Uniform, stride*passes, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | upload);

  vk.DestroyDescriptorPool(device, descriptorPool, nullptr);
  descriptorPool = VK_NULL_HANDLE;
  const VkDescriptorPoolSize counts[] = {{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, passes*4},
                                         {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, passes}};
  VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
  pool.maxSets = passes;
  pool.poolSizeCount = 2;
  pool.pPoolSizes = counts;
  Check(vk.CreateDescriptorPool(device, &pool, nullptr, &descriptorPool), "vkCreateDescriptorPool.M1_SOURCE");
  std::array<VkDescriptorSetLayout, passes> layouts;
  layouts.fill(descriptorLayout);
  std::array<VkDescriptorSet, passes> bindings{};
  VkDescriptorSetAllocateInfo setInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
  setInfo.descriptorPool = descriptorPool;
  setInfo.descriptorSetCount = passes;
  setInfo.pSetLayouts = layouts.data();
  Check(vk.AllocateDescriptorSets(device, &setInfo, bindings.data()), "vkAllocateDescriptorSets.M1_SOURCE");
  for (uint32_t pass = 0; pass < passes; ++pass) {
    const size_t output = pass < 5 ? kM1Events : pass < 7 ? kM1Pub : kM1Tail;
    const size_t slots[] = {kM1Base, kM1Candidate, kM1Work, output, kM1Uniform};
    std::array<VkDescriptorBufferInfo, 5> ranges{};
    std::array<VkWriteDescriptorSet, 5> writes{};
    for (uint32_t binding = 0; binding < 5; ++binding) {
      const auto &b = buffers[slots[binding]];
      ranges[binding] = {b.handle, binding == 4 ? stride*pass : 0, binding == 4 ? block : b.bytes};
      writes[binding] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, bindings[pass], binding, 0, 1,
                         binding == 4 ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                         nullptr, &ranges[binding], nullptr};
    }
    vk.UpdateDescriptorSets(device, uint32_t(writes.size()), writes.data(), 0, nullptr);
  }

  auto reset = reinterpret_cast<PFN_vkResetCommandPool>(vk.GetDeviceProcAddr(device, "vkResetCommandPool"));
  auto fill = reinterpret_cast<PFN_vkCmdFillBuffer>(vk.GetDeviceProcAddr(device, "vkCmdFillBuffer"));
  Require(reset && fill, "M1_SOURCE_VULKAN_FUNCTIONS");
  Check(reset(device, commandPool, 0), "vkResetCommandPool.M1_SOURCE");
  vk.DestroyFence(device, fence, nullptr);
  fence = VK_NULL_HANDLE;
  VkFenceCreateInfo fenceInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
  Check(vk.CreateFence(device, &fenceInfo, nullptr, &fence), "vkCreateFence.M1_SOURCE");
  grs::Identity identity{};
  identity.run = narrow(pc::QpcTicks());
  identity.owner = narrow(GetCurrentThreadId());
  identity.context = identity.config = narrow(attempt.ticket);
  identity.surface = narrow(attempt.surface);
  identity.geometry = narrow(attempt.geometry);
  digestWords(sourceSHA, identity.shaderSha);
  digestWords(kAngleSha256, identity.angleSha);
  digestWords(m1::manifestSha, identity.layoutSha);
  m1::SourceRootOwner root;
  Require(root.Initialize(identity, K, Kw) == ControlCode::OK, "M1_SOURCE_OWNER_INIT");

  m1::ProgressClockV2 clock;
  Require(clock.BeginSession(), "M1_SOURCE_SESSION");
  clock.ready = true;
  const uint64_t pulse = attempt.sourcePulse();
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
  Require(clock.Claim(pc::TimeNs(), 0, 1, true, permit), "M1_SOURCE_PERMIT");
  RootRequest request;
  request.kind = 1;
  request.nonce = narrow(pc::QpcTicks());
  request.control = narrow(permit.id);
  request.frame = {1,0};
  request.eventArena = request.publicationSlot = 1;
  request.eventCapacity = K+1;
  request.dispatchCount = passes;
  request.barrierCount = 9;
  request.uploadedBytes = 128+12*4+3*8+8*4;
  request.carry = uint32_t(permit.carry);
  std::array<uint32_t,32> cmd{};
  cmd[0] = 1129534023;
  cmd[1] = 2;
  cmd[4] = identity.run.lo; cmd[5] = identity.run.hi;
  cmd[6] = cmd[8] = cmd[10] = cmd[14] = cmd[15] = 1;
  cmd[12] = identity.geometry.lo; cmd[13] = identity.geometry.hi;
  cmd[24] = uint32_t(pulse); cmd[25] = uint32_t(pulse >> 32);
  cmd[27] = 1;
  cmd[28] = identity.config.lo; cmd[29] = identity.config.hi;
  const double source[] = {64.5, 0.0, 0.125};
  std::memcpy(cmd.data()+16, source, sizeof(source));
  Require(root.Begin(request, cmd) == ControlCode::OK, "M1_SOURCE_OWNER_BEGIN");
  const auto deadline = permit.grantedNs+m1::ProgressClockV2::JobNs;
  VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  Check(vk.BeginCommandBuffer(command, &begin), "vkBeginCommandBuffer.M1_SOURCE");
  fill(command, buffers[kM1Base].handle, 0, headerAt*4, 0);
  vk.CmdUpdateBuffer(command, buffers[kM1Base].handle, headerAt*4, 1024, root.Header().data());
  fill(command, buffers[kM1Work].handle, 128, VK_WHOLE_SIZE, 0);
  vk.CmdUpdateBuffer(command, buffers[kM1Work].handle, 0, sizeof(cmd), cmd.data());
  for (uint32_t pass = 0; pass < passes; ++pass) {
    const std::array<uint32_t,20> uniforms{
      K,Kw,16*K,kColumn,headerAt,stateWords,pass,1048576,request.nonce.lo,request.nonce.hi,
      request.frame.lo,request.frame.hi,request.control.lo,request.control.hi,root.Bank(),
      request.publicationSlot,request.eventArena,request.carry,request.uploadedBytes,0};
    vk.CmdUpdateBuffer(command, buffers[kM1Uniform].handle, stride*pass, block, uniforms.data());
  }
  Barrier(kTransfer, VK_ACCESS_TRANSFER_WRITE_BIT, kCompute, kShaderRW | VK_ACCESS_UNIFORM_READ_BIT);
  vk.CmdBindPipeline(command, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
  for (uint32_t pass = 0; pass < passes; ++pass) {
    vk.CmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1, &bindings[pass], 0, nullptr);
    vk.CmdDispatch(command, (stateWords+63)/64, 1, 1);
    if (pass+1 < passes) Barrier(kCompute, VK_ACCESS_SHADER_WRITE_BIT, kCompute, kShaderRW);
  }
  Barrier(kCompute, VK_ACCESS_SHADER_WRITE_BIT, kTransfer, VK_ACCESS_TRANSFER_READ_BIT);
  const VkBufferCopy copy{0,0,1024};
  vk.CmdCopyBuffer(command, buffers[kM1Tail].handle, buffers[kStaging].handle, 1, &copy);
  Barrier(kTransfer, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_HOST_BIT, VK_ACCESS_HOST_READ_BIT);
  Check(vk.EndCommandBuffer(command), "vkEndCommandBuffer.M1_SOURCE");
  Checkpoint("M1_SOURCE_SUBMIT");
  Require(refresh() && pc::TimeNs() < deadline, "M1_SOURCE_EXPIRED_BEFORE_SUBMIT");
  VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
  submit.commandBufferCount = 1;
  submit.pCommandBuffers = &command;
  Check(vk.QueueSubmit(queue, 1, &submit, fence), "vkQueueSubmit.M1_SOURCE");
  inFlight = true;
  fields << ",\"m1QueueVkResult\":0,\"m1Nonce\":\"" << ((uint64_t(request.nonce.hi)<<32) | request.nonce.lo)
         << "\",\"m1DeadlineNs\":\"" << deadline << "\",\"m1PulseNs\":\"" << pulse << "\"";
  VkResult waited = VK_NOT_READY;
  uint64_t fenceNs = pc::TimeNs();
  while (fenceNs < deadline && refresh()) {
    waited = vk.WaitForFences(device, 1, &fence, VK_TRUE, std::min<uint64_t>(deadline-fenceNs, 2000000));
    fenceNs = pc::TimeNs();
    if (waited != VK_TIMEOUT) break;
  }
  if (waited == VK_SUCCESS || waited == VK_ERROR_DEVICE_LOST) inFlight = false;
  const bool fresh = refresh();
  fields << ",\"m1FenceVkResult\":" << int(waited) << ",\"m1FenceNs\":\"" << fenceNs
         << "\",\"m1PulseCount\":" << clock.pulses << ",\"m1PulseFault\":" << Quote(clock.fault);
  if (waited != VK_SUCCESS || fenceNs > deadline || !fresh) {
    root.Stop();
    return Receipt("FAILED", !fresh ? "M1_SOURCE_PERMIT_REVOKED" :
                   waited == VK_TIMEOUT || fenceNs > deadline ? "M1_SOURCE_DEADLINE_EXPIRED" : "M1_SOURCE_FENCE_FAILED");
  }
  Checkpoint("M1_SOURCE_FENCED_READBACK");
  const auto &staging = buffers[kStaging];
  void *mapped = nullptr;
  Check(vk.MapMemory(device, staging.memory, 0, VK_WHOLE_SIZE, 0, &mapped), "vkMapMemory.M1_SOURCE");
  VkResult invalidation = VK_SUCCESS;
  if (!(staging.flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
    VkMappedMemoryRange range{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE, nullptr, staging.memory, 0, VK_WHOLE_SIZE};
    invalidation = vk.InvalidateMappedMemoryRanges(device, 1, &range);
  }
  Words256 tail{};
  if (invalidation == VK_SUCCESS) std::memcpy(tail.data(), mapped, sizeof(tail));
  vk.UnmapMemory(device, staging.memory);
  Check(invalidation, "vkInvalidateMappedMemoryRanges.M1_SOURCE");
  Require(pc::TimeNs() <= deadline && refresh(), "M1_SOURCE_REVOKED_AT_ACCEPT");
  AcceptedRoot accepted;
  const auto result = root.Poll(FenceState::Signaled, tail.data(), tail.size(), 0, accepted);
  fields << ",\"m1Completion\":" << Words(tail) << ",\"m1OwnerCode\":" << int(result);
  Require(result == ControlCode::OK && accepted.sourceSuccess && tail[61] != 0, "M1_SOURCE_NOT_ADMITTED");
  fields << ",\"m1AdmittedQ\":" << tail[61] << ",\"m1PublicationGeneration\":" << accepted.publicationGeneration.lo;
  if (!attempt.presentBoot) return Receipt("M1_DRY_SOURCE_ACCEPTED", "");

  Checkpoint("M1_SOURCE_WSI");
  PublicationLease lease;
  Require(root.Acquire(accepted.publication, accepted.publicationGeneration, lease) == ControlCode::OK,
          "M1_SOURCE_PUBLICATION_ACQUIRE");
  const auto frame = pc::PresentBoot(vk, instance, candidate.physical, device, queue, candidate.family,
      surface, buffers[kM1Pub].handle, staging.handle, staging.memory, staging.flags, attempt.window,
      attempt.current, [&](const pc::Frame &visible) {
        fields << ",\"windowFrame\":{\"kind\":\"M1_DRY_SOURCE_DIAGNOSTIC\",\"acceptedRoot\":\""
               << ((uint64_t(lease.nonce.hi)<<32) | lease.nonce.lo) << "\",\"publicationSlot\":" << accepted.publication
               << ",\"generation\":\"" << ((uint64_t(lease.generation.hi)<<32) | lease.generation.lo)
               << "\",\"imageIndex\":" << visible.imageIndex << ",\"width\":" << visible.width
               << ",\"height\":" << visible.height << ",\"format\":" << visible.format
               << ",\"acquire\":" << int(visible.acquire) << ",\"submit\":" << int(visible.submit)
               << ",\"fence\":" << int(visible.fence) << ",\"present\":" << int(visible.present)
               << ",\"imagePresent\":" << int(visible.imagePresent) << ",\"pixel\":" << visible.pixel
               << ",\"presentEnterQpc\":\"" << visible.presentEnter << "\",\"presentExitQpc\":\""
               << visible.presentExit << "\"}";
        publish(Receipt("M1_DRY_SOURCE_PRESENT_QUEUED", ""));
      }, buffers[kM1Pub].bytes);
  Require(root.Release(accepted.publication, accepted.publicationGeneration, true) == ControlCode::OK,
          "M1_SOURCE_PUBLICATION_RELEASE");
  fields << ",\"m1PublicationReleased\":true,\"m1WsiResult\":" << int(frame.error)
         << ",\"m1WsiStage\":" << Quote(frame.stage);
  return frame.error == VK_SUCCESS ? Receipt("M1_DRY_SOURCE_PRESENT_RETIRED", "") :
                                    Receipt("WSI_FAILED", frame.stage);
}
