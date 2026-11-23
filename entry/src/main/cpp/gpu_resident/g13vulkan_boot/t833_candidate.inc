namespace {
constexpr uint64_t kCandidateGapNs=33333333;
constexpr uint64_t kCandidateJobNs=250000000;
constexpr size_t kCandidateSlotCount=14;
constexpr size_t kCandidateLatchWords=32;
constexpr size_t kCandidateErrorWords=10;

bool T833TailMatches(const Completion &tail,const Permit &permit,uint64_t quantum,uint64_t sourceSeq) {
  const bool source=permit[1]==50;
  const bool privateProject=permit[92]==1;
  if (tail[0]!=kMagic_tail || tail[1]!=1 || tail[2]!=kMagic_profile ||
      tail[3]!=256 || tail[255]!=kSeal || tail[4]!=permit[1] ||
      tail[37]!=permit[7] || tail[38]>>20 || tail[39]!=(source ? 16u : 820u) ||
      uint64_t(tail[40])+tail[41]!=tail[39] || tail[120]!=permit[3] ||
      tail[121]!=permit[5] || tail[125]!=1 || tail[126]!=permit[92] ||
      tail[5]!=(privateProject ? 7u : 1u) || tail[42]!=uint32_t(!privateProject) ||
      tail[43]!=3 || (tail[38]&permit[7])!=permit[7] ||
      g13::U64(tail.data()+34)!=quantum+(!source && !privateProject) ||
      (source && g13::U64(tail.data()+44)!=sourceSeq) || g13::U64(tail.data()+32)!=0)
    return false;
  for (const auto &group:std::array<std::array<size_t,3>,8>{{
      {{8,8,16}},{{24,32,8}},{{48,40,8}},{{56,48,8}},{{64,56,8}},
      {{72,64,8}},{{88,72,8}},{{128,24,8}}}})
    if (!std::equal(tail.begin()+group[0],tail.begin()+group[0]+group[2],permit.begin()+group[1]))
      return false;
  return std::all_of(tail.begin()+146,tail.begin()+255,[](uint32_t word) { return !word; });
}

std::array<uint32_t,1024> T833Capsule(const Config &config,const Input &original,
                                    const Input &canonical,const Permit &permit) {
  std::array<uint32_t,1024> capsule{};
  std::copy(original.begin(),original.end(),capsule.begin()+64);
  std::copy(canonical.begin(),canonical.end(),capsule.begin()+192);
  std::copy(config.begin(),config.end(),capsule.begin()+832);
  const auto receipt=[&](size_t at,size_t words,uint32_t tag,const std::string &body) {
    Require(body.size()<=(words-16)*4,"T833_CAPSULE_RANGE");
    capsule[at]=tag;
    capsule[at+1]=capsule[at+2]=1;
    capsule[at+3]=uint32_t(words);
    capsule[at+4]=uint32_t(body.size());
    const auto sha=Hash(body.data(),body.size());
    std::copy(sha.begin(),sha.end(),capsule.begin()+at+6);
    capsule[at+15]=kSeal;
    std::memcpy(capsule.data()+at+16,body.data(),body.size());
  };
  receipt(0,64,8,"{\"nativeNs\":\""+std::to_string(g13::U64(canonical.data()+22))+
          "\",\"serial\":\""+std::to_string(g13::U64(canonical.data()+106))+
          "\",\"sourceSeq\":\""+std::to_string(g13::U64(canonical.data()+20))+
          "\",\"normalization\":\"STAMP_ONLY\"}");
  receipt(320,128,9,"{\"fixtureSHA\":\""+Hex(config.data()+68)+"\"}");
  receipt(448,128,10,"{}");
  receipt(576,128,11,"{\"boundary\":"+std::to_string(config[6])+
          ",\"bottomBits\":"+std::to_string(config[32])+",\"topBits\":"+std::to_string(config[33])+"}");
  receipt(704,128,12,"{\"baseQuantum\":\""+std::to_string(g13::U64(permit.data()+88))+
          "\",\"sourceSeq\":\""+std::to_string(g13::U64(permit.data()+90))+
          "\",\"modeSource\":\"STATE_HEADER_24_31_192_222\"}");
  receipt(960,64,13,"{\"boot\":\""+std::to_string(kBoot)+
          "\",\"nonce\":\""+std::to_string(g13::U64(permit.data()+8))+
          "\",\"domain\":\"PAGE_NATIVE_CONTROL\"}");
  return capsule;
}
} // namespace

