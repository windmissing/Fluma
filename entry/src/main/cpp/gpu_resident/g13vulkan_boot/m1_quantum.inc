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
constexpr uint32_t angleWitnessWord=replay+256, angleWitnessWords=12;
constexpr uint32_t packetPublicationWords=20*kw;
constexpr uint32_t capacity=4*(48*k+16*kw+254)+k;
constexpr uint32_t block=192;
constexpr auto storage=VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
constexpr auto transferDst=VK_BUFFER_USAGE_TRANSFER_DST_BIT;
constexpr const char *timingPoints[]{
    "ROOT_BEGIN","UPLOAD_DONE","PREPARE_DONE","WATER_DONE","FROZEN_DONE","SORT_DONE",
    "WATER_MOTION_DONE","MOTION_DONE","BED_DONE","ARRIVAL_SORT_DONE","ARRIVE_DONE","FINAL_AUDIT_DONE",
    "INDEPENDENT_INVENTORY_DONE","INDEPENDENT_ANGLE_DONE","INDEPENDENT_SOURCE_LEDGER_DONE",
    "REPLAY_BEGIN","REPLAY_COPY_DONE","REPLAY_EVENTS_DONE","REPLAY_DONE","PACK_DONE","SEAL_DONE","TAIL_COPY_DONE"};
constexpr uint32_t timingStages[]{1,4,5,9,10,11,14,17,18,19,29,30,31,21,22,23,24,25,27};
static_assert(std::size(timingPoints)==std::size(timingStages)+3);
constexpr const char *sourceTimingPoints[]{"ROOT_BEGIN","UPLOAD_DONE","SOURCE_PASSES_DONE","TAIL_COPY_DONE"};
}

void Boot::ReportM1Timing(VkResult retired,bool accepted) {
  if (!m1TimedNonce) return;
  const bool readable=retired==VK_SUCCESS && m1TimedFence==fence;
  struct Point { uint64_t tick=0,available=0; };
  std::ostringstream row;
  row << "{\"stage\":\"M1_GPU_TIMESTAMPS\",\"run\":\"" << m1Run << "\",\"geometry\":\"" << attempt.geometry
      << "\",\"nonce\":\"" << m1TimedNonce << "\",\"fence\":\"" << reinterpret_cast<uintptr_t>(m1TimedFence)
      << "\",\"kind\":" << Quote(m1TimedSource ? "SOURCE" : "QUANTUM")
      << ",\"queue\":\"" << reinterpret_cast<uintptr_t>(queue)
      << "\",\"pool\":\"" << reinterpret_cast<uintptr_t>(m1TimingPool)
      << "\",\"poolGeneration\":\"1\",\"resetSerial\":\"" << m1TimingResetSerial
      << "\",\"acceptedRoot\":" << (accepted && attempt.m1PrivateReference.empty() ?
          Quote(std::to_string(m1TimedNonce)) : "null")
      << ",\"fenceVkResult\":" << int(retired) << ",\"validBits\":" << m1TimingBits
      << ",\"periodNs\":" << m1TimingPeriod << ",\"points\":[";
  bool comma=false;
  const auto points=m1TimedSource ? m1pc::sourceTimingPoints : m1pc::timingPoints;
  const auto count=m1TimedSource ? std::size(m1pc::sourceTimingPoints) : std::size(m1pc::timingPoints);
  for (uint32_t i=0;i<count;++i) {
    if (!(m1TimingMask&(uint64_t{1}<<i))) continue;
    Point value;
    const auto result=readable ? m1ReadTiming(device,m1TimingPool,i,1,sizeof(value),&value,sizeof(value),
        VK_QUERY_RESULT_64_BIT|VK_QUERY_RESULT_WITH_AVAILABILITY_BIT) : VK_NOT_READY;
    const bool available=result==VK_SUCCESS && value.available;
    if (comma) row << ',';
    comma=true;
    row << "{\"point\":" << Quote(points[i]) << ",\"vkResult\":" << int(result)
        << ",\"available\":" << (available ? "true" : "false")
        << ",\"tick\":" << (available ? Quote(std::to_string(value.tick)) : "null") << '}';
  }
  publish(row.str()+"]}");
  m1TimedNonce=0;
}

