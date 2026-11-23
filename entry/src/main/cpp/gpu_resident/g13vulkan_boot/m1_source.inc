constexpr char kM1CandidateSHA[] = "b8f0e3275fc8de0569a6c946097e8f47263d52d24a1a6e6f3573ebe960f8c85c";
constexpr char kM1CommitSHA[] = "caf0614c3b2b56e3ad03d3bb4394347bbea9f2ec5692fce5ee1ddf1fdfd906ce";

void Boot::SubmitM1Root(const VkSubmitInfo &submit,uint64_t hold,uint64_t deadline,uint64_t nonce,M1WaitLog &log) {
  if (!attempt.m1D1) {
    log.submit=vk.QueueSubmit(queue,1,&submit,fence);
    return;
  }
  if (!attempt.acceptPointer(hold,[&] {
        if (D1Ended()) m1Reject="D1_END_INTAKE";
        else if (pc::TimeNs()>=deadline) m1Reject="ROOT_DEADLINE";
        else {
          log.submit=vk.QueueSubmit(queue,1,&submit,fence);
          if (log.submit==VK_SUCCESS) {
            d1FenceObject=reinterpret_cast<uintptr_t>(fence);
            d1FenceNonce=nonce;
            d1OwnerPoll.reset();
            d1FenceResult=VK_NOT_READY;
            d1FenceEnter=d1FenceExit=0;
          }
        }
      }))
    m1Reject=hold && attempt.current() ? "SOURCE_HOLD_REVOKED" : "WINDOW_REVOKED";
}

const char *Boot::AwaitM1Fence(uint64_t deadline,const std::function<const char *()> &guard,
                              M1WaitLog &log) {
  for (;;) {
    const auto now=pc::TimeNs();
    if (now>=deadline) return "ROOT_DEADLINE";
    if (const auto why=guard()) return why;
    M1WaitLog::Wait sample;
    const auto before=pc::TimeNs();
    if (before>=deadline) return "ROOT_DEADLINE";
    sample.timeout=std::min<uint64_t>(deadline-before,2000000);
    sample.enter=pc::QpcTicks();
    sample.result=vk.WaitForFences(device,1,&fence,VK_TRUE,sample.timeout);
    sample.exit=pc::QpcTicks();
    log.fenceNs=pc::QpcNs(sample.exit,pc::QpcFrequency());
    log.last=sample.result;
    if (attempt.m1D1) {
      d1FenceResult=sample.result;
      d1FenceEnter=sample.enter;
      d1FenceExit=sample.exit;
    }
    log.lastWait=sample;
    if (log.calls<log.rows.size()) log.rows[log.calls]=sample;
    ++log.calls;
    if (log.last==VK_TIMEOUT) continue;
    if (log.last==VK_SUCCESS || log.last==VK_ERROR_DEVICE_LOST) inFlight=false;
    if (log.last==VK_ERROR_DEVICE_LOST) return "DEVICE_LOST";
    if (log.last!=VK_SUCCESS) return "FENCE_VK_ERROR";
    if (log.fenceNs>deadline) return "LATE_FENCE_SIGNAL";
    return guard();
  }
}

void Boot::RetireM1Root(m1::SourceRootOwner &root,M1WaitLog &log) {
  root.Stop();
  if (log.submit!=VK_SUCCESS) {
    if (log.submit==VK_ERROR_DEVICE_LOST) root.ContextLost();
    else {
      grs::AcceptedRoot discarded;
      root.Poll(FenceState::Signaled,nullptr,0,0,discarded); // Never submitted.
    }
    return;
  }
  log.retire=log.last;
  if (inFlight) {
    log.retireEnter=pc::QpcTicks();
    const auto end=attempt.m1D1 ? attempt.d1Intake->Snapshot() : M1D1Intake::End{};
    const auto now=pc::TimeNs();
    const auto until=end.ns+M1D1Intake::kDrainNs;
    const auto timeout=end.ns ? std::min(m1::ProgressClockV2::JobNs,until>now ? until-now : 0) :
                                m1::ProgressClockV2::JobNs;
    log.retire=vk.WaitForFences(device,1,&fence,VK_TRUE,timeout);
    log.retireExit=pc::QpcTicks();
    if (log.retire==VK_SUCCESS || log.retire==VK_ERROR_DEVICE_LOST) inFlight=false;
    if (attempt.m1D1) {
      d1FenceResult=log.retire;
      d1FenceEnter=log.retireEnter;
      d1FenceExit=log.retireExit;
    }
  }
  if (log.retire==VK_ERROR_DEVICE_LOST) root.ContextLost();
  if (log.retire==VK_SUCCESS) {
    grs::AcceptedRoot discarded;
    const auto polled=root.Poll(FenceState::Signaled,nullptr,0,0,discarded);
    if (attempt.m1D1) d1OwnerPoll=polled;
  }
}