std::string Boot::RunT833Candidate(const Profile &profile,const Config &config) {
  const bool privateProject=attempt.t833Case=="F02";
  const bool sourceFirst=attempt.t833Case!="F02" && attempt.t833Case!="F03" && !attempt.t833F05Rest;
  const uint32_t diagnostic=privateProject ? 1u : attempt.t833Case.empty() ? 0u : 2u;
  fields << ",\"oldMonolithicPCReference\":\"MISSING_PC_REFERENCE\""
         << ",\"candidateInput\":" << Quote(attempt.t833Case.empty() ? "REST_BOOT_RAMP_SOURCE" : attempt.t833Case)
         << ",\"candidateRoute\":" << Quote(privateProject ? "PRIVATE_PROJECT" : "PC_SPLIT_CANDIDATE");
  const auto prepare=[&](bool source,const auto &plan) {
    for (size_t i=0;i<std::size(plan);++i) {
      pc::CandidatePass pass{};
      Require(pc::PlanSplitCandidate(source,i,plan[i],pass),"T833_SPLIT_PLAN");
      for (size_t k=0;k<pass.count;++k) {
        const auto name=pass.roles[k];
        const auto found=std::find_if(t833Pipelines.begin(),t833Pipelines.end(),
                                     [&](const auto &role) { return role.first==name; });
        if (found!=t833Pipelines.end()) continue;
        QualifySteady(std::string(name));
        t833Pipelines.emplace_back(name,pipeline);
        pipeline=VK_NULL_HANDLE;
      }
    }
  };
  if (sourceFirst) prepare(true,k_source);
  prepare(false,k_quantum);
  fields << ",\"candidateModules\":[";
  for (size_t i=0;i<t833Pipelines.size();++i) {
    for (const auto &module:pc::kSteadyModules) if (t833Pipelines[i].first==module.name) {
      if (i) fields << ',';
      fields << "{\"role\":" << Quote(module.name) << ",\"sha256\":" << Quote(module.sha) << '}';
      break;
    }
  }
  fields << ']';
  auto reset=reinterpret_cast<PFN_vkResetCommandPool>(vk.GetDeviceProcAddr(device,"vkResetCommandPool"));
  Require(reset,"T833_RESET_FUNCTION");

  const VkDeviceSize slotBytes=(kCandidateLatchWords+kCandidateErrorWords)*sizeof(uint32_t);
  const VkDeviceSize slotRegion=kCandidateSlotCount*slotBytes;
  const VkDeviceSize coldBytes=sizeof(Completion)+slotRegion+3*profile.sw*4ull+
                              buffers[kWork].bytes+buffers[kControl].bytes+2*profile.pw*4ull;
  Require(coldBytes<=16*1024*1024 && profile.ww%4==0,"T833_CAPTURE_RANGE");
  Allocate(kM1ColdStaging,coldBytes,VK_BUFFER_USAGE_TRANSFER_DST_BIT,true);
  const auto &staging=buffers[kM1ColdStaging];

  uint64_t lastPulse=attempt.sourcePulse(),pulseCount=lastPulse ? 1 : 0,earned=0,remainder=0,
           overload=0,maxGap=0,acceptedNonce=0,consumed=0;
  bool clockFault=false;
  const auto readPulse=[&] {
    if (clockFault) return false;
    for (const auto ns:attempt.t833Pulses(lastPulse)) {
      if (lastPulse) {
        const auto gap=ns-lastPulse;
        if (gap>kCandidateGapNs) {
          clockFault=true;
          return false;
        }
        maxGap=std::max(maxGap,gap);
        remainder+=120*gap;
        earned+=remainder/1000000000ull;
        remainder%=1000000000ull;
        if (earned-consumed-overload>4) overload=earned-consumed-4;
      }
      lastPulse=ns;
      ++pulseCount;
    }
    const auto now=pc::TimeNs();
    return lastPulse && now>=lastPulse && now-lastPulse<=kCandidateGapNs;
  };
  const auto beginClock=pc::TimeNs();
  while (attempt.current() && pc::TimeNs()-beginClock<kCandidateJobNs &&
         (!readPulse() || pulseCount<2)) Sleep(1);
  Require(attempt.current() && pulseCount>=2,"T833_REAL_PULSE_ABSENT");

  const g13::Identity id{attempt.ticket,GetCurrentThreadId(),attempt.ticket,attempt.surface,attempt.geometry,
                    attempt.ticket,attempt.ticket,attempt.ticket,attempt.ticket,attempt.ticket};
  const auto acceptedScope=[&](uint64_t commit) {
    const auto key=commit==0 ? g13::Identity{} : id;
    std::ostringstream token;
    token << "G13F1:" << kBoot << ':' << key.run << ':' << key.owner << ':' << key.context
          << ':' << key.surface << ':' << key.geometry << ':' << key.config << ':' << key.session
          << ':' << key.permission << ':' << key.inputEpoch << ':' << key.control << ':' << commit
          << ':' << acceptedNonce << ':' << Hex(config.data()+76);
    return token.str();
  };
  std::array<std::vector<uint8_t>,2> evidence;
  const auto root=[&](bool source,const auto &plan,uint32_t base,uint32_t candidate,uint32_t pack,
                       uint64_t commit,uint64_t sourceSeq,uint64_t quantumTick) {
    if (!source && earned==overload+consumed) {
      const auto waitFrom=pc::TimeNs();
      while (attempt.current() && pc::TimeNs()-waitFrom<kCandidateGapNs && earned==overload+consumed) {
        if (!readPulse()) break;
        Sleep(1);
      }
    }
    Require(attempt.current() && readPulse() && (source || earned>overload+consumed),"T833_PERMIT_MISSING_OR_STALE");
    const uint64_t grant=pc::TimeNs(),deadline=grant+kCandidateJobNs,nonce=pc::QpcTicks();
    Permit permit{};
    permit[0]=1; permit[1]=source ? 50 : 0; permit[3]=candidate;
    permit[4]=2; permit[5]=pack-3; permit[7]=RequiredAudits(source,diagnostic);
    permit[92]=diagnostic;
    permit[93]=privateProject ? 1 : attempt.t833ForcedJ;
    const std::array<uint64_t,16> lineage{nonce,commit+1,id.run,id.owner,id.context,id.surface,
      id.geometry,id.config,id.session,id.permission,id.inputEpoch,id.control,commit,commit+1,grant,deadline};
    for (size_t i=0;i<lineage.size();++i) Put(permit.data()+8+2*i,lineage[i]);
    for (const auto &h:std::array<std::pair<size_t,size_t>,4>{{{40,76},{48,36},{56,44},{64,52}}})
      std::copy_n(config.data()+h.second,8,permit.data()+h.first);
    const auto token=acceptedScope(commit);
    const auto baseHash=Hash(token.data(),token.size());
    std::copy(baseHash.begin(),baseHash.end(),permit.begin()+80);
    Put(permit.data()+88,quantumTick);
    Put(permit.data()+90,sourceSeq);
    permit[127]=kSeal;
    Input original{};
    original[0]=kMagic_input; original[1]=1; original[2]=128; original[3]=permit[1];
    const std::array<uint64_t,8> inputScope{id.run,id.context,id.surface,id.geometry,id.config,
                                          id.session,id.permission,id.control};
    for (size_t i=0;i<inputScope.size();++i) Put(original.data()+4+2*i,inputScope[i]);
    Put(original.data()+20,source ? 1 : nonce);
    Put(original.data()+22,source ? 0 : grant);
    Put(original.data()+24,commit);
    Put(original.data()+26,quantumTick);
    if (!source) { original[30]=permit[92]; original[31]=permit[93]; }
    for (const auto &h:std::array<std::pair<size_t,size_t>,6>{
        {{32,76},{40,36},{48,44},{56,52},{64,60},{72,68}}})
      std::copy_n(config.data()+h.second,8,original.data()+h.first);
    if (source) {
      original[88]=2; original[89]=2;
      original[90]=config[23]; original[91]=Bits(1.f); original[92]=Bits(4.f);
      original[101]=config[32]; original[102]=config[33]; original[103]=1;
      Put(original.data()+104,1);
    }
    original[127]=kSeal;
    SealHash(original,80);
    Input canonical=original;
    if (source) {
      Put(canonical.data()+22,grant);
      Put(canonical.data()+106,1);
      SealHash(canonical,80);
    }
    std::copy_n(canonical.data()+80,8,permit.data()+72);
    const auto capsule=T833Capsule(config,original,canonical,permit);
    std::array<uint32_t,256> quota{};
    for (const auto &entry:std::array<std::pair<size_t,uint64_t>,16>{{
        {0,id.session},{2,pulseCount},{4,lastPulse},{6,lastPulse},{8,pulseCount},{10,maxGap},
        {12,maxGap},{14,earned},{16,consumed},{18,overload},{22,remainder},{26,commit+1},{30,!source},
        {32,quantumTick},{36,id.control},{46,grant}}})
      Put(quota.data()+entry.first,entry.second);
    Put(quota.data()+34,sourceSeq);
    Put(quota.data()+48,deadline);

    const std::array<std::array<size_t,2>,4> pairs{{
        {{base,candidate}},{{base,2}},{{candidate,2}},{{candidate,pack}}}};
    for (size_t set=0;set<pairs.size();++set) {
      std::array<VkDescriptorBufferInfo,2> infos{};
      std::array<VkWriteDescriptorSet,2> writes{};
      for (uint32_t binding=0;binding<infos.size();++binding) {
        const auto &buffer=buffers[pairs[set][binding]];
        infos[binding]={buffer.handle,0,buffer.bytes};
        writes[binding]={VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,nullptr,sets[set],binding,
                        0,1,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,nullptr,&infos[binding],nullptr};
      }
      vk.UpdateDescriptorSets(device,uint32_t(writes.size()),writes.data(),0,nullptr);
    }
    Check(reset(device,commandPool,0),"vkResetCommandPool.T833");
    vk.DestroyFence(device,fence,nullptr);
    fence=VK_NULL_HANDLE;
    VkFenceCreateInfo fenceInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    Check(vk.CreateFence(device,&fenceInfo,nullptr,&fence),"vkCreateFence.T833");
    VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    Check(vk.BeginCommandBuffer(command,&begin),"vkBeginCommandBuffer.T833");
    const auto write=[&](size_t slot,size_t word,const auto &words) {
      vk.CmdUpdateBuffer(command,buffers[slot].handle,word*4,sizeof(words),&words);
    };
    write(kControl,128,permit);
    write(kControl,256,quota);
    write(kControl,1024,capsule);
    write(kLayout,kLayoutBytes/4,profile.ww);
    write(kWork,profile.ww,std::array<uint32_t,kCandidateLatchWords>{});
    Barrier(kTransfer,VK_ACCESS_TRANSFER_WRITE_BIT,kCompute,kShaderRW|VK_ACCESS_UNIFORM_READ_BIT);
    uint32_t issued=0,slotIndex=0,computeBarriers=0,copies=0;
    const auto copy=[&](size_t slot,VkDeviceSize from,VkDeviceSize to,VkDeviceSize bytes) {
      const VkBufferCopy region{from,to,bytes};
      vk.CmdCopyBuffer(command,buffers[slot].handle,staging.handle,1,&region);
      ++copies;
    };
    for (size_t ordinal=0;ordinal<std::size(plan);++ordinal) {
      pc::CandidatePass pass{};
      Require(pc::PlanSplitCandidate(source,ordinal,plan[ordinal],pass),"T833_RECORD_PLAN");
      const auto &binding=pass.binding;
      const auto set=binding.targetSlot==4 ? 3u : binding.baseSlot==1 ? 2u : binding.targetSlot==2 ? 1u : 0u;
      vk.CmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_COMPUTE,pipelineLayout,0,1,&sets[set],0,nullptr);
      vk.CmdPushConstants(command,pipelineLayout,VK_SHADER_STAGE_COMPUTE_BIT,0,
                          sizeof(binding.push),binding.push.data());
      for (size_t i=0;i<pass.count;++i) {
        const auto role=std::find_if(t833Pipelines.begin(),t833Pipelines.end(),
                                     [&](const auto &item) { return item.first==pass.roles[i]; });
        Require(role!=t833Pipelines.end(),"T833_ROLE_NOT_QUALIFIED");
        vk.CmdBindPipeline(command,VK_PIPELINE_BIND_POINT_COMPUTE,role->second);
        vk.CmdDispatch(command,1,1,1);
        ++issued;
        Barrier(kCompute,VK_ACCESS_SHADER_WRITE_BIT,kCompute,kShaderRW);
        ++computeBarriers;
      }
      if (plan[ordinal].op==17) {
        Require(slotIndex<kCandidateSlotCount,"T833_SLOT_RANGE");
        Barrier(kCompute,VK_ACCESS_SHADER_WRITE_BIT,kTransfer,VK_ACCESS_TRANSFER_READ_BIT);
        const auto where=sizeof(Completion)+slotIndex++*slotBytes;
        copy(kWork,uint64_t(profile.ww)*4,where,kCandidateLatchWords*4);
        copy(kControl,513*4,where+kCandidateLatchWords*4,kCandidateErrorWords*4);
        Barrier(kTransfer,VK_ACCESS_TRANSFER_READ_BIT,kCompute,kShaderRW);
      }
    }
    Barrier(kCompute,VK_ACCESS_SHADER_WRITE_BIT,kTransfer,VK_ACCESS_TRANSFER_READ_BIT);
    copy(kControl,kColdTailWordOffset*4,0,sizeof(Completion));
    const auto fieldsAt=sizeof(Completion)+slotIndex*slotBytes;
    VkDeviceSize offset=fieldsAt;
    const std::array<size_t,7> captured{base,candidate,2,kWork,kControl,3,4};
    std::array<uint32_t,7> bytes{};
    for (size_t i=0;i<captured.size();++i) {
      bytes[i]=uint32_t(buffers[captured[i]].bytes);
      copy(captured[i],0,offset,bytes[i]);
      offset+=bytes[i];
    }
    Barrier(kTransfer,VK_ACCESS_TRANSFER_WRITE_BIT,VK_PIPELINE_STAGE_HOST_BIT,VK_ACCESS_HOST_READ_BIT);
    Check(vk.EndCommandBuffer(command),"vkEndCommandBuffer.T833");
    const std::string prefix=source ? "t833Source" : "t833Quantum";
    fields << ",\"" << prefix << "Plan\":{\"logical\":" << std::size(plan) << ",\"dispatches\":" << issued
           << ",\"computeBarriers\":" << computeBarriers << ",\"copies\":" << copies
           << ",\"nonce\":\"" << nonce << "\",\"grantNs\":\"" << grant
           << "\",\"deadlineNs\":\"" << deadline << "\"}";
    if (commit==0 && attempt.t833RevokeAt=="before") attempt.t833Revoke();
    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.commandBufferCount=1;
    submit.pCommandBuffers=&command;
    VkResult submitted=VK_NOT_READY;
    uint64_t submittedQpc=0;
    if (!readPulse() || pc::TimeNs()>deadline || !attempt.acceptPointer(0,[&] {
          submittedQpc=pc::QpcTicks();
          submitted=vk.QueueSubmit(queue,1,&submit,fence);
        })) {
      fields << ",\"" << prefix << "Edge\":\"REVOKED_BEFORE_SUBMIT\"";
      return false;
    }
    Check(submitted,"vkQueueSubmit.T833");
    inFlight=true;
    if (commit==0 && attempt.t833RevokeAt=="after") attempt.t833Revoke();
    VkResult waited=VK_NOT_READY;
    while (attempt.current() && pc::TimeNs()<deadline && readPulse()) {
      waited=vk.WaitForFences(device,1,&fence,VK_TRUE,2000000);
      if (waited!=VK_TIMEOUT) break;
    }
    const bool timely=waited==VK_SUCCESS;
    if (waited!=VK_SUCCESS && waited!=VK_ERROR_DEVICE_LOST)
      waited=vk.WaitForFences(device,1,&fence,VK_TRUE,kCandidateJobNs);
    if (waited==VK_SUCCESS || waited==VK_ERROR_DEVICE_LOST) inFlight=false;
    fields << ",\"" << prefix << "Fence\":" << int(waited);
    fields << ",\"" << prefix << "Submit\":" << int(submitted)
           << ",\"" << prefix << "SubmitQpc\":\"" << submittedQpc << "\""
           << ",\"" << prefix << "FenceQpc\":\"" << pc::QpcTicks() << "\""
           << ",\"" << prefix << "FenceObject\":\"" << reinterpret_cast<uintptr_t>(fence) << "\"";
    Check(waited,"vkWaitForFences.T833_RETIRE");
    void *mapped=nullptr;
    Check(vk.MapMemory(device,staging.memory,0,VK_WHOLE_SIZE,0,&mapped),"vkMapMemory.T833");
    const VkMappedMemoryRange range{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,nullptr,staging.memory,0,VK_WHOLE_SIZE};
    const auto invalidate=(staging.flags&VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) ? VK_SUCCESS :
                          vk.InvalidateMappedMemoryRanges(device,1,&range);
    std::vector<uint8_t> raw(size_t(offset),uint8_t(0));
    if (invalidate==VK_SUCCESS) std::memcpy(raw.data(),mapped,raw.size());
    vk.UnmapMemory(device,staging.memory);
    Check(invalidate,"vkInvalidateMappedMemoryRanges.T833");
    Completion tail{};
    std::memcpy(tail.data(),raw.data(),sizeof(tail));
    bool latchOK=slotIndex==(source ? 0 : kCandidateSlotCount);
    size_t i=0;
    for (size_t ordinal=0;ordinal<std::size(plan);++ordinal) if (plan[ordinal].op==17) {
      std::array<uint32_t,kCandidateLatchWords> latch{};
      std::memcpy(latch.data(),raw.data()+sizeof(tail)+i++*slotBytes,sizeof(latch));
      pc::CandidatePass pass{};
      Require(pc::PlanSplitCandidate(source,ordinal,plan[ordinal],pass),"T833_CAPTURE_PLAN");
      latchOK=latchOK && latch[0]==0x54383534u && latch[1]==3 && g13::U64(latch.data()+2)==nonce &&
              std::equal(pass.binding.push.begin(),pass.binding.push.end(),latch.begin()+4) &&
              latch[10]<=1 && !latch[11] && !latch[30] && !latch[31];
    }
    uint64_t matchedNs=0;
    if (timely && readPulse() && latchOK && T833TailMatches(tail,permit,quantumTick,sourceSeq))
      attempt.acceptPointer(0,[&] {
        const auto observed=pc::TimeNs();
        if (observed<=deadline) {
          matchedNs=observed;
          if (!privateProject) acceptedNonce=nonce;
        }
      });
    const bool matched=matchedNs!=0;
    const bool accepted=matched && !privateProject;
    const uint32_t nativeError=matched ? 0u : !attempt.current() ? 11u :
        pc::TimeNs()>deadline ? 9u : tail[7] ? tail[7] : 2u;
    if (accepted && !source) ++consumed;
    fields << ",\"" << prefix << "Archive\":{\"schema\":\"PC_G13_T833_RAW_V2\",\"sha256\":"
           << Quote(Hex(Hash(raw.data(),raw.size()).data()))
           << ",\"fileBytes\":" << raw.size() << ",\"slotCount\":" << slotIndex << ",\"slotBytes\":" << slotBytes
           << ",\"fieldOffset\":" << fieldsAt << ",\"segmentsBytes\":" << Words(bytes)
           << ",\"segments\":[\"BASE\",\"CANDIDATE\",\"REPLAY\",\"WORK\",\"CTRL\",\"PACK0\",\"PACK1\"]"
           << ",\"baseBank\":" << base << ",\"candidateBank\":" << candidate
           << ",\"publicationSlot\":" << pack-3 << ",\"gpuTailAvailable\":true"
           << ",\"nativeError\":" << nativeError
           << ",\"acceptedNs\":\"" << (accepted ? matchedNs : 0) << "\""
           << ",\"baseToken\":" << Quote(token)
           << ",\"descriptorBuffers\":[";
    for (size_t j=0;j<captured.size();++j) {
      if (j) fields << ',';
      fields << Quote(std::to_string(reinterpret_cast<uintptr_t>(buffers[captured[j]].handle)));
    }
    fields << ']'
           << ",\"layoutWords\":" << Words(BootLayout(profile)) << ",\"completion\":" << Words(tail)
           << ",\"latchOK\":" << (latchOK ? "true" : "false")
           << ",\"privateProjectComplete\":" << (matched && privateProject ? "true" : "false") << ",\"acceptedRoot\":"
           << (accepted ? Quote(std::to_string(nonce)) : "null") << "}";
    evidence[source ? 0 : 1]=std::move(raw);
    return matched;
  };
  const auto exportEvidence=[&](uint32_t tick=0) {
    for (size_t i=0;i<evidence.size();++i) {
      auto &raw=evidence[i];
      if (raw.empty()) continue;
      auto path=attempt.t833CandidateFile+(i==0 ? L".source" : L".quantum");
      if (i && tick) path+=L"."+std::to_wstring(tick);
      const auto temp=path+L".pending-"+std::to_wstring(GetCurrentProcessId());
      const HANDLE output=CreateFileW(temp.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
      Require(output!=INVALID_HANDLE_VALUE,"T833_RAW_CREATE");
      DWORD wrote=0;
      const auto saved=WriteFile(output,raw.data(),DWORD(raw.size()),&wrote,nullptr) &&
                       wrote==raw.size() && FlushFileBuffers(output);
      CloseHandle(output);
      const auto committed=saved && MoveFileExW(temp.c_str(),path.c_str(),MOVEFILE_WRITE_THROUGH);
      if (!committed) DeleteFileW(temp.c_str());
      Require(committed,"T833_RAW_WRITE_OR_EXISTS");
      raw.clear();
    }
  };
  bool source=false,quantum=false;
  if (attempt.t833Case=="F03") {
    const auto initial=fields.str();
    for (uint32_t tick=0;tick<30;++tick) {
      fields.str(initial);
      fields.seekp(0,std::ios::end);
      fields << ",\"t833SeriesStep\":" << tick+1;
      const auto base=tick&1u;
      quantum=root(false,k_quantum,base,1-base,4-base,tick,0,tick);
      exportEvidence(tick+1);
      if (!attempt.t833RevokeAt.empty() && !attempt.current() && !quantum)
        return Receipt("T833_CANDIDATE_REVOKED","");
      const bool kept=publish(Receipt(quantum ? "T833_SERIES_ROOT" : "FAILED",
                                     quantum ? "" : "T833_F03_ROOT_NOT_ACCEPTED"));
      if (!quantum || !kept) return Receipt("FAILED","T833_F03_SERIES_INCOMPLETE");
    }
    fields << ",\"t833SeriesComplete\":30";
    return Receipt("T833_CANDIDATE_CAPTURED","");
  }
  try {
    if (sourceFirst) source=root(true,k_source,0,1,4,0,1,0);
    if (!sourceFirst) quantum=root(false,k_quantum,0,1,4,0,0,0);
    else if (source) quantum=root(false,k_quantum,1,0,3,1,1,0);
  } catch (...) {
    exportEvidence();
    throw;
  }
  exportEvidence();
  if (!attempt.t833RevokeAt.empty() && !attempt.current() && !source && !quantum)
    return Receipt("T833_CANDIDATE_REVOKED","");
  if (sourceFirst && !source) return Receipt("FAILED","T833_SOURCE_NOT_ACCEPTED");
  if (!quantum) return Receipt("FAILED","T833_QUANTUM_NOT_ACCEPTED");
  return Receipt(privateProject ? "T833_PRIVATE_PROJECT_CAPTURED" : "T833_CANDIDATE_CAPTURED","");
}