void Boot::PrepareM1Quantum(const Candidate &candidate) {
  using namespace m1pc;
  auto &passes=m1QuantumPasses;
  const bool groupSort=attempt.m1Compact && (attempt.m1InventoryV1 || attempt.m1Integrated);
  Require(!groupSort || candidate.properties.limits.maxComputeSharedMemorySize>=4096,"M1_GROUP_SORT_SHARED_LIMIT");
  m1::EachQuantumPass(stateWords,pad,kw,[&](m1::QuantumPassOp p) { passes.push_back(p); },
                      groupSort,attempt.m1AuditBreakdown);
  const auto &limits = candidate.properties.limits;
  if (attempt.m1StageTiming) {
    VkResult created=VK_NOT_READY;
    m1TimingBits=candidate.timestampBits;
    m1TimingPeriod=limits.timestampPeriod;
    auto create=reinterpret_cast<PFN_vkCreateQueryPool>(vk.GetDeviceProcAddr(device,"vkCreateQueryPool"));
    m1DestroyTiming=reinterpret_cast<PFN_vkDestroyQueryPool>(vk.GetDeviceProcAddr(device,"vkDestroyQueryPool"));
    m1ResetTiming=reinterpret_cast<PFN_vkCmdResetQueryPool>(vk.GetDeviceProcAddr(device,"vkCmdResetQueryPool"));
    m1WriteTiming=reinterpret_cast<PFN_vkCmdWriteTimestamp>(vk.GetDeviceProcAddr(device,"vkCmdWriteTimestamp"));
    m1ReadTiming=reinterpret_cast<PFN_vkGetQueryPoolResults>(vk.GetDeviceProcAddr(device,"vkGetQueryPoolResults"));
    if (m1TimingBits && m1TimingBits<=64 && create && m1DestroyTiming && m1ResetTiming && m1WriteTiming && m1ReadTiming) {
      VkQueryPoolCreateInfo query{VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};
      query.queryType=VK_QUERY_TYPE_TIMESTAMP;
      query.queryCount=uint32_t(std::size(timingPoints));
      created=create(device,&query,nullptr,&m1TimingPool);
      if (created!=VK_SUCCESS) m1TimingPool=VK_NULL_HANDLE;
    }
    fields << ",\"m1TimingValidBits\":" << m1TimingBits << ",\"m1TimingCreateVkResult\":" << int(created);
  }
  Require(capacity*256ull <= limits.maxStorageBufferRange && block <= limits.maxUniformBufferRange &&
          limits.minUniformBufferOffsetAlignment, "M1_QUANTUM_RESOURCE_LIMIT");
  const auto align = limits.minUniformBufferOffsetAlignment;
  const auto stride = (block+align-1)/align*align;
  const auto pubBytes = buffers[kM1Pub].bytes;
  const auto copyOut = attempt.m1ColdFile.empty() && attempt.m1ReplayPrefix.empty() &&
      attempt.m1PrivateReference.empty() && !attempt.m1Observation.frame
      ? 0 : VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  Allocate(kM1QScratch, stateWords*4, storage);
  Allocate(kM1QWork, (angleWitnessWord+(attempt.m1AngleWitness ? angleWitnessWords : 0))*4,
           storage | transferDst | (attempt.m1AngleWitness || !attempt.m1PrivateReference.empty() ?
           VK_BUFFER_USAGE_TRANSFER_SRC_BIT : 0));
  Allocate(kM1QEvents, capacity*256ull, storage | transferDst | copyOut);
  Allocate(kM1QUniform, stride*passes.size(), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | transferDst);
  Allocate(kM1QPub, pubBytes, storage | copyOut);
  Allocate(kM1QPubAlt, pubBytes, storage | copyOut);
  if (attempt.m1Observation.packetId)
    Allocate(kM1PacketStaging,packetPublicationWords*4,VK_BUFFER_USAGE_TRANSFER_DST_BIT,true);

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
  if (attempt.m1Integrated) {
    static_assert(std::size(kAngleWords)==15361*4);
    for (size_t i=4;i<std::size(kAngleWords);i+=4)
      Require(int32_t(kAngleWords[i-4])<=int32_t(kAngleWords[i]),"M1_TANGENT_TABLE_ORDER");
  }
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
                        uint64_t &frameSerial, uint32_t &ordinal, std::optional<uint32_t> frozenCarry) {
  using namespace m1pc;
  m1Retirement={};
  m1::QuantumParameters p;
  p.kappa=attempt.m1Kappa;
  const auto &passes=m1QuantumPasses;
  const auto &sets=m1QuantumSets;
  const uint32_t count=uint32_t(passes.size());
  const auto align=candidate.properties.limits.minUniformBufferOffsetAlignment;
  const auto stride=(block+align-1)/align*align;
  const auto baseBank=root.Bank();
  const auto previous=accepted.completion;
  Require(!inFlight,"M1_QUANTUM_RETIRE_PENDING");
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
    m1Reject=D1Ended() ? "D1_END_INTAKE" : !attempt.current() ? "WINDOW_REVOKED" : !refresh() ? clock.fault : nullptr;
    if (m1Reject) {
      if (SuspendM1Hold(clock,root) && root.State()==FlightState::Idle && !root.HasLeases())
        m1Retirement={0,pc::QpcTicks(),false,true}; // No new GPU submission.
      fields << ",\"m1QuantumReject\":" << Quote(m1Reject);
      M1RootTrace("Quantum",0,clock,{},accepted,{});
      return false;
    }
    if (ClaimM1(clock,0,false,permit)) {
      ++frameSerial;
      ordinal=0;
      break;
    }
    Sleep(1);
  }
  p.requestedAngle=permit.angle;
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
  // The private comparison freezes the shader projection. Deadline and
  // authorization always use the grant just obtained from the real clock.
  r.carry=frozenCarry.value_or(uint32_t(permit.carry));
  // Match the original scalar-uniform accounting in the native Completion ABI.
  r.uploadedBytes=26*4+28+8*4+4;
  for (const auto &op : passes)
    r.uploadedBytes+=4+(op.color!=UINT32_MAX ? 4 : 0)+(op.sortK ? 8 : 0);
  Require(root.BeginQuantum(r,p.requestedAngle)==ControlCode::OK,"M1_QUANTUM_OWNER_BEGIN");
  const uint64_t deadline=permit.grantedNs+m1::ProgressClockV2::JobNs;
  M1WaitLog log;
  log.recordBegin=pc::QpcTicks();
  VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  begin.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  Check(vk.BeginCommandBuffer(command,&begin),"vkBeginCommandBuffer.M1_QUANTUM");
  const auto mark=[&](uint32_t index,VkPipelineStageFlagBits stage=VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT) {
    if (!m1TimingPool) return;
    m1WriteTiming(command,stage,m1TimingPool,index);
    m1TimingMask|=uint64_t{1}<<index;
  };
  if (m1TimingPool) {
    m1TimingMask=0;
    m1ResetTiming(command,m1TimingPool,0,uint32_t(std::size(timingPoints)));
    ++m1TimingResetSerial;
    mark(0,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
  }
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
  mark(1);
  Barrier(kCompute,VK_ACCESS_SHADER_WRITE_BIT,kCompute,kShaderRW);
  const VkCommandBuffer cmd=command;
  std::array<uint32_t,std::size(timingStages)> lastStage{};
  if (m1TimingPool) {
    lastStage.fill(UINT32_MAX);
    for (uint32_t i=0;i<count;++i)
      for (uint32_t point=0;point<std::size(timingStages);++point)
        if (passes[i].stage==timingStages[point]) lastStage[point]=i;
  }
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
    if (m1TimingPool)
      for (uint32_t point=0;point<std::size(timingStages);++point)
        if (lastStage[point]==i) mark(point+2);
  }
  Barrier(kCompute,VK_ACCESS_SHADER_WRITE_BIT,kTransfer,VK_ACCESS_TRANSFER_READ_BIT);
  const VkBufferCopy tailCopy{0,0,1024};
  const auto &staging=buffers[kStaging];
  vk.CmdCopyBuffer(cmd,buffers[kM1Tail].handle,staging.handle,1,&tailCopy);
  if (attempt.m1AngleWitness) {
    const VkBufferCopy witness{angleWitnessWord*4ull,1024,angleWitnessWords*4ull};
    Require(staging.bytes>=witness.dstOffset+witness.size,"M1_ANGLE_WITNESS_RANGE");
    vk.CmdCopyBuffer(cmd,work,staging.handle,1,&witness);
  }
  if (attempt.m1Observation.packetId) {
    const VkBufferCopy waterPublication{16*k*4,0,packetPublicationWords*4};
    vk.CmdCopyBuffer(cmd,buffers[pubs[r.publicationSlot]].handle,buffers[kM1PacketStaging].handle,1,&waterPublication);
  }
  Barrier(kTransfer,VK_ACCESS_TRANSFER_WRITE_BIT,VK_PIPELINE_STAGE_HOST_BIT,VK_ACCESS_HOST_READ_BIT);
  mark(uint32_t(std::size(timingPoints)-1));
  Check(vk.EndCommandBuffer(cmd),"vkEndCommandBuffer.M1_QUANTUM");
  stage="M1_QUANTUM_SUBMIT";
  fields << ",\"m1QuantumNonce\":\"" << nonce << "\",\"m1QuantumDispatches\":" << count
         << ",\"m1QuantumUiNs\":\"" << clock.uiNs << "\""
         << ",\"m1QuantumEventStart\":" << r.eventStart << ",\"m1QuantumDeadlineNs\":\"" << deadline << "\"";
  const auto guard = [&]() -> const char * {
    if (!attempt.current()) return "WINDOW_REVOKED";
    if (!refresh()) return clock.fault;
    if (!clock.Identity(permit.id,permit.session) || !clock.activeRemaining) return "PLAN_SESSION_MISMATCH";
    return nullptr;
  };
  log.recordEnd=pc::QpcTicks();
  const auto reject=[&] {
    log.stopQpc=pc::QpcTicks();
    if (SuspendM1Hold(clock,root)) clock.Revoke(m1Reject);
    else attempt.disarmM1Input();
    RetireM1Root(root,nonce,log);
    if (!inFlight) ReportM1Timing(log.retireEnter ? log.retire : log.last,false);
    fields << ",\"m1QuantumReject\":" << Quote(m1Reject);
    M1RootTrace("Quantum",nonce,clock,permit,accepted,log);
    return false;
  };
  m1Reject=pc::TimeNs()>=deadline ? "ROOT_DEADLINE" : guard();
  if (m1Reject) return reject();
  VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
  submit.commandBufferCount=1;
  submit.pCommandBuffers=&cmd;
  SubmitM1Root(submit,0,deadline,nonce,log);
  if (m1Reject) return reject();
  if (log.submit!=VK_SUCCESS) {
    m1Reject=log.submit==VK_ERROR_DEVICE_LOST ? "DEVICE_LOST" : "SUBMIT_VK_ERROR";
    return reject();
  }
  inFlight=true;
  if (m1TimingPool) {
    m1TimedNonce=nonce;
    m1TimedFence=fence;
    m1TimedSource=false;
  }
  M1BusyFence busy{attempt.hotCapture,M1HotCapture::Phase::QuantumFence,nonce};
  m1Reject=AwaitM1Fence(deadline,guard,log);
  if (attempt.hotCapture) attempt.hotCapture->StopBusy();
  fields << ",\"m1QuantumFenceVkResult\":" << int(log.last)
         << ",\"m1QuantumFenceNs\":\"" << log.fenceNs << "\"";
  if (m1Reject) return reject();
  void *mapped=nullptr;
  Check(vk.MapMemory(device,staging.memory,0,VK_WHOLE_SIZE,0,&mapped),"vkMapMemory.M1_QUANTUM");
  VkResult invalidation=VK_SUCCESS;
  if (!(staging.flags&VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
    VkMappedMemoryRange range{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,nullptr,staging.memory,0,VK_WHOLE_SIZE};
    invalidation=vk.InvalidateMappedMemoryRanges(device,1,&range);
  }
  Words256 tail{};
  std::array<uint32_t,angleWitnessWords> angleWitness{};
  if (invalidation==VK_SUCCESS) {
    std::memcpy(tail.data(),mapped,sizeof(tail));
    if (attempt.m1AngleWitness)
      std::memcpy(angleWitness.data(),static_cast<const uint8_t *>(mapped)+sizeof(tail),sizeof(angleWitness));
  }
  vk.UnmapMemory(device,staging.memory);
  Check(invalidation,"vkInvalidateMappedMemoryRanges.M1_QUANTUM");
  ControlCode result=ControlCode::STALE;
  m1Reject=guard();
  if (!m1Reject && !attempt.acceptPointer(0,[&] {
      if (pc::TimeNs()>deadline) { m1Reject="ACCEPT_DEADLINE"; return; }
      if (!refresh()) { m1Reject=clock.fault; return; }
      if (!clock.Identity(permit.id,permit.session) || !clock.activeRemaining) {
        m1Reject="PLAN_SESSION_MISMATCH"; return;
      }
      if (pc::TimeNs()>deadline) { m1Reject="ACCEPT_DEADLINE"; return; }
      grs::AcceptedRoot candidate;
      result=root.Poll(FenceState::Signaled,tail.data(),tail.size(),0,candidate);
      if (result==ControlCode::OK) {
        clock.AcceptQuantum(permit.id);
        accepted=candidate;
      }
      busy.Complete(result==ControlCode::OK ? &tail : nullptr);
    }))
    m1Reject="WINDOW_REVOKED";
  fields << ",\"m1QuantumCompletion\":" << Words(tail) << ",\"m1QuantumOwnerCode\":" << int(result);
  if (!m1Reject && result!=ControlCode::OK) m1Reject="TAIL_REJECTED";
  if (m1Reject) return reject();
  log.stopQpc=pc::QpcTicks();
  M1RootTrace("Quantum",nonce,clock,permit,accepted,log);
  if (attempt.m1StageTiming)
    fields << ",\"m1QuantumWork\":{\"baseSand\":" << previous[88] << ",\"baseWater\":" << previous[90]
           << ",\"liveSand\":" << tail[88] << ",\"liveWater\":" << tail[90]
           << ",\"events\":" << tail[53]-tail[52] << ",\"sourceLedgerRows\":" << tail[55]
           << ",\"eventStart\":" << tail[52] << ",\"motionRows\":" << tail[155]
           << ",\"arrivalRows\":" << tail[156] << ",\"dispatches\":" << count << '}';
  if (attempt.m1Observation.packetId) {
    m1PacketSample={};
    const auto &packet=buffers[kM1PacketStaging];
    void *packetWords=nullptr;
    const auto mappedPacket=vk.MapMemory(device,packet.memory,0,VK_WHOLE_SIZE,0,&packetWords);
    if (mappedPacket==VK_SUCCESS) {
      const VkMappedMemoryRange range{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,nullptr,packet.memory,0,VK_WHOLE_SIZE};
      const auto invalidated=(packet.flags&VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) ? VK_SUCCESS :
          vk.InvalidateMappedMemoryRanges(device,1,&range);
      if (invalidated==VK_SUCCESS) {
        const auto *w=static_cast<const uint32_t *>(packetWords);
        m1PacketSample.nonce=nonce;
        for (uint32_t i=0;i<std::min(kw,tail[90]);++i)
          if (w[i] && w[5*kw+i]==attempt.m1Observation.packetId && w[11*kw+i]==attempt.m1Observation.packetGeneration) {
            m1PacketSample={nonce,w[i],w[16*kw+i],w[17*kw+i]};
            break;
          }
      }
      vk.UnmapMemory(device,packet.memory);
    }
  }
  if (attempt.m1AngleWitness) {
    const int32_t lo=int32_t(angleWitness[2]), hi=int32_t(angleWitness[3]);
    const bool valid=g13::U64(angleWitness.data())==nonce && pc::ValidM1Angle(lo) && pc::ValidM1Angle(hi) &&
        lo<=hi && angleWitness[4]<=127 && angleWitness[5]<=127 &&
        std::clamp(permit.angle,lo,hi)==int32_t(tail[107]);
    fields << ",\"m1AngleWitness\":{\"state\":" << Quote(valid ? "SAME_ROOT" : "IDENTITY_OR_RANGE_MISMATCH")
           << ",\"nonce\":\"" << nonce << "\",\"slot\":" << accepted.publication
           << ",\"generation\":\"" << g13::U64(tail.data()+36)
           << "\",\"requestedAngle256\":" << permit.angle << ",\"acceptedAngle256\":" << int32_t(tail[107])
           << ",\"rawWords\":" << (valid ? Words(angleWitness) : "null") << '}';
  }
  ReportM1Timing(VK_SUCCESS,true);
  return true;
}