void Boot::M1RootTrace(const char *kind,uint64_t nonce,const m1::ProgressClockV2 &clock,
                       const m1::PlanGrantV2 &grant,const grs::AcceptedRoot &accepted,
                       const M1WaitLog &log) {
  fields << ",\"m1" << kind << "Authority\":{\"reason\":" << (m1Reject ? Quote(m1Reject) : "null")
         << ",\"nonce\":\"" << nonce << "\",\"windowCurrent\":" << (attempt.current() ? "true" : "false")
         << ",\"run\":\"" << m1Run << "\",\"geometry\":\"" << attempt.geometry
         << "\",\"session\":\"" << grant.session << "\",\"clockSession\":\"" << clock.session
         << "\",\"plan\":\"" << grant.id << "\",\"activePlan\":\"" << clock.active.id
         << "\",\"grantNs\":\"" << grant.grantedNs << "\",\"deadlineNs\":\""
         << (grant.id ? grant.grantedNs+m1::ProgressClockV2::JobNs : 0)
         << "\",\"clockOpen\":" << (clock.open ? "true" : "false")
         << ",\"clockReady\":" << (clock.ready ? "true" : "false") << ",\"clockFault\":" << Quote(clock.fault)
         << ",\"producerSeq\":\"" << m1Latest.seq << "\",\"producerTick\":\"" << m1Latest.callbackTicks
         << "\",\"producerReceiptTick\":\"" << m1Latest.receiptTicks
         << "\",\"consumedSeq\":\"" << m1Consumed.seq << "\",\"callbackTick\":\"" << m1Consumed.callbackTicks
         << "\",\"receiptTick\":\"" << m1Consumed.receiptTicks << "\",\"uiNs\":\"" << clock.uiNs
         << "\",\"receiptNs\":\"" << clock.receiveNs << "\",\"maxUiGapNs\":\"" << clock.maxUiGap
         << "\",\"maxNativeGapNs\":\"" << clock.maxNativeGap << "\",\"pending\":" << clock.pending
         << ",\"active\":" << clock.activeRemaining << ",\"carry\":" << clock.carry
         << ",\"acceptedRoot\":\"" << ((uint64_t(accepted.nonce.hi)<<32)|accepted.nonce.lo)
         << "\",\"submitVkResult\":" << int(log.submit) << ",\"retireVkResult\":" << int(log.retire)
         << ",\"retireEnterQpc\":\"" << log.retireEnter << "\",\"retireExitQpc\":\"" << log.retireExit << "\""
         << ",\"fencePending\":" << (inFlight ? "true" : "false") << ",\"waitCount\":" << log.calls
         << ",\"fenceObject\":\"" << (grant.id ? reinterpret_cast<uintptr_t>(fence) : 0) << "\""
         << ",\"waitsTruncated\":" << (log.calls>log.rows.size() ? "true" : "false");
  const auto writeWait=[&](const M1WaitLog::Wait &w) {
    fields << "[\"" << w.enter << "\",\"" << w.exit << "\"," << w.timeout << ',' << int(w.result) << ']';
  };
  fields << ",\"lastWait\":";
  if (log.calls) writeWait(log.lastWait);
  else fields << "null";
  fields << ",\"waits\":[";
  const auto retained=(attempt.m1Trace || m1Reject) ? std::min(log.calls,log.rows.size()) : 0;
  for (size_t i=0;i<retained;++i) {
    if (i) fields << ',';
    writeWait(log.rows[i]);
  }
  fields << "]}";
}

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
  const bool fullM1=interactive && attempt.m1DualSource;
  const uint32_t emitQ=fullM1 ? 16u*1048576u : 1048576u;
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
         << ",\"inputKind\":" << Quote(interactive ? "WIN32_POINTER_HOLD" : "PC_DIAGNOSTIC_DRY_SOURCE")
         << ",\"m1InputProfile\":" << Quote(fullM1 ? "FULL_M1_V2" : "DRY_M1_V2")
         << ",\"m1EmitQ\":" << emitQ;
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
  Allocate(kM1Pub, pubWords*4, storage | (attempt.m1ReplayPrefix.empty() ? 0 : copyOut));
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
  using SourceProfile=m1::SourceRootOwner::Profile;
  Require(root.Initialize(identity,K,Kw,fullM1 ? SourceProfile::FullM1 : SourceProfile::SourceCore)
          ==ControlCode::OK,"M1_SOURCE_OWNER_INIT");

  m1::ProgressClockV2 clock;
  const auto wide=[](grs::U64 value) { return (uint64_t(value.hi)<<32)|value.lo; };
  m1Run=wide(identity.run);
  Require(attempt.m1Pulses,"M1_PULSE_QUEUE_MISSING");
  M1PulseScope pulseScope{wide(identity.run),attempt.geometry,0};
  uint64_t pulseStartTicks=0;
  const auto beginClock=[&](bool ready) {
    Require(clock.BeginSession(),"M1_CLOCK_SESSION_EXHAUSTED");
    clock.ready=ready;
    m1Latest=m1Consumed={};
    pulseScope.session=clock.session;
    pulseStartTicks=pc::QpcTicks();
    attempt.m1Pulses->Begin(pulseScope,pulseStartTicks);
  };
  const M1PulseQueue::Observer pulseObserver=attempt.m1D1 ?
        M1PulseQueue::Observer([&](const M1Pulse &p,const char *state) {
          std::ostringstream row;
          row << "{\"stage\":\"M1_D1_PULSE_OWNER\",\"actionId\":" << p.action
              << ",\"session\":\"" << p.session << "\",\"seq\":\"" << p.seq
              << "\",\"callbackQpc\":\"" << p.callbackTicks << "\",\"nativeReceiptQpc\":\"" << p.receiptTicks
              << "\",\"ownerQpc\":\"" << pc::QpcTicks() << "\",\"state\":" << Quote(state)
              << ",\"generated\":" << clock.generated << ",\"pending\":" << clock.pending
              << ",\"carry\":" << clock.carry << ",\"overload\":" << clock.overload << '}';
          attempt.d1PulseTrace(row.str());
        }) : M1PulseQueue::Observer{};
  const auto refresh=[&] {
    if (!clock.open || !attempt.m1Pulses->Consume(pulseScope,clock,m1Latest,m1Consumed,pulseObserver)) return false;
    return clock.Fresh(pc::TimeNs());
  };
  const auto primeClock=[&] {
    const auto started=pc::TimeNs();
    do {
      if (refresh()) return true;
      if (!clock.open || !attempt.current()) return false;
      Sleep(1);
    } while (pc::TimeNs()-started<=m1::ProgressClockV2::GapNs);
    clock.Revoke("CLOCK_GAP");
    return false;
  };
  beginClock(false);
  using IngressCode=M1Ingress::Code;
  if (interactive) {
    Require(attempt.ingress && attempt.sourceScope,"M1_INGRESS_MISSING");
    attempt.sourceScope({wide(identity.run),wide(identity.owner),wide(identity.context),
                         wide(identity.surface),wide(identity.geometry),wide(identity.config),0,attempt.ticket});
  }
  const auto finishOffer=[&](const SourceOffer &offer,IngressCode code,uint64_t rootNonce=0,
                             const char *reason=nullptr,uint64_t checked=0) {
    if (offer.seq) attempt.ingress->Finish(offer.seq,{code,rootNonce},reason,checked);
  };
  const auto takeFresh=[&] {
    if (!attempt.m1D1) {
      auto offer=attempt.ingress->TakeLatest(pc::TimeNs(),pulseStartTicks,clock.session);
      if (offer.seq && (!refresh() || offer.current.seq>clock.sequence)) {
        finishOffer(offer,IngressCode::Rejected,0,"PULSE_NOT_ACCEPTED");
        return SourceOffer{};
      }
      return offer;
    }
    for (unsigned i=0;i<4;++i) {
      const auto offer=attempt.ingress->Take();
      const auto now=pc::TimeNs();
      if (!offer.seq || (offer.pulseTicks>pulseStartTicks && now>=offer.pulseNs &&
                         now-offer.pulseNs<=m1::ProgressClockV2::GapNs))
        return offer;
      finishOffer(offer,IngressCode::Expired,0,offer.pulseTicks<=pulseStartTicks ? "OLD_BASELINE" :
                   now<offer.pulseNs ? "CLOCK_REVERSED" : "STALE_ENDPOINT",now);
    }
    return SourceOffer{};
  };
  const auto setupFields=fields.str();
  PointerSample inputSample{};
  SourceOffer firstOffer;
  uint64_t pulse = 0;
  uint64_t delta = 0;
  const auto awaitSource=[&] {
    inputSample={};
    firstOffer={};
    stage="M1_POINTER_READY";
    const bool opened=attempt.pointerReady(Receipt("RUNNING",""));
    stage="M1_POINTER_WAIT";
    if (!opened) return false;
    while (attempt.current() && !D1Ended()) {
      const bool fresh=refresh();
      if (!clock.open) {
        attempt.disarmM1Input();
        fields << ",\"m1IdlePulseFault\":" << Quote(clock.fault);
        publish(Receipt("M1_WAIT_NEW_INPUT",""));
        beginClock(false);
        if (!attempt.pointerReady(Receipt("RUNNING",""))) break;
        continue;
      }
      if (!fresh) { Sleep(1); continue; }
      const auto offer = takeFresh();
      if (!offer.seq) { Sleep(1); continue; }
      inputSample=offer.sample;
      firstOffer=offer;
      pulse=offer.pulseNs;
      delta=offer.deltaNs;
      break;
    }
    if (!inputSample.hold) return false;
    return true;
  };
  const auto retireSubmitted=[&](const M1D1Intake::End &end) {
    if (!inFlight) return;
    if (!d1CaptureLease) root.Stop();
    const auto now=pc::TimeNs();
    const auto deadline=end.ns+M1D1Intake::kDrainNs;
    d1FenceEnter=pc::QpcTicks();
    d1DrainProbe=vk.WaitForFences(device,1,&fence,VK_TRUE,0);
    d1ProbeQpc=pc::QpcTicks();
    d1FenceResult=d1DrainProbe;
    if (d1DrainProbe==VK_TIMEOUT && end.ns && deadline>now) {
      const auto before=pc::TimeNs();
      if (deadline>before) d1FenceResult=vk.WaitForFences(device,1,&fence,VK_TRUE,deadline-before);
    }
    d1FenceExit=pc::QpcTicks();
    if (d1FenceResult==VK_SUCCESS) {
      inFlight=false;
      grs::AcceptedRoot discarded;
      if (!d1CaptureLease) d1OwnerPoll=root.Poll(FenceState::Signaled,nullptr,0,0,discarded);
      else if (root.Release(d1CaptureLease->first,d1CaptureLease->second,true)==ControlCode::OK) d1CaptureLease.reset();
    } else if (d1FenceResult==VK_ERROR_DEVICE_LOST) {
      inFlight=false;
      root.ContextLost();
      d1CaptureLease.reset();
    }
  };
  const auto reportDrain=[&](const M1D1Intake::End &end,const pc::Frame *frame) {
    attempt.disarmM1Input();
    attempt.m1Pulses->RevokeQueued(pulseScope,pulseObserver);
    const auto offers=attempt.ingress->PendingCount();
    const auto captures=attempt.hotCapture && attempt.hotCapture->HasLiveRequest();
    const auto pending=clock.pending;
    const auto active=clock.activeRemaining;
    const auto carry=clock.carry;
    const auto revoked=clock.revoked;
    const auto cleared=clock.clearedCarry;
    clock.Revoke("D1_END_INTAKE");
    const auto finished=pc::TimeNs();
    const bool deviceLost=root.State()==FlightState::StateLost || d1FenceResult==VK_ERROR_DEVICE_LOST ||
                          (frame && frame->retire==VK_ERROR_DEVICE_LOST);
    const bool within=end.ns && finished>=end.ns && finished-end.ns<=M1D1Intake::kDrainNs;
    const bool retired=within && !deviceLost && (!frame || frame->retire==VK_SUCCESS) &&
                       !offers && !captures && !inFlight && root.State()==FlightState::Idle &&
                       !root.HasLeases() && !d1CaptureLease && clock.Balanced() &&
                       clock.revoked-revoked==uint64_t(pending)+active && clock.clearedCarry-cleared==carry;
    const char *state=deviceLost ? "DEVICE_LOST" : !end.ns ? "END_INTAKE_MISSING" :
                      (inFlight && d1FenceResult!=VK_TIMEOUT) ? "FENCE_VK_ERROR" :
                      (inFlight || !within) ? "DRAIN_TIMEOUT" : retired ? "RETIRED" : "DRAIN_INCOMPLETE";
    std::ostringstream row;
    row << "\"state\":" << Quote(state) << ",\"run\":\"" << m1Run
        << "\",\"owner\":\"" << wide(identity.owner) << "\",\"generation\":\"" << attempt.geometry
        << "\",\"device\":\"" << reinterpret_cast<uintptr_t>(device) << "\",\"endNs\":\"" << end.ns
        << "\",\"drainNs\":\"" << finished << "\",\"lastPulseSeq\":\"" << end.pulse
        << "\",\"lastOffer\":\"" << end.offer << "\",\"offers\":" << offers
        << ",\"inFlight\":" << int(inFlight) << ",\"leases\":" << int(root.HasLeases())
        << ",\"ownerState\":" << int(root.State()) << ",\"ownerFenceVkResult\":" << int(d1FenceResult)
        << ",\"ownerFence\":\"" << d1FenceObject << "\",\"ownerFenceNonce\":\"" << d1FenceNonce
        << "\",\"ownerProbeVkResult\":" << int(d1DrainProbe) << ",\"ownerProbeQpc\":\"" << d1ProbeQpc
        << "\",\"ownerTombstonePoll\":" << (d1OwnerPoll ? std::to_string(int(*d1OwnerPoll)) : "null")
        << ",\"ownerFenceEnterQpc\":\"" << d1FenceEnter << "\",\"ownerFenceExitQpc\":\"" << d1FenceExit
        << "\",\"wsiUsed\":" << (frame ? "true" : "false")
        << ",\"retireVkResult\":" << int(frame ? frame->retire : VK_NOT_READY)
        << ",\"pending\":" << clock.pending << ",\"permits\":" << clock.activeRemaining
        << ",\"captureRequests\":" << captures << ",\"writer\":0,\"reader\":0"
        << ",\"writersCompleted\":" << d1WritersCompleted
        << ",\"pin\":" << int(d1CaptureLease.has_value())
        << ",\"credit\":{\"pendingBefore\":" << pending << ",\"activeBefore\":" << active
        << ",\"carryBefore\":" << carry << ",\"revokedBefore\":" << revoked
        << ",\"clearedCarryBefore\":" << cleared << ",\"generated\":" << clock.generated
        << ",\"accepted\":" << clock.accepted << ",\"overload\":" << clock.overload
        << ",\"revoked\":" << clock.revoked << ",\"carry\":" << clock.carry
        << ",\"clearedCarry\":" << clock.clearedCarry << ",\"balanced\":"
        << (clock.Balanced() ? "true" : "false") << '}';
    attempt.d1Report(row.str(),retired);
  };
  const auto finishWithoutWsi=[&](const char *why) {
    if (attempt.m1D1) {
      const auto end=attempt.d1Intake->Await();
      retireSubmitted(end);
      reportDrain(end,nullptr);
    }
    return Receipt("FAILED",why);
  };
  if (interactive && !awaitSource()) {
    if (attempt.m1D1) return finishWithoutWsi("ATTEMPT_REVOKED");
    Require(false,"ATTEMPT_REVOKED");
  }
  if (!primeClock()) {
    finishOffer(firstOffer,IngressCode::Rejected,0,"M1_SOURCE_UI_PULSE_ABSENT",pc::TimeNs());
    if (attempt.m1D1) return finishWithoutWsi("M1_SOURCE_UI_PULSE_ABSENT");
    Require(false,"M1_SOURCE_UI_PULSE_ABSENT");
  }
  if (!interactive) pulse=clock.uiNs;
  clock.ready=clock.baseline=true;
  m1::PlanGrantV2 permit;
  if (!clock.Claim(pc::TimeNs(),0,interactive ? inputSample.hold : 1,true,permit)) {
    finishOffer(firstOffer,IngressCode::Rejected,0,"M1_SOURCE_PERMIT",pc::TimeNs());
    if (attempt.m1D1) return finishWithoutWsi("M1_SOURCE_PERMIT");
    Require(false,"M1_SOURCE_PERMIT");
  }
  uint64_t frameSerial=1;
  uint32_t sourceSerial=0;
  std::array<uint32_t,32> cmd{};
  Words256 tail{};
  AcceptedRoot accepted;
  const auto &staging=buffers[kStaging];
  const auto runSource=[&](const PointerSample &inputSample,uint64_t pulse,uint64_t delta,
                           const SourceOffer &offer) {
    namespace sourceWire=grs::m1::sourceCommand32;
    Require(sourceSerial!=UINT32_MAX,"M1_SOURCE_SEQUENCE_EXHAUSTED");
    Require(!inFlight,"M1_SOURCE_RETIRE_PENDING");
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
    cmd[sourceWire::kind] = fullM1 ? inputSample.material : 0;
    Require(cmd[sourceWire::kind]<=1,"M1_SOURCE_MATERIAL_INVALID");
    cmd[4] = identity.run.lo; cmd[5] = identity.run.hi;
    cmd[8] = 1;
    cmd[sourceWire::sourceId] = 1+cmd[sourceWire::kind];
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
    if (offer.seq) {
      fields << ",\"m1Hold\":\"" << inputSample.hold << "\",\"m1PointerSeq\":\"" << inputSample.sequence
             << "\",\"m1PointerNs\":\"" << inputSample.timeNs << "\",\"m1SourceDeltaNs\":\"" << delta << "\"";
      if (offer.current.seq)
        fields << ",\"m1ReservationPulses\":" << Words(std::array<uint64_t,9>{
            offer.current.session,offer.previous.seq,offer.current.seq,
            offer.previous.callbackTicks,offer.current.callbackTicks,offer.previous.receiptTicks,
            offer.current.receiptTicks,offer.previous.uiNs,offer.current.uiNs});
      const auto &s=offer.scope;
      std::array<uint64_t,25> original{s.run,s.owner,s.context,s.surface,s.geometry,s.config,s.permission,
          s.pageSession,offer.seq,offer.pulseNs,offer.pulseTicks,offer.sampleTicks,offer.frequency,offer.origin,
          inputSample.hold,inputSample.sequence,inputSample.timeNs,uint32_t(inputSample.clientX),
          uint32_t(inputSample.clientY),0,0,inputSample.material,offer.action,offer.deltaNs,wide(request.nonce)};
      std::memcpy(original.data()+19,source,2*sizeof(double));
      fields << ",\"m1OfferAction\":" << offer.action << ",\"m1ProducerWords\":" << Words(original)
             << ",\"m1ProducerSHA\":" << Quote(Hex(Hash(original.data(),sizeof(original)-8).data()))
             << ",\"m1SourceWords\":" << Words(cmd)
             << ",\"m1SourceWordsSHA\":" << Quote(Hex(Hash(cmd.data(),sizeof(cmd)).data()));
    }
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
        K,Kw,16*K,kColumn,headerAt,stateWords,pass,emitQ,request.nonce.lo,request.nonce.hi,
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
    stage="M1_SOURCE_SUBMIT";
    const auto sourceGuard = [&]() -> const char * {
      if (!attempt.current()) return "WINDOW_REVOKED";
      if (!refresh()) return clock.fault;
      if (!clock.Identity(permit.id,permit.session)) return "PLAN_SESSION_MISMATCH";
      if (!interactive) return nullptr;
      const auto held=attempt.pointer();
      return held.hold==inputSample.hold && held.material==inputSample.material ? nullptr : "SOURCE_HOLD_REVOKED";
    };
    M1WaitLog log;
    const auto reject=[&] {
      if (!clock.open || std::strcmp(m1Reject,"SOURCE_HOLD_REVOKED")!=0) attempt.disarmM1Input();
      RetireM1Root(root,log);
      if (root.State()!=FlightState::Idle) attempt.disarmM1Input();
      fields << ",\"m1SourceReject\":" << Quote(m1Reject);
      M1RootTrace("Source",wide(request.nonce),clock,permit,accepted,log);
      return false;
    };
    m1Reject=pc::TimeNs()>=deadline ? "ROOT_DEADLINE" : sourceGuard();
    if (m1Reject) return reject();
    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &command;
    SubmitM1Root(submit,inputSample.hold,deadline,wide(request.nonce),log);
    if (m1Reject) return reject();
    if (log.submit!=VK_SUCCESS) {
      m1Reject=log.submit==VK_ERROR_DEVICE_LOST ? "DEVICE_LOST" : "SUBMIT_VK_ERROR";
      return reject();
    }
    inFlight = true;
    M1BusyFence busy{attempt.m1ColdOnBusy ? nullptr : attempt.hotCapture,
                     M1HotCapture::Phase::SourceFence,wide(request.nonce)};
    fields << ",\"m1QueueVkResult\":0,\"m1Nonce\":\"" << ((uint64_t(request.nonce.hi)<<32) | request.nonce.lo)
           << "\",\"m1DeadlineNs\":\"" << deadline << "\",\"m1PulseNs\":\"" << pulse << "\"";
    m1Reject=AwaitM1Fence(deadline,sourceGuard,log);
    if (attempt.hotCapture) attempt.hotCapture->StopBusy();
    fields << ",\"m1FenceVkResult\":" << int(log.last) << ",\"m1FenceNs\":\"" << log.fenceNs
           << "\",\"m1PulseCount\":" << clock.pulses << ",\"m1PulseFault\":"
           << (clock.open ? "null" : Quote(clock.fault));
    if (m1Reject) return reject();
    stage="M1_SOURCE_FENCED_READBACK";
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
      if (pc::TimeNs()>deadline) m1Reject="ACCEPT_DEADLINE";
      else if (!refresh()) m1Reject=clock.fault;
      else if (!clock.Identity(permit.id,permit.session)) m1Reject="PLAN_SESSION_MISMATCH";
      else if (pc::TimeNs()>deadline) m1Reject="ACCEPT_DEADLINE";
      else {
        AcceptedRoot candidate;
        result = root.Poll(FenceState::Signaled, tail.data(), tail.size(), 0, candidate);
        if (result==ControlCode::OK) accepted=candidate;
        if (result==ControlCode::OK)
          finishOffer(offer,accepted.sourceSuccess ? IngressCode::Accepted : IngressCode::Rejected,
                      wide(request.nonce));
        busy.Complete(result==ControlCode::OK ? &tail : nullptr);
      }
    };
    m1Reject=sourceGuard();
    if (!m1Reject && !attempt.acceptPointer(inputSample.hold,accept))
      m1Reject=attempt.current() ? "SOURCE_HOLD_REVOKED" : "WINDOW_REVOKED";
    if (!m1Reject && result!=ControlCode::OK) m1Reject="TAIL_REJECTED";
    if (m1Reject) return reject();
    M1RootTrace("Source",wide(request.nonce),clock,permit,accepted,log);
    fields << ",\"m1Completion\":" << Words(tail) << ",\"m1OwnerCode\":" << int(result);
    fields << ",\"m1AdmittedQ\":" << tail[61] << ",\"m1PublicationGeneration\":" << accepted.publicationGeneration.lo
           << ",\"m1SourceMaterial\":" << cmd[sourceWire::kind] << ",\"m1SourceOutcome\":" << tail[28]
           << ",\"m1SourceAdmissionClass\":" << tail[158];
    if (!attempt.m1ReplayPrefix.empty() && accepted.sourceSuccess) {
      const auto exported=CaptureM1Quantum(root,accepted,tail,stateWords,cmd,{}).get();
      if (attempt.m1D1 && exported.bytes) ++d1WritersCompleted;
      fields << ",\"m1SourceReplaySHA\":" << Quote(exported.sha);
    }
    return true;
  };
  const auto consumeSource=[&](const PointerSample &input,uint64_t ns,uint64_t elapsed,const SourceOffer &offer) {
    try {
      const bool completed=runSource(input,ns,elapsed,offer);
      if (!completed) finishOffer(offer,IngressCode::Rejected,0,m1Reject);
      return completed && (!attempt.m1D1 || (!inFlight && root.State()!=FlightState::StateLost));
    } catch (...) {
      finishOffer(offer,IngressCode::Rejected);
      throw;
    }
  };
  if (!(consumeSource(inputSample,pulse,delta,firstOffer) && accepted.sourceSuccess && tail[61])) {
    if (attempt.m1D1) return finishWithoutWsi("M1_SOURCE_NOT_ADMITTED");
    Require(false,"M1_SOURCE_NOT_ADMITTED");
  }
  uint32_t ordinal=0;
  const auto recoverableClock=[&] {
    return !clock.open && m1Reject &&
        (std::strcmp(m1Reject,"CLOCK_GAP")==0 || std::strcmp(m1Reject,"PULSE_QUEUE_OVERFLOW")==0);
  };
  const auto advanceQuantum=[&](bool hadFrame) {
    while (!RunM1Quantum(candidate,root,clock,permit,refresh,accepted,frameSerial,ordinal)) {
      attempt.disarmM1Input();
      if (!attempt.current() || !recoverableClock() || inFlight || root.State()!=FlightState::Idle ||
          attempt.m1D1 || attempt.m1SinglePresent || !attempt.m1ColdFile.empty()) return false;
      if (!publish(Receipt(hadFrame ? "M1_STALE_WAIT_NEW_INPUT" : "M1_FAULT_WAIT_NEW_INPUT",m1Reject)))
        return false;
      fields.str(setupFields);
      fields.seekp(0,std::ios::end);
      beginClock(false);
      if (!awaitSource() || !primeClock()) return false;
      clock.ready=clock.baseline=true;
      if (!clock.Claim(pc::TimeNs(),0,inputSample.hold,true,permit)) return false;
      ++frameSerial;
      ordinal=0;
      stage="M1_RECOVERED_SOURCE";
      if (!consumeSource(inputSample,pulse,delta,firstOffer)) return false;
    }
    return true;
  };
  if (interactive && !advanceQuantum(false))
    return finishWithoutWsi("M1_QUANTUM_NOT_ACCEPTED");
  std::future<M1ColdResult> coldWriter;
  const auto captureAccepted=[&] {
    if (!interactive) return;
    if (!attempt.m1ReplayPrefix.empty()) {
      const auto exported=CaptureM1Quantum(root,accepted,tail,stateWords,cmd,{}).get();
      if (attempt.m1D1 && exported.bytes) ++d1WritersCompleted;
      fields << ",\"m1QuantumReplaySHA\":" << Quote(exported.sha);
    }
    if (attempt.m1ColdFile.empty()) return;
    const auto ticket=attempt.m1ColdOnBusy
        ? attempt.hotCapture->ClaimCold(wide(accepted.nonce),pc::TimeNs()) : std::optional<M1HotCapture::Page>{};
    if (ticket || !attempt.m1ColdOnBusy) {
      try {
        coldWriter=CaptureM1Quantum(root,accepted,tail,stateWords,cmd,ticket);
      } catch (...) {
        if (ticket) attempt.hotCapture->FailCold(*ticket);
        throw;
      }
    } else if (!attempt.m1RootCapture) {
      fields << ",\"m1ColdReservation\":\"NO_FIRST_QUANTUM_BUSY_RESERVATION\"";
    }
  };
  if (!attempt.m1RootCapture) captureAccepted();
  if (attempt.m1D1 && (inFlight || root.State()==FlightState::StateLost))
    return finishWithoutWsi("M1_CAPTURE_FENCE");
  const auto reportCold=[&] {
    if (!coldWriter.valid()) return;
    const auto exported=coldWriter.get();
    if (!exported.bytes) {
      fields << ",\"m1ColdReservation\":\"REVOKED_BEFORE_FROZEN\"";
      return;
    }
    fields << ",\"m1ColdPayloadSHA\":" << Quote(exported.sha)
           << ",\"m1ColdBytes\":" << exported.bytes;
  };
  if (!attempt.presentBoot) return Receipt("M1_DRY_SOURCE_ACCEPTED", "");

  Checkpoint("M1_SOURCE_WSI");
  PublicationLease lease;
  Require(root.Acquire(accepted.publication, accepted.publicationGeneration, lease) == ControlCode::OK,
          "M1_SOURCE_PUBLICATION_ACQUIRE");
  bool leased=true;
  bool nextRootFailed=false;
  bool coldPresent=true;
  const auto publicationBuffer=[&] {
    return buffers[!interactive ? kM1Pub : accepted.publication ? kM1QPubAlt : kM1QPub].handle;
  };
  std::function<VkBuffer()> nextPublication;
  if (interactive && !attempt.m1SinglePresent && (attempt.m1ColdFile.empty() || attempt.m1RootCapture)) {
    nextPublication=[&]() -> VkBuffer {
      Require(root.Release(accepted.publication,accepted.publicationGeneration,true)==ControlCode::OK,
              "M1_QUANTUM_FRAME_RELEASE");
      leased=false;
      fields.str(setupFields);
      fields.seekp(0,std::ios::end);
       if (D1Ended()) return VK_NULL_HANDLE;
      if (coldPresent) {
        // The first graphics pipeline is cold work. Resume with a new
        // anchor instead of turning its elapsed time into physics debt.
        beginClock(true);
        primeClock();
        coldPresent=false;
        fields << ",\"m1ClockReanchoredAfterColdPresent\":true";
      }
      bool sourceFatal=false;
      const auto reportSourceReject=[&] {
        publish(Receipt("M1_SOURCE_REVOKED",""));
        sourceFatal=clock.open && m1Reject && std::strcmp(m1Reject,"SOURCE_HOLD_REVOKED")!=0;
      };
      if (!clock.activeRemaining && !inFlight && root.State()==FlightState::Idle && attempt.current() && refresh()) {
        const auto observed=pc::TimeNs();
        const auto offer=takeFresh();
        fields << ",\"m1SourceBoundary\":{\"active\":" << clock.activeRemaining
               << ",\"pending\":" << clock.pending << ",\"frame\":\"" << frameSerial
               << "\",\"offer\":\"" << offer.seq << "\",\"previousPulse\":\"" << offer.previous.seq
               << "\",\"currentPulse\":\"" << offer.current.seq << "\",\"deltaNs\":\"" << offer.deltaNs
               << "\",\"ageNs\":\"" << (offer.seq && observed>=offer.pulseNs ? observed-offer.pulseNs : 0) << "\"}";
        if (offer.seq) {
          if (clock.Claim(pc::TimeNs(),0,offer.sample.hold,true,permit)) {
            ++frameSerial;
            ordinal=0;
            if (!consumeSource(offer.sample,offer.pulseNs,offer.deltaNs,offer)) reportSourceReject();
          } else {
            finishOffer(offer,IngressCode::Rejected,0,clock.open ? "GRANT_UNAVAILABLE" : clock.fault);
          }
        }
      }
       if (sourceFatal || inFlight || root.State()==FlightState::StateLost || D1Ended() || !advanceQuantum(true)) {
         const bool intakeStop=D1Ended() && (!sourceFatal ||
             (m1Reject && std::strcmp(m1Reject,"D1_END_INTAKE")==0)) && !inFlight &&
             root.State()==FlightState::Idle;
         nextRootFailed=attempt.current() && !intakeStop;
        attempt.disarmM1Input();
        if (nextRootFailed) publish(Receipt("FAILED","M1_QUANTUM_CONTINUATION_FAILED"));
         if (attempt.m1D1 && inFlight) {
           const auto end=attempt.d1Intake->Await();
           retireSubmitted(end);
           if (inFlight) reportDrain(end,nullptr);
         }
        return VK_NULL_HANDLE;
      }
      if (attempt.m1RootCapture || !attempt.m1ReplayPrefix.empty()) captureAccepted();
       if (attempt.m1D1 && (inFlight || root.State()==FlightState::StateLost)) {
         nextRootFailed=true;
         const auto end=attempt.d1Intake->Await();
         retireSubmitted(end);
         if (inFlight) reportDrain(end,nullptr);
         return VK_NULL_HANDLE;
       }
      Require(root.Acquire(accepted.publication,accepted.publicationGeneration,lease)==ControlCode::OK,
              "M1_QUANTUM_FRAME_ACQUIRE");
      leased=true;
      return publicationBuffer();
    };
  }
  pc::PresentBusy captureBusy;
  if (attempt.hotCapture && !attempt.m1ColdOnBusy) captureBusy=[&](M1HotCapture::Phase phase,bool entered,bool usable) {
    const auto nonce=wide(lease.nonce);
    if (entered) {
      attempt.hotCapture->Busy(phase,nonce);
    } else {
      attempt.hotCapture->StopBusy();
      if (!usable || !attempt.acceptPointer(0,[&] {
            attempt.hotCapture->Safe(nonce,&accepted.completion,pc::TimeNs());
          }))
        attempt.hotCapture->Safe(nonce,nullptr,pc::TimeNs());
    }
  };
  const auto frame = pc::PresentBoot(vk, instance, candidate.physical, device, queue, candidate.family,
      surface, publicationBuffer(), staging.handle, staging.memory, staging.flags, attempt.window,
      attempt.current, [&](const pc::Frame &visible) {
        reportCold();
        const auto &completion=accepted.completion;
        const auto q64=[&](size_t lo) { return wide({completion[lo],completion[lo+1]}); };
        if (fullM1) {
          namespace layout=grs::m1::completion256;
          fields << ",\"m1SceneLease\":{\"abi\":\"GRS_GPU_SCENE_Q20_M1_V2\",\"sand16Word\":0"
                 << ",\"water20Word\":" << 16*K << ",\"column16Word\":" << 16*K+20*Kw
                 << ",\"geometry\":\"" << wide(lease.identity.geometry)
                 << "\",\"sandStockQ\":\"" << q64(layout::sandAfterLo)
                 << "\",\"waterStockQ\":\"" << q64(layout::waterAfterLo)
                 << "\",\"sandAdmittedTotalQ\":\"" << q64(layout::I_sandLo)
                 << "\",\"waterAdmittedTotalQ\":\"" << q64(layout::I_waterLo)
                 << "\",\"activeSand\":" << completion[layout::sandLive]
                 << ",\"activeWater\":" << completion[layout::waterLive] << "}";
        }
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
      }, buffers[kM1Pub].bytes, !interactive, nextPublication, fullM1, captureBusy);
  reportCold();
  const bool usable = frame.retire != VK_ERROR_DEVICE_LOST && frame.error != VK_ERROR_DEVICE_LOST &&
                      root.State()!=FlightState::StateLost;
  if (usable && leased)
    Require(root.Release(accepted.publication, accepted.publicationGeneration, true) == ControlCode::OK,
            "M1_SOURCE_PUBLICATION_RELEASE");
  else if (!usable)
    root.ContextLost();
  fields << ",\"m1PublicationReleased\":" << (usable ? "true" : "false") << ",\"m1WsiResult\":" << int(frame.error)
         << ",\"m1WsiRetireVkResult\":" << int(frame.retire) << ",\"m1WsiStage\":" << Quote(frame.stage);
  if (attempt.m1D1) reportDrain(attempt.d1Intake->Await(),&frame);
  if (nextRootFailed) return Receipt("FAILED","M1_QUANTUM_CONTINUATION_FAILED");
  return frame.error == VK_SUCCESS ? Receipt(interactive ? "M1_QUANTUM_PRESENT_RETIRED" : "M1_DRY_SOURCE_PRESENT_RETIRED", "") :
                                    Receipt("WSI_FAILED", frame.stage);
}