std::future<M1ColdResult> Boot::CaptureM1Quantum(m1::SourceRootOwner &root, const grs::AcceptedRoot &accepted,
                            const grs::Words256 &sourceCompletion, uint32_t stateWords,
                            const std::array<uint32_t,32> &sourceCommand,std::optional<M1HotCapture::Page> ticket,
                            const std::wstring &observationPrefix) {
  const auto noCapture=[] { return std::async(std::launch::deferred,[] { return M1ColdResult{}; }); };
  if (D1Ended()) return noCapture();
  Checkpoint("M1_QUANTUM_COLD_CAPTURE");
  const uint32_t eventCount=accepted.completion[53];
  const auto &stored=accepted.completion;
  const auto &replayPrefix=observationPrefix.empty() ? attempt.m1ReplayPrefix : observationPrefix;
  const bool replay=!replayPrefix.empty();
  const bool rootOnly=attempt.m1RootCapture || replay;
  const bool sourceRoot=stored[27]==1;
  const auto nonce=(uint64_t(accepted.nonce.hi)<<32)|accepted.nonce.lo;
  const auto generation=(uint64_t(accepted.publicationGeneration.hi)<<32)|accepted.publicationGeneration.lo;
  const std::array<uint64_t,5> key{m1Run,attempt.geometry,stored[27],nonce,generation};
  Require(!inFlight && m1AcceptedFence.object && m1AcceptedFence.exit,"M1_COLD_ACCEPT_EDGE");
  const bool d1Replay=!attempt.m1ReplayPrefix.empty() && observationPrefix.empty();
  const size_t bank=accepted.bank ? kM1Candidate : kM1Base;
  const size_t publication=sourceRoot ? kM1Pub : accepted.publication ? kM1QPubAlt : kM1QPub;
  const size_t events=sourceRoot ? kM1Events : kM1QEvents;
  const M1ColdAnchor anchor{m1Run,attempt.geometry,nonce,generation,m1AcceptedFence.nonce,
                            accepted.bank,accepted.publication};
  Require(root.Bank()==accepted.bank && M1ColdBackingMatches(stored,anchor,replay,buffers[events].bytes),
          "M1_COLD_ROOT_BACKING");
  if (d1Replay && m1LastReplay==key) return noCapture();
  const std::array<uint32_t, 8> sizes{1024,stateWords*4,eventCount*256,uint32_t(buffers[publication].bytes),
                                    rootOnly ? 0u : stateWords*4,rootOnly ? 0u : sourceCompletion[53]*256,
                                    rootOnly ? 0u : 1024u,rootOnly ? 0u : 128u};
  VkDeviceSize payload=0;
  for (auto bytes : sizes) payload+=bytes;
  Require((rootOnly || (sourceCompletion[53]<=sourceCompletion[54] && bank==kM1Base &&
                       publication==kM1QPub)) &&
          payload<=16*1024*1024,"M1_COLD_RANGE");
  const auto gpuBytes=payload-sizes[6]-sizes[7];
  if (replay && payload>M1HotCapture::kColdLimit-m1ReplayBytes) {
    fields << ",\"m1ReplayIncomplete\":\"RAW_CREDIT_EXHAUSTED\",\"m1ReplayBytes\":" << m1ReplayBytes;
    return std::async(std::launch::deferred,[] { return M1ColdResult{}; });
  }
  if (!buffers[kM1ColdStaging].handle)
    Allocate(kM1ColdStaging,replay ? M1HotCapture::kColdLimit : gpuBytes,
             VK_BUFFER_USAGE_TRANSFER_DST_BIT,true);
  const auto &staging=buffers[kM1ColdStaging];
  const size_t slots[]={kM1Tail,bank,events,publication,kM1Candidate,kM1Events};
  Require(gpuBytes<=staging.bytes,"M1_COLD_STAGING_RANGE");
  for (size_t i=0;i<std::size(slots);++i)
    Require(sizes[i]<=buffers[slots[i]].bytes,"M1_COLD_BUFFER_RANGE");
  PublicationLease lease;
  Require(root.Acquire(accepted.publication,accepted.publicationGeneration,lease)==ControlCode::OK,
          "M1_COLD_LEASE");
  struct Pin {
    m1::SourceRootOwner &owner;
    const grs::AcceptedRoot &root;
    const bool &inFlight;
    bool held=true;
    ~Pin() {
      if (held && !inFlight && owner.State()!=FlightState::StateLost)
        owner.Release(root.publication,root.publicationGeneration,true);
    }
  } pin{root,accepted,inFlight};
  Require(lease.leased && lease.bank==accepted.bank &&
          Compare64(lease.nonce,accepted.nonce)==0 &&
          Compare64(lease.generation,accepted.publicationGeneration)==0 &&
          Compare64(lease.identity.run,{uint32_t(m1Run),uint32_t(m1Run>>32)})==0 &&
          Compare64(lease.identity.geometry,{uint32_t(attempt.geometry),uint32_t(attempt.geometry>>32)})==0,
          "M1_COLD_LEASE_BACKING");
  std::ostringstream proof;
  proof << "\"run\":\"" << m1Run << "\",\"geometry\":\"" << attempt.geometry
        << "\",\"rootKind\":" << Quote(sourceRoot ? "SOURCE" : "QUANTUM")
        << ",\"nonce\":\"" << nonce << "\",\"publicationGeneration\":\"" << generation
        << "\",\"baseBank\":" << stored[32] << ",\"finalBank\":" << stored[33]
        << ",\"publicationSlot\":" << stored[35] << ",\"eventArena\":" << stored[51]
        << ",\"eventStart\":" << stored[52] << ",\"eventEnd\":" << eventCount
        << ",\"eventCapacity\":" << stored[54] << ",\"businessFence\":\"" << m1AcceptedFence.object
        << "\",\"businessFenceEnterQpc\":\"" << m1AcceptedFence.enter
        << "\",\"businessFenceExitQpc\":\"" << m1AcceptedFence.exit
        << "\",\"businessFenceVkResult\":0,\"ranges\":[";
  for (size_t i=0;i<std::size(slots);++i) {
    const auto &backing=buffers[slots[i]];
    if (i) proof << ',';
    proof << "{\"slot\":" << slots[i] << ",\"buffer\":\"" << reinterpret_cast<uintptr_t>(backing.handle)
          << "\",\"allocation\":\"" << reinterpret_cast<uintptr_t>(backing.memory)
          << "\",\"generation\":" << backing.generation << ",\"offset\":0,\"bytes\":" << sizes[i] << '}';
  }
  proof << ']';
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
  uint64_t submitEnter=0,submitExit=0;
  const auto submitCopy=[&] {
    submitEnter=pc::QpcTicks();
    const auto result=vk.QueueSubmit(queue,1,&submit,fence);
    submitExit=pc::QpcTicks();
    return result;
  };
  VkResult submitted=VK_NOT_READY;
  if (attempt.m1D1) {
    const bool authorized=attempt.acceptPointer(0,[&] {
      if (!D1Ended()) submitted=submitCopy();
    });
    if (!authorized || submitted==VK_NOT_READY) return noCapture();
    if (submitted==VK_ERROR_DEVICE_LOST) {
      d1FenceResult=submitted;
      root.ContextLost();
      return noCapture();
    }
  } else {
    submitted=submitCopy();
    if (submitted==VK_ERROR_DEVICE_LOST) root.ContextLost();
  }
  Check(submitted,"vkQueueSubmit.M1_COLD");
  inFlight=true;
  if (d1Replay) m1LastReplay=key;
  const auto waitEnter=pc::QpcTicks();
  const auto wait=vk.WaitForFences(device,1,&fence,VK_TRUE,m1::ProgressClockV2::JobNs);
  const auto waitExit=pc::QpcTicks();
  if (wait==VK_SUCCESS || wait==VK_ERROR_DEVICE_LOST) inFlight=false;
  if (wait==VK_ERROR_DEVICE_LOST) root.ContextLost();
  if (attempt.m1D1) {
    d1FenceObject=reinterpret_cast<uintptr_t>(fence);
    d1FenceNonce=(uint64_t(accepted.nonce.hi)<<32)|accepted.nonce.lo;
    d1OwnerPoll.reset();
    d1FenceResult=wait;
    d1FenceKind=wait==VK_SUCCESS ? "COLD_COPY_SIGNALED" : "COLD_COPY_SUBMITTED";
    d1FenceEnter=waitEnter;
    d1FenceExit=waitExit;
    if (inFlight)
      d1CaptureLease.emplace(accepted.publication,accepted.publicationGeneration);
  }
  if (replay) {
    const auto copyFence=std::to_string(reinterpret_cast<uintptr_t>(fence));
    const auto evidence=proof.str()+",\"copyFence\":\""+copyFence+"\"";
    publish("{\"stage\":\"COLD_COPY_SUBMITTED\","+evidence+
            ",\"submitEnterQpc\":\""+std::to_string(submitEnter)+"\",\"submitExitQpc\":\""+
            std::to_string(submitExit)+"\",\"submitVkResult\":0}");
    publish("{\"stage\":\"COLD_COPY_WAIT\","+evidence+
            ",\"waitEnterQpc\":\""+std::to_string(waitEnter)+"\",\"waitExitQpc\":\""+
            std::to_string(waitExit)+"\",\"waitVkResult\":"+std::to_string(wait)+"}");
  }
  if (attempt.m1D1 && wait!=VK_SUCCESS) return noCapture();
  Check(wait,"vkWaitForFences.M1_COLD");
  std::vector<uint8_t> raw(size_t(payload),0);
  void *mapped=nullptr;
  const auto map=vk.MapMemory(device,staging.memory,0,VK_WHOLE_SIZE,0,&mapped);
  VkResult invalidation=VK_NOT_READY;
  if (map==VK_SUCCESS) {
    invalidation=VK_SUCCESS;
    if (!(staging.flags&VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
      VkMappedMemoryRange range{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,nullptr,staging.memory,0,VK_WHOLE_SIZE};
      invalidation=vk.InvalidateMappedMemoryRanges(device,1,&range);
    }
    if (invalidation==VK_SUCCESS) std::memcpy(raw.data(),mapped,size_t(gpuBytes));
    vk.UnmapMemory(device,staging.memory);
  }
  if (map==VK_ERROR_DEVICE_LOST || invalidation==VK_ERROR_DEVICE_LOST) root.ContextLost();
  if (replay) publish("{\"stage\":\"COLD_COPY_READBACK\","+proof.str()+
                      ",\"mapVkResult\":"+std::to_string(map)+",\"invalidateVkResult\":"+std::to_string(invalidation)+"}");
  Check(map,"vkMapMemory.M1_COLD");
  Check(invalidation,"vkInvalidateMappedMemoryRanges.M1_COLD");
  Require(root.Release(accepted.publication,accepted.publicationGeneration,true)==ControlCode::OK,
          "M1_COLD_RELEASE");
  pin.held=false;
  Require(std::memcmp(raw.data(),stored.data(),1024)==0,"M1_COLD_COMPLETION_CHANGED");
  if (!rootOnly) {
    std::memcpy(raw.data()+gpuBytes,sourceCompletion.data(),sizes[6]);
    std::memcpy(raw.data()+gpuBytes+sizes[6],sourceCommand.data(),sizes[7]);
  }
  fields << (sourceRoot ? ",\"m1SourceColdBacking\":{" : ",\"m1ColdBacking\":{")
         << "\"format\":" << Quote(rootOnly ? "M1PROOT1" : "M1PCOLD1")
         << ",\"bankSlot\":" << accepted.bank << ",\"publicationSlot\":" << accepted.publication
         << ",\"eventArena\":" << stored[51] << ",\"eventEnd\":" << eventCount
         << ",\"bankBuffer\":\"" << reinterpret_cast<uintptr_t>(buffers[bank].handle)
         << "\",\"eventBuffer\":\"" << reinterpret_cast<uintptr_t>(buffers[events].handle)
         << "\",\"publicationBuffer\":\"" << reinterpret_cast<uintptr_t>(buffers[publication].handle)
         << "\",\"fence\":" << int(wait) << ",\"invalidate\":" << int(invalidation);
  if (!observationPrefix.empty())
    fields << ",\"copyFence\":\"" << reinterpret_cast<uintptr_t>(fence) << "\",\"waitEnterQpc\":\""
           << waitEnter << "\",\"waitExitQpc\":\"" << waitExit << '"';
  fields << '}';
  // The worker owns only sealed CPU bytes and the destination path.
  if (replay) m1ReplayBytes+=size_t(payload);
  const auto outputPath=replay ? replayPrefix+L"-"+std::to_wstring(nonce)+L".m1root" : attempt.m1ColdFile;
  return std::async(std::launch::async,[path=outputPath,sizes,raw=std::move(raw),
                                       capture=attempt.hotCapture,ticket,rootOnly]() mutable {
    try {
    const auto digest=Hash(raw.data(),raw.size());
    const auto temp=path+L".pending-"+std::to_wstring(GetCurrentProcessId());
    const HANDLE out=CreateFileW(temp.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
    Require(out!=INVALID_HANDLE_VALUE,"M1_COLD_CREATE");
    const auto write=[&](const void *src,size_t bytes) {
      DWORD n=0;
      return WriteFile(out,src,DWORD(bytes),&n,nullptr) ? n : 0;
    };
    const bool wrote=WriteM1Cold(sizes,digest,raw.data(),raw.size(),write,rootOnly) && FlushFileBuffers(out);
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
