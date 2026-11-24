namespace {
using pc::kCandidateGapNs;
using pc::kCandidateJobNs;
constexpr size_t kCandidateSlotCount=14;
constexpr size_t kCandidateLatchWords=32;
constexpr size_t kCandidateErrorWords=10;
constexpr size_t kCandidateLatchBytes=kCandidateLatchWords*sizeof(uint32_t);
constexpr size_t kCandidateSlotBytes=(kCandidateLatchWords+kCandidateErrorWords)*sizeof(uint32_t);
constexpr size_t kCandidateSlotRegion=kCandidateSlotCount*kCandidateSlotBytes;
constexpr bool CandidateSnapshotOrdinalsMatch() {
  size_t slot=0;
  for (size_t i=0;i<std::size(k_quantum);++i) {
    if (k_quantum[i].op!=17) continue;
    if (slot>=std::size(pc::kT899SlotOrdinals) || pc::kT899SlotOrdinals[slot++]!=i) return false;
  }
  for (auto &pass:k_source) if (pass.op==17) return false;
  return slot==kCandidateSlotCount && slot==std::size(pc::kT899SlotOrdinals);
}
static_assert(CandidateSnapshotOrdinalsMatch());
static_assert(k_quantum[815].op==7 && k_quantum[816].op==8 && k_quantum[817].op==9 &&
              k_quantum[818].op==10 && k_quantum[819].op==11);
enum T877Point {
  Upload, ActualBegin, ActualEnd, ReplayBegin, ReplayEnd,
  ActualPressureBegin, ActualPressureEnd, ActualViscosityBegin, ActualViscosityEnd,
  ReplayPressureBegin, ReplayPressureEnd, ReplayViscosityBegin, ReplayViscosityEnd,
  CompareEnd, PackEnd, BeforeTail, BusinessCopiesEnd, ColdCopiesEnd, RootBegin, ColdCopiesBegin,
  FinalAuditBegin, FinalAuditEnd, TimePrepareEnd, PackViewEnd, SealBegin, SealEnd, TransferReady, PackAuditEnd,
  ActualJ1ViscosityBegin, ActualJ1ViscosityEnd, ActualJ1PressureBegin, ActualJ1PressureEnd,
  ReplayJ1ViscosityBegin, ReplayJ1ViscosityEnd, ReplayJ1PressureBegin, ReplayJ1PressureEnd,
  SealPrepareEnd, SealShaEnd, CycleBegin, CycleEnd=CycleBegin+16,
  TrialEdges=CycleEnd+1, PointCount=TrialEdges+8
};
constexpr const char *kT877PointNames[]{
    "UPLOAD_DONE","ACTUAL_BEGIN","ACTUAL_END","REPLAY_BEGIN","REPLAY_END",
    "ACTUAL_PRESSURE_SPAN_BEGIN","ACTUAL_PRESSURE_SPAN_END","ACTUAL_VISCOSITY_SPAN_BEGIN","ACTUAL_VISCOSITY_SPAN_END",
    "REPLAY_PRESSURE_SPAN_BEGIN","REPLAY_PRESSURE_SPAN_END","REPLAY_VISCOSITY_SPAN_BEGIN","REPLAY_VISCOSITY_SPAN_END",
    "COMPARE_DONE","PACK_DONE","BEFORE_TAIL","BUSINESS_COPIES_DONE","COLD_COPIES_DONE","ROOT_BEGIN","COLD_COPIES_BEGIN",
    "FINAL_AUDIT_BEGIN","FINAL_AUDIT_END","TIME_PREPARE_END","PACK_VIEW_END","SEAL_BEGIN","SEAL_END","TAIL_TRANSFER_READY",
    "PACK_AUDIT_END",
    "ACTUAL_J1_MICRO0_VISC_BLOCK0_BEGIN","ACTUAL_J1_MICRO0_VISC_BLOCK0_END",
    "ACTUAL_J1_MICRO0_PRESSURE_CYCLE0_BEGIN","ACTUAL_J1_MICRO0_PRESSURE_CYCLE0_END",
    "REPLAY_J1_MICRO0_VISC_BLOCK0_BEGIN","REPLAY_J1_MICRO0_VISC_BLOCK0_END",
    "REPLAY_J1_MICRO0_PRESSURE_CYCLE0_BEGIN","REPLAY_J1_MICRO0_PRESSURE_CYCLE0_END",
    "SEAL_PREPARE_END","SEAL_SHA_END"};
static_assert(std::size(kT877PointNames)==CycleBegin && PointCount<=64);
std::string T877PointName(size_t point) {
  if (point<CycleBegin) return kT877PointNames[point];
  if (point==CycleBegin) return "J1_OPERATOR_BEGIN";
  if (point<=CycleEnd) return "J1_OPERATOR_BLOCK_"+std::to_string(point-CycleBegin-1)+"_END";
  const auto edge=point-TrialEdges;
  return std::string(edge<4?"ACTUAL":"REPLAY")+"_J"+(edge%4<2?"2":"4")+(edge%2?"_END":"_BEGIN");
}

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

void Boot::ReportLateCandidate(VkResult confirmed,VkResult drained,uint64_t entered,uint64_t exited) {
  const auto &d=t877Late;
  struct Point { uint64_t tick=0,available=0; };
  std::array<Point,PointCount> points{};
  const bool readable=confirmed==VK_SUCCESS && d.pool && d.read && d.bits && d.bits<=64;
  const auto mask=d.bits==64 ? UINT64_MAX : d.bits && d.bits<64 ? (uint64_t{1}<<d.bits)-1 : 0;
  std::ostringstream row;
  row << "{\"stage\":\"T877_GPU_TIMESTAMPS\",\"state\":"
      << Quote(d.cold ? "LATE_COLD_COPY_DIAGNOSTIC" : "LATE_RETIRED_DIAGNOSTIC")
      << ",\"businessRejected\":" << (d.cold ? "null" : "true")
      << ",\"acceptedRoot\":null,\"rootKind\":" << Quote(d.source ? "SOURCE" : "QUANTUM")
      << ",\"run\":\"" << d.run << "\",\"nonce\":\"" << d.nonce
      << "\",\"ticket\":\"" << attempt.ticket << "\",\"geometry\":\"" << attempt.geometry
      << "\",\"device\":\"" << reinterpret_cast<uintptr_t>(d.device)
      << "\",\"queue\":\"" << reinterpret_cast<uintptr_t>(d.queue)
      << "\",\"fence\":\"" << reinterpret_cast<uintptr_t>(d.fence)
      << "\",\"fenceGeneration\":\"" << d.generation
      << "\",\"queryPool\":\"" << reinterpret_cast<uintptr_t>(d.pool)
      << "\",\"writtenMask\":\"" << d.written << "\",\"firstStop\":" << Quote(d.firstStop)
      << ",\"stopQpc\":\"" << d.stopQpc << "\",\"retireConfirmEnterQpc\":\"" << entered
      << "\",\"retireConfirmExitQpc\":\"" << exited << "\",\"fenceVkResult\":" << int(confirmed)
      << ",\"drainVkResult\":" << int(drained) << ",\"validBits\":" << d.bits
      << ",\"periodNs\":" << d.period << ",\"points\":[";
  bool comma=false,any=false;
  for (uint32_t i=0;i<PointCount;++i) {
    if (!(d.written&(uint64_t{1}<<i))) continue;
    auto &p=points[i];
    const auto status=readable ? d.read(d.device,d.pool,i,1,sizeof(p),&p,sizeof(p),
                                      VK_QUERY_RESULT_64_BIT|VK_QUERY_RESULT_WITH_AVAILABILITY_BIT) : VK_NOT_READY;
    if (status!=VK_SUCCESS) p.available=0;
    any=any || p.available;
    if (comma) row << ',';
    comma=true;
    row << "{\"point\":" << Quote(T877PointName(i)) << ",\"vkResult\":" << int(status)
        << ",\"available\":" << (p.available ? "true" : "false") << ",\"tick\":"
        << (p.available ? Quote(std::to_string(p.tick&mask)) : "null") << "}";
  }
  row << "],\"queryState\":" << Quote(any ? "AVAILABLE_POINTS" : "UNAVAILABLE") << ",\"spansNs\":{";
  const auto span=[&](const char *name,T877Point a,T877Point b,bool separator=true) {
    row << (separator ? "," : "") << Quote(name) << ':';
    if (points[a].available && points[b].available)
      row << pc::TimestampDelta(points[a].tick,points[b].tick,d.bits)*double(d.period);
    else row << "null";
  };
  span("root",RootBegin,BusinessCopiesEnd,false);
  span("actual",ActualBegin,ActualEnd);
  span("replay",ReplayBegin,ReplayEnd);
  span("comparePack",ReplayEnd,PackEnd);
  span("businessCopies",BeforeTail,BusinessCopiesEnd);
  span("independentColdCopy",ColdCopiesBegin,ColdCopiesEnd);
  publish(row.str()+"}}");
  if (!d.slotCount) return;
  const auto &staging=buffers[kM1ColdStaging];
  std::array<uint32_t,kCandidateSlotRegion/sizeof(uint32_t)> slots{};
  VkResult slotRead=confirmed;
  if (confirmed==VK_SUCCESS) {
    void *mapped=nullptr;
    slotRead=vk.MapMemory(device,staging.memory,0,VK_WHOLE_SIZE,0,&mapped);
    if (slotRead==VK_SUCCESS) {
      const VkMappedMemoryRange range{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,nullptr,staging.memory,0,VK_WHOLE_SIZE};
      if (!(staging.flags&VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
        slotRead=vk.InvalidateMappedMemoryRanges(device,1,&range);
      if (slotRead==VK_SUCCESS)
        std::memcpy(slots.data(),static_cast<const uint8_t *>(mapped)+sizeof(Completion),d.slotCount*kCandidateSlotBytes);
      vk.UnmapMemory(device,staging.memory);
    }
  }
  std::ostringstream evidenceRow;
  evidenceRow << "{\"stage\":\"T899_SLOT_EVIDENCE\",\"state\":\"LATE_RETIRED_DIAGNOSTIC\",\"acceptedRoot\":null"
              << ",\"run\":\"" << d.run << "\",\"nonce\":\"" << d.nonce
              << "\",\"fenceGeneration\":\"" << d.generation << "\",\"slotCount\":" << d.slotCount
              << ",\"fenceVkResult\":" << int(confirmed) << ",\"readVkResult\":" << int(slotRead)
              << ",\"snapshot\":" << (attempt.t899SlotSnapshot ? "true" : "false") << ",\"slotWords\":";
  if (slotRead==VK_SUCCESS) {
    evidenceRow << '[';
    for (size_t i=0;i<d.slotCount*kCandidateSlotBytes/sizeof(uint32_t);++i)
      evidenceRow << (i ? "," : "") << slots[i];
    evidenceRow << ']';
  } else evidenceRow << "null";
  publish(evidenceRow.str()+"}");
}

std::string Boot::RunT833Candidate(const Profile &profile,const Config &config,const Candidate &hardware) {
  const bool privateProject=attempt.t833Case=="F02";
  const bool sourceFirst=attempt.t833Case!="F02" && attempt.t833Case!="F03" && !attempt.t833F05Rest;
  const uint32_t diagnostic=privateProject ? 1u : attempt.t833Case.empty() ? 0u : 2u;
  const bool t877=!attempt.t877Mode.empty(),hot=attempt.t877Mode=="hot";
  const bool snapshot=attempt.t899SlotSnapshot;
  const bool stagedSeal=attempt.g13SealStages;
  const bool diagnosticRaw=stagedSeal || !attempt.g13CycleScope.empty();
  const auto selectSeal=[&](pc::CandidatePass &pass,uint32_t op) {
    if (stagedSeal && op==11) {
      pass.roles={"OP11_T955_PREPARE","OP11_T955_SHA","OP11_T955_PUBLISH"};
      pass.count=3;
    }
  };
  const auto evidenceRole=[&](std::string_view role,uint32_t op) {
    if (attempt.g13SplitOp48 && role=="OP_48") {
      if (op==48) return std::string_view("OP48_VISCOSITY");
      if (op==49) return std::string_view("OP49_PRESSURE");
      return std::string_view("OP48_REMAINDER");
    }
    if (attempt.g13SealV1 && op==11 && role=="OP_00_9_11")
      return std::string_view("OP_00_9_11_SEAL_V1");
    return snapshot && role=="OP17_PRIVATE_LEDGER" ? std::string_view("OP17_PRIVATE_LEDGER_SNAPSHOT") : role;
  };
  fields << ",\"oldMonolithicPCReference\":\"MISSING_PC_REFERENCE\""
         << ",\"candidateInput\":" << Quote(attempt.t833Case.empty() ? "REST_BOOT_RAMP_SOURCE" : attempt.t833Case)
         << ",\"candidateRoute\":" << Quote(privateProject ? "PRIVATE_PROJECT" : "PC_SPLIT_CANDIDATE")
         << ",\"t899SlotSnapshot\":" << (snapshot ? "true" : "false")
         << ",\"t946SplitOp48\":" << (attempt.g13SplitOp48 ? "true" : "false")
         << ",\"t909SealV1\":" << (attempt.g13SealV1 ? "true" : "false");
  if (diagnosticRaw)
    fields << ",\"t955StagedSeal\":" << (stagedSeal ? "true" : "false")
           << ",\"t955CycleScope\":" << Quote(attempt.g13CycleScope)
           << ",\"t955CycleOperator\":" << Quote(privateProject?"PRESSURE":"VISCOSITY");
  const auto prepare=[&](bool source,const auto &plan) {
    for (size_t i=0;i<std::size(plan);++i) {
      pc::CandidatePass pass{};
      Require(pc::PlanSplitCandidate(source,i,plan[i],pass),"T833_SPLIT_PLAN");
      selectSeal(pass,plan[i].op);
      for (size_t k=0;k<pass.count;++k) {
        const auto name=evidenceRole(pass.roles[k],plan[i].op);
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
  const auto createQueries=reinterpret_cast<PFN_vkCreateQueryPool>(vk.GetDeviceProcAddr(device,"vkCreateQueryPool"));
  const auto resetQueries=reinterpret_cast<PFN_vkCmdResetQueryPool>(vk.GetDeviceProcAddr(device,"vkCmdResetQueryPool"));
  const auto writeTimestamp=reinterpret_cast<PFN_vkCmdWriteTimestamp>(vk.GetDeviceProcAddr(device,"vkCmdWriteTimestamp"));
  const auto readQueries=reinterpret_cast<PFN_vkGetQueryPoolResults>(vk.GetDeviceProcAddr(device,"vkGetQueryPoolResults"));
  t877DestroyQueries=reinterpret_cast<PFN_vkDestroyQueryPool>(vk.GetDeviceProcAddr(device,"vkDestroyQueryPool"));
  VkResult queryCreate=VK_NOT_READY;
  const bool queryAttempted=t877 && hardware.timestampBits && createQueries && t877DestroyQueries &&
                            resetQueries && writeTimestamp && readQueries;
  if (queryAttempted) {
    VkQueryPoolCreateInfo query{VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};
    query.queryType=VK_QUERY_TYPE_TIMESTAMP;
    query.queryCount=PointCount;
    queryCreate=createQueries(device,&query,nullptr,&t877Queries);
    if (queryCreate!=VK_SUCCESS) t877Queries=VK_NULL_HANDLE;
  }
  if (t877) fields << ",\"t877Mode\":" << Quote(attempt.t877Mode) << ",\"t877TimestampValidBits\":"
                   << hardware.timestampBits << ",\"t877TimestampPeriodNs\":" << hardware.properties.limits.timestampPeriod
                   << ",\"t877TimestampCreateVkResult\":" << (queryAttempted ? std::to_string(int(queryCreate)) : "null");

  const auto archiveWorkBytes=buffers[kWork].bytes-(snapshot ? kCandidateSlotRegion : 0);
  const auto slotBytes=kCandidateSlotBytes;
  const VkDeviceSize coldBytes=sizeof(Completion)+kCandidateSlotRegion+3*profile.sw*4ull+
                              archiveWorkBytes+buffers[kControl].bytes+2*profile.pw*4ull;
  Require(coldBytes<=16*1024*1024 && profile.ww%4==0,"T833_CAPTURE_RANGE");
  Allocate(kM1ColdStaging,coldBytes,VK_BUFFER_USAGE_TRANSFER_DST_BIT,true);
  const auto &staging=buffers[kM1ColdStaging];

  const auto recent=attempt.t833Pulses(0);
  pc::CandidatePulse lastPulse=recent.empty() ? pc::CandidatePulse{} : recent.back();
  pc::CandidatePulse observedPulse=lastPulse;
  uint64_t pulseCount=lastPulse.sequence ? 1 : 0,earned=0,remainder=0,
           overload=0,maxGap=0,maxReceiptGap=0,acceptedNonce=0,consumed=0;
  const char *clockFault=nullptr;
  const auto readPulse=[&] {
    if (clockFault) return false;
    for (const auto &sample:attempt.t833Pulses(lastPulse.sequence)) {
      observedPulse=sample;
      if ((clockFault=pc::CandidatePulseOrder(lastPulse,sample))) return false;
      if (lastPulse.sequence) {
        const auto gap=sample.callbackNs-lastPulse.callbackNs;
        maxGap=std::max(maxGap,gap);
        maxReceiptGap=std::max(maxReceiptGap,sample.receiptNs-lastPulse.receiptNs);
        remainder+=120*gap;
        earned+=remainder/1000000000ull;
        remainder%=1000000000ull;
        if (earned-consumed-overload>4) overload=earned-consumed-4;
      }
      lastPulse=sample;
      ++pulseCount;
    }
    return !pc::CandidatePulseFreshness(lastPulse,pc::TimeNs());
  };
  const auto beginClock=pc::TimeNs();
  while (attempt.current() && pc::TimeNs()-beginClock<kCandidateJobNs &&
         (!readPulse() || pulseCount<2)) Sleep(1);
  if (!attempt.current() || pulseCount<2 || !readPulse()) {
    if (t877) fields << ",\"t877InitialPulseSeq\":\"" << lastPulse.sequence
                     << "\",\"t877InitialPulseFault\":" << Quote(!attempt.current() ? "WINDOW_REVOKED" :
                                                                   clockFault ? clockFault : "PULSE_MISSING");
    return Receipt("FAILED","T833_REAL_PULSE_ABSENT");
  }

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
  std::array<bool,2> evidenceIsHot{};
  bool coldFailure=false;
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
    const auto recordBegin=pc::QpcTicks();
    stage=source ? "T833_SOURCE_RECORD" : "T833_QUANTUM_RECORD";
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
        {0,id.session},{2,pulseCount},{4,lastPulse.callbackNs},{6,lastPulse.receiptNs},{8,pulseCount},{10,maxGap},
        {12,maxReceiptGap},{14,earned},{16,consumed},{18,overload},{22,remainder},{26,commit+1},{30,!source},
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
    ++t877FenceGeneration;
    VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    Check(vk.BeginCommandBuffer(command,&begin),"vkBeginCommandBuffer.T833");
    std::array<bool,PointCount> written{};
    const auto mark=[&](T877Point point,VkPipelineStageFlagBits pipeStage) {
      if (!t877Queries) return;
      writeTimestamp(command,pipeStage,t877Queries,point);
      written[point]=true;
    };
    if (t877Queries) resetQueries(command,t877Queries,0,PointCount);
    mark(RootBegin,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
    const auto writtenMask=[&] {
      uint64_t mask=0;
      for (size_t i=0;i<written.size();++i) if (written[i]) mask|=uint64_t{1}<<i;
      return mask;
    };
    const auto reportGpu=[&](bool safe,bool coldSafe=false) {
      if (!t877) return;
      std::ostringstream row;
      row << "{\"stage\":\"T877_GPU_TIMESTAMPS\",\"nonce\":\"" << nonce
          << "\",\"ticket\":\"" << attempt.ticket << "\",\"validBits\":" << hardware.timestampBits
          << ",\"periodNs\":" << hardware.properties.limits.timestampPeriod
          << ",\"queryPool\":\"" << reinterpret_cast<uintptr_t>(t877Queries)
          << "\",\"writtenMask\":\"" << writtenMask() << "\""
          << ",\"state\":" << Quote(!t877Queries ? "UNAVAILABLE" : !safe ? "FENCE_NOT_SIGNALED" : "READ_AFTER_FENCE")
          << ",\"points\":[";
      bool comma=false;
      for (uint32_t i=0;i<PointCount;++i) if (written[i]) {
        std::array<uint64_t,2> values{}; // value, availability
        const bool readable=safe && ((i!=ColdCopiesEnd && i!=ColdCopiesBegin) || coldSafe);
        const auto status=readable ? readQueries(device,t877Queries,i,1,sizeof(values),values.data(),
                                              sizeof(values),VK_QUERY_RESULT_64_BIT|VK_QUERY_RESULT_WITH_AVAILABILITY_BIT)
                               : VK_NOT_READY;
        if (comma) row << ',';
        comma=true;
        row << "{\"point\":" << Quote(T877PointName(i)) << ",\"vkResult\":"
            << (readable ? std::to_string(int(status)) : "null")
            << ",\"available\":" << (values[1] ? "true" : "false") << ",\"tick\":"
            << (values[1] ? Quote(std::to_string(values[0])) : "null") << '}';
      }
      publish(row.str()+"]}");
    };
    const auto write=[&](size_t slot,size_t word,const auto &words) {
      vk.CmdUpdateBuffer(command,buffers[slot].handle,word*4,sizeof(words),&words);
    };
    write(kControl,128,permit);
    write(kControl,256,quota);
    write(kControl,1024,capsule);
    write(kLayout,kLayoutBytes/4,profile.ww);
    write(kWork,profile.ww,std::array<uint32_t,kCandidateLatchWords>{});
    Barrier(kTransfer,VK_ACCESS_TRANSFER_WRITE_BIT,kCompute,kShaderRW|VK_ACCESS_UNIFORM_READ_BIT);
    mark(Upload,VK_PIPELINE_STAGE_TRANSFER_BIT);
    uint32_t issued=0,slotIndex=0,computeBarriers=0,copies=0;
    uint32_t pipelineBinds=0,descriptorBinds=0;
    VkPipeline boundPipeline=VK_NULL_HANDLE;
    VkDescriptorSet boundSet=VK_NULL_HANDLE;
    const auto copy=[&](size_t slot,VkDeviceSize from,VkDeviceSize to,VkDeviceSize bytes) {
      const VkBufferCopy region{from,to,bytes};
      vk.CmdCopyBuffer(command,buffers[slot].handle,staging.handle,1,&region);
      ++copies;
    };
    std::array<size_t,PointCount> positions;
    positions.fill(std::size(plan));
    if (t877Queries) {
      const auto span=[&](size_t ordinal,T877Point first,T877Point last) {
        positions[first]=std::min(positions[first],ordinal);
        positions[last]=ordinal;
      };
      for (size_t i=0;i<std::size(plan);++i) {
        const auto &p=plan[i];
        RootBinding binding{};
        Require(ResolveRootBinding(source,i,plan[i],binding),"T877_PHASE_PLAN");
        if (binding.phase==RootPhase::Actual) {
          span(i,ActualBegin,ActualEnd);
          if (plan[i].op==OP_PRESSURE_V) span(i,ActualPressureBegin,ActualPressureEnd);
          if (plan[i].op==OP_VISC_BLOCK) span(i,ActualViscosityBegin,ActualViscosityEnd);
        }
        if (binding.phase==RootPhase::Replay) {
          span(i,ReplayBegin,ReplayEnd);
          if (plan[i].op==OP_PRESSURE_V) span(i,ReplayPressureBegin,ReplayPressureEnd);
          if (plan[i].op==OP_VISC_BLOCK) span(i,ReplayViscosityBegin,ReplayViscosityEnd);
        }
        if (binding.phase==RootPhase::Compare) positions[CompareEnd]=i;
        if (binding.phase==RootPhase::Pack) positions[PackEnd]=i;
        if (!source && !attempt.g13CycleScope.empty() && (p.scope==1 || p.scope==2) &&
            (p.J==2 || p.J==4)) {
          const auto entry=TrialEdges+4*(p.scope-1)+(p.J==4?2:0);
          span(i,T877Point(entry),T877Point(entry+1));
        }
        if (privateProject && attempt.g13TailTiming && plan[i].J==1 && plan[i].micro==0 && plan[i].block==0) {
          if (binding.phase==RootPhase::Actual && plan[i].op==OP_VISC_BLOCK)
            span(i,ActualJ1ViscosityBegin,ActualJ1ViscosityEnd);
          if (binding.phase==RootPhase::Actual && plan[i].op==OP_PRESSURE_V)
            span(i,ActualJ1PressureBegin,ActualJ1PressureEnd);
          if (binding.phase==RootPhase::Replay && plan[i].op==OP_VISC_BLOCK)
            span(i,ReplayJ1ViscosityBegin,ReplayJ1ViscosityEnd);
          if (binding.phase==RootPhase::Replay && plan[i].op==OP_PRESSURE_V)
            span(i,ReplayJ1PressureBegin,ReplayJ1PressureEnd);
        }
      }
    }
    constexpr T877Point beforePoints[]{ActualBegin,ReplayBegin,ActualPressureBegin,ActualViscosityBegin,
                                       ReplayPressureBegin,ReplayViscosityBegin,FinalAuditBegin,SealBegin,
                                       ActualJ1ViscosityBegin,ActualJ1PressureBegin,ReplayJ1ViscosityBegin,ReplayJ1PressureBegin};
    constexpr T877Point afterPoints[]{ActualEnd,ReplayEnd,ActualPressureEnd,ActualViscosityEnd,
                                      ReplayPressureEnd,ReplayViscosityEnd,CompareEnd,PackEnd,
                                      FinalAuditEnd,TimePrepareEnd,PackViewEnd,PackAuditEnd,SealEnd,
                                      ActualJ1ViscosityEnd,ActualJ1PressureEnd,ReplayJ1ViscosityEnd,ReplayJ1PressureEnd};
    if (t877Queries && attempt.g13TailTiming && !source) {
      positions[FinalAuditBegin]=positions[FinalAuditEnd]=815;
      positions[TimePrepareEnd]=816;
      positions[PackViewEnd]=817;
      positions[PackAuditEnd]=818;
      positions[SealBegin]=positions[SealEnd]=819;
    }
    for (size_t ordinal=0;ordinal<std::size(plan);++ordinal) {
      for (auto point:beforePoints) if (positions[point]==ordinal) mark(point,kCompute);
      pc::CandidatePass pass{};
      Require(pc::PlanSplitCandidate(source,ordinal,plan[ordinal],pass),"T833_RECORD_PLAN");
      selectSeal(pass,plan[ordinal].op);
      const auto &op=plan[ordinal];
      const bool cycle=!source && !attempt.g13CycleScope.empty() && op.J==1 && op.micro==0 &&
          op.scope==(attempt.g13CycleScope=="actual"?1u:2u) &&
          op.op==(privateProject?OP_PRESSURE_V:OP_VISC_BLOCK);
      if (cycle && op.block==0) mark(CycleBegin,kCompute);
      for (size_t i=0;i<8;i+=2)
        if(positions[TrialEdges+i]==ordinal) mark(T877Point(TrialEdges+i),kCompute);
      const auto &binding=pass.binding;
      const auto set=pc::CandidateSet(binding);
      if (!t877 || boundSet!=sets[set]) {
        boundSet=sets[set];
        vk.CmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_COMPUTE,pipelineLayout,0,1,&boundSet,0,nullptr);
        ++descriptorBinds;
      }
      vk.CmdPushConstants(command,pipelineLayout,VK_SHADER_STAGE_COMPUTE_BIT,0,
                          sizeof(binding.push),binding.push.data());
      for (size_t i=0;i<pass.count;++i) {
        const auto name=evidenceRole(pass.roles[i],plan[ordinal].op);
        const auto role=std::find_if(t833Pipelines.begin(),t833Pipelines.end(),
                                     [&](const auto &item) { return item.first==name; });
        Require(role!=t833Pipelines.end(),"T833_ROLE_NOT_QUALIFIED");
        if (!t877 || boundPipeline!=role->second) {
          boundPipeline=role->second;
          vk.CmdBindPipeline(command,VK_PIPELINE_BIND_POINT_COMPUTE,boundPipeline);
          ++pipelineBinds;
        }
        vk.CmdDispatch(command,1,1,1);
        ++issued;
        Barrier(kCompute,VK_ACCESS_SHADER_WRITE_BIT,kCompute,kShaderRW);
        ++computeBarriers;
        if (stagedSeal && op.op==11 && i<2)
          mark(i==0?SealPrepareEnd:SealShaEnd,kCompute);
      }
      if (cycle) mark(T877Point(CycleBegin+1+op.block),kCompute);
      for (size_t i=1;i<8;i+=2)
        if(positions[TrialEdges+i]==ordinal) mark(T877Point(TrialEdges+i),kCompute);
      for (auto point:afterPoints) if (positions[point]==ordinal) mark(point,kCompute);
      if (plan[ordinal].op==17) {
        Require(slotIndex<kCandidateSlotCount,"T833_SLOT_RANGE");
        const auto where=sizeof(Completion)+slotIndex++*slotBytes;
        if (!snapshot) {
          Barrier(kCompute,VK_ACCESS_SHADER_WRITE_BIT,kTransfer,VK_ACCESS_TRANSFER_READ_BIT);
          copy(kWork,uint64_t(profile.ww)*4,where,kCandidateLatchBytes);
          copy(kControl,513*4,where+kCandidateLatchBytes,kCandidateErrorWords*4);
          Barrier(kTransfer,VK_ACCESS_TRANSFER_READ_BIT,kCompute,kShaderRW);
        }
      }
    }
    Barrier(kCompute,VK_ACCESS_SHADER_WRITE_BIT,kTransfer,VK_ACCESS_TRANSFER_READ_BIT);
    if (attempt.g13TailTiming && !source) mark(TransferReady,kTransfer);
    mark(BeforeTail,kCompute);
    copy(kControl,kColdTailWordOffset*4,0,sizeof(Completion));
    if (snapshot && slotIndex)
      copy(kWork,uint64_t(profile.ww)*4+kCandidateLatchBytes,sizeof(Completion),slotIndex*slotBytes);
    const auto fieldsAt=sizeof(Completion)+slotIndex*slotBytes;
    VkDeviceSize offset=fieldsAt;
    const std::array<size_t,7> captured{base,candidate,2,kWork,kControl,3,4};
    std::array<uint32_t,7> bytes{};
    for (size_t i=0;i<captured.size();++i) {
      bytes[i]=uint32_t(captured[i]==kWork ? archiveWorkBytes : buffers[captured[i]].bytes);
      if (!hot) copy(captured[i],0,offset,bytes[i]);
      offset+=bytes[i];
    }
    mark(BusinessCopiesEnd,VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
    Barrier(kTransfer,VK_ACCESS_TRANSFER_WRITE_BIT,VK_PIPELINE_STAGE_HOST_BIT,VK_ACCESS_HOST_READ_BIT);
    Check(vk.EndCommandBuffer(command),"vkEndCommandBuffer.T833");
    const auto recordEnd=pc::QpcTicks();
    const std::string prefix=source ? "t833Source" : "t833Quantum";
    fields << ",\"" << prefix << "Plan\":{\"logical\":" << std::size(plan) << ",\"dispatches\":" << issued
           << ",\"computeBarriers\":" << computeBarriers << ",\"copies\":" << copies
           << ",\"pipelineBinds\":" << pipelineBinds << ",\"descriptorBinds\":" << descriptorBinds
           << ",\"businessCopyBytes\":" << (hot ? fieldsAt : offset)
           << ",\"recordBeginQpc\":\"" << recordBegin << "\",\"recordEndQpc\":\"" << recordEnd << "\""
           << ",\"nonce\":\"" << nonce << "\",\"grantNs\":\"" << grant
           << "\",\"deadlineNs\":\"" << deadline << "\"}";
    if (commit==0 && attempt.t833RevokeAt=="before") attempt.t833Revoke();
    const auto boundary=[&]() -> const char * {
      if (!attempt.current()) return "WINDOW_REVOKED";
      if (!readPulse()) return clockFault ? clockFault : pc::CandidatePulseFreshness(lastPulse,pc::TimeNs());
      if (!pc::CandidatePollNs(pc::TimeNs(),deadline)) return "GRANT_DEADLINE";
      return nullptr;
    };
    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.commandBufferCount=1;
    submit.pCommandBuffers=&command;
    VkResult submitted=VK_NOT_READY;
    uint64_t submittedQpc=0,submitExit=0;
    pc::CandidateTiming timing;
    const auto reportWaits=[&] {
      if (!t877) return;
      const auto now=pc::QpcNs(timing.stopQpc,pc::QpcFrequency());
      std::ostringstream row;
      row << "{\"stage\":\"T877_ROOT_WAIT\",\"rootKind\":" << Quote(source ? "SOURCE" : "QUANTUM")
          << ",\"nonce\":\"" << nonce << "\",\"ticket\":\"" << attempt.ticket
          << "\",\"geometry\":\"" << attempt.geometry << "\",\"queue\":\"" << reinterpret_cast<uintptr_t>(queue)
          << "\",\"device\":\"" << reinterpret_cast<uintptr_t>(device)
          << "\",\"fence\":\"" << reinterpret_cast<uintptr_t>(fence)
          << "\",\"fenceGeneration\":\"" << t877FenceGeneration
          << "\",\"queryPool\":\"" << reinterpret_cast<uintptr_t>(t877Queries)
          << "\",\"writtenMask\":\"" << writtenMask()
          << "\",\"grantNs\":\"" << grant << "\",\"deadlineNs\":\"" << deadline
          << "\",\"recordBeginQpc\":\"" << recordBegin << "\",\"recordEndQpc\":\"" << recordEnd
          << "\",\"submitEnterQpc\":\"" << submittedQpc << "\",\"submitExitQpc\":\"" << submitExit
          << "\",\"submitVkResult\":" << int(submitted) << ",\"firstStop\":" << Quote(timing.stop)
          << ",\"stopQpc\":\"" << timing.stopQpc << "\",\"pulseSeq\":\"" << lastPulse.sequence
          << "\",\"callbackQpc\":\"" << lastPulse.callbackQpc << "\",\"receiptQpc\":\"" << lastPulse.receiptQpc
          << "\",\"observedPulseSeq\":\"" << observedPulse.sequence
          << "\",\"observedCallbackQpc\":\"" << observedPulse.callbackQpc
          << "\",\"observedReceiptQpc\":\"" << observedPulse.receiptQpc
          << "\",\"pulseAgeNs\":\"" << (lastPulse.sequence && now>=lastPulse.receiptNs ? now-lastPulse.receiptNs : 0)
          << "\",\"maxCallbackGapNs\":\"" << maxGap << "\",\"maxReceiptGapNs\":\"" << maxReceiptGap
          << "\",\"pollCount\":" << timing.count << ",\"pollsTruncated\":"
          << (timing.count>timing.polls.size() ? "true" : "false");
      const auto wait=[&](const pc::CandidateTiming::Wait &w) {
        row << "{\"enterQpc\":\"" << w.enterQpc << "\",\"exitQpc\":\"" << w.exitQpc
            << "\",\"timeoutNs\":" << w.timeoutNs << ",\"vkResult\":" << w.result << "}";
      };
      row << ",\"lastBusinessWait\":";
      if (timing.count) wait(timing.last); else row << "null";
      row << ",\"retireWait\":";
      if (timing.retire.enterQpc) wait(timing.retire); else row << "null";
      row << ",\"businessWaits\":[";
      for (size_t i=0;i<std::min(timing.count,timing.polls.size());++i) {
        if (i) row << ',';
        wait(timing.polls[i]);
      }
      publish(row.str()+"]}");
    };
    const auto bindInitialGuard=boundary();
    if (bindInitialGuard) timing.Stop(bindInitialGuard,pc::QpcTicks());
    if (!timing.stop && !attempt.acceptPointer(0,[&] {
          if (pc::TimeNs()>=deadline) { timing.Stop("GRANT_DEADLINE",pc::QpcTicks()); return; }
          submittedQpc=pc::QpcTicks();
          submitted=vk.QueueSubmit(queue,1,&submit,fence);
          submitExit=pc::QpcTicks();
        })) timing.Stop("WINDOW_REVOKED",pc::QpcTicks());
    if (timing.stop) {
      fields << ",\"" << prefix << "Edge\":" << Quote(std::strcmp(timing.stop,"WINDOW_REVOKED")==0 ?
                                                        "REVOKED_BEFORE_SUBMIT" : timing.stop);
      reportWaits();
      reportGpu(false);
      return false;
    }
    if (submitted!=VK_SUCCESS) {
      timing.Stop("SUBMIT_RESULT",submitExit);
      reportWaits();
      reportGpu(false);
    }
    Check(submitted,"vkQueueSubmit.T833");
    inFlight=true;
    if (commit==0 && attempt.t833RevokeAt=="after") attempt.t833Revoke();
    stage=source ? "T833_SOURCE_BUSINESS_WAIT" : "T833_QUANTUM_BUSINESS_WAIT";
    VkResult waited=VK_NOT_READY;
    bool timely=false;
    for (;;) {
      if (const auto why=boundary()) { timing.Stop(why,pc::QpcTicks()); break; }
      const auto timeout=pc::CandidatePollNs(pc::TimeNs(),deadline);
      if (!timeout) { timing.Stop("GRANT_DEADLINE",pc::QpcTicks()); break; }
      const auto enter=pc::QpcTicks();
      waited=vk.WaitForFences(device,1,&fence,VK_TRUE,timeout);
      const auto exit=pc::QpcTicks();
      timing.AddPoll({enter,exit,timeout,int(waited)});
      if (waited==VK_TIMEOUT) continue;
      timing.Stop("FENCE_RESULT",exit);
      timely=waited==VK_SUCCESS && pc::QpcNs(exit,pc::QpcFrequency())<=deadline;
      break;
    }
    if (waited!=VK_SUCCESS && waited!=VK_ERROR_DEVICE_LOST) {
      stage="T833_ROOT_RETIRE_WAIT";
      timing.retire.enterQpc=pc::QpcTicks();
      timing.retire.timeoutNs=kCandidateJobNs;
      waited=vk.WaitForFences(device,1,&fence,VK_TRUE,timing.retire.timeoutNs);
      timing.retire.exitQpc=pc::QpcTicks();
      timing.retire.result=int(waited);
    }
    if (diagnosticRaw && waited==VK_TIMEOUT && attempt.current()) {
      const auto original=fence;
      const auto entered=pc::QpcTicks();
      waited=vk.WaitForFences(device,1,&original,VK_TRUE,5000000000ull);
      publish("{\"stage\":\"T955_ORIGINAL_RETIRE\",\"nonce\":\""+std::to_string(nonce)+
              "\",\"fence\":\""+std::to_string(reinterpret_cast<uintptr_t>(original))+
              "\",\"generation\":\""+std::to_string(t877FenceGeneration)+"\",\"waitEnterQpc\":\""+
              std::to_string(entered)+"\",\"waitExitQpc\":\""+std::to_string(pc::QpcTicks())+
              "\",\"vkResult\":"+std::to_string(int(waited))+",\"acceptedRoot\":null}");
    }
    if (waited==VK_SUCCESS || waited==VK_ERROR_DEVICE_LOST) inFlight=false;
    fields << ",\"" << prefix << "Fence\":" << int(waited);
    fields << ",\"" << prefix << "Submit\":" << int(submitted)
           << ",\"" << prefix << "SubmitQpc\":\"" << submittedQpc << "\""
           << ",\"" << prefix << "FenceQpc\":\"" << pc::QpcTicks() << "\""
           << ",\"" << prefix << "FenceObject\":\"" << reinterpret_cast<uintptr_t>(fence) << "\"";
    if (t877 && !timely) {
      t877Late={device,queue,fence,t877Queries,readQueries,nonce,id.run,t877FenceGeneration,
                writtenMask(),timing.stopQpc,hardware.timestampBits,
                hardware.properties.limits.timestampPeriod,timing.stop,source,false,inFlight ? slotIndex : 0};
      if (!inFlight) {
        ReportLateCandidate(waited,VK_NOT_READY,0,pc::QpcTicks());
        t877Late={};
      }
    }
    if (waited!=VK_SUCCESS) {
      if (t877 && inFlight) t877PendingNonce=nonce;
      reportWaits();
      reportGpu(false);
    }
    Check(waited,"vkWaitForFences.T833_RETIRE");
    stage="T833_ROOT_TAIL_READBACK";
    const auto mapEnter=pc::QpcTicks();
    void *mapped=nullptr;
    const auto mapResult=vk.MapMemory(device,staging.memory,0,VK_WHOLE_SIZE,0,&mapped);
    if (mapResult!=VK_SUCCESS) {
      reportWaits();
      reportGpu(mapResult!=VK_ERROR_DEVICE_LOST);
    }
    Check(mapResult,"vkMapMemory.T833");
    const auto atom=hardware.properties.limits.nonCoherentAtomSize;
    const auto hotRange=(fieldsAt+atom-1)/atom*atom;
    const VkMappedMemoryRange range{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,nullptr,staging.memory,0,
                                    hot && hotRange<=staging.bytes ? hotRange : VK_WHOLE_SIZE};
    const auto invalidate=(staging.flags&VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) ? VK_SUCCESS :
                          vk.InvalidateMappedMemoryRanges(device,1,&range);
    std::vector<uint8_t> raw(size_t(hot ? fieldsAt : offset),uint8_t(0));
    if (invalidate==VK_SUCCESS) std::memcpy(raw.data(),mapped,raw.size());
    vk.UnmapMemory(device,staging.memory);
    const auto mapExit=pc::QpcTicks();
    if (invalidate!=VK_SUCCESS) {
      reportWaits();
      reportGpu(invalidate!=VK_ERROR_DEVICE_LOST);
    }
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
    const auto checkEnter=pc::QpcTicks();
    const char *acceptReject=nullptr;
    if (!timely) acceptReject=timing.last.result==VK_SUCCESS ? "LATE_FENCE_SIGNAL" : timing.stop;
    else if ((acceptReject=boundary())) {}
    else if (!latchOK || !T833TailMatches(tail,permit,quantumTick,sourceSeq)) acceptReject="TAIL_OR_LATCH_REJECTED";
    else if (!attempt.acceptPointer(0,[&] {
        const auto observed=pc::TimeNs();
        if (observed>deadline) acceptReject="GRANT_DEADLINE";
        else if ((acceptReject=pc::CandidatePulseFreshness(lastPulse,observed))) {}
        else {
          matchedNs=observed;
          if (!privateProject) acceptedNonce=nonce;
        }
      })) acceptReject="WINDOW_REVOKED";
    const bool matched=matchedNs!=0;
    const bool accepted=matched && !privateProject;
    const std::string_view reason=acceptReject ? acceptReject : "";
    const uint32_t nativeError=matched ? 0u : reason=="WINDOW_REVOKED" ? 11u :
        reason=="GRANT_DEADLINE" || reason=="LATE_FENCE_SIGNAL" ? 9u : tail[7] ? tail[7] : 2u;
    if (accepted && !source) ++consumed;
    reportWaits();
    if (t877) {
      std::ostringstream row;
      row << "{\"stage\":\"T877_ROOT_ACCEPT\",\"nonce\":\"" << nonce
          << "\",\"mapEnterQpc\":\"" << mapEnter << "\",\"mapExitQpc\":\"" << mapExit
          << "\",\"checkEnterQpc\":\"" << checkEnter << "\",\"checkExitQpc\":\"" << pc::QpcTicks()
          << "\",\"acceptNs\":\"" << matchedNs << "\",\"acceptedRoot\":"
          << (accepted ? Quote(std::to_string(nonce)) : "null")
          << ",\"reason\":" << (acceptReject ? Quote(acceptReject) : "null")
          << ",\"latchOK\":" << (latchOK ? "true" : "false") << ",\"nativeError\":" << nativeError << '}';
      publish(row.str());
    }
    if (t877 && timely && !matched) {
      t877Late={device,queue,fence,t877Queries,readQueries,nonce,id.run,t877FenceGeneration,
                writtenMask(),timing.stopQpc,hardware.timestampBits,
                hardware.properties.limits.timestampPeriod,timing.stop,source};
      ReportLateCandidate(waited,VK_NOT_READY,0,pc::QpcTicks());
      t877Late={};
    }
    if (hot) {
      const auto evidenceIndex=source ? 0 : 1;
      evidence[evidenceIndex]=std::move(raw);
      evidenceIsHot[evidenceIndex]=true;
      const auto &hotPrefix=evidence[evidenceIndex];
      fields << ",\"" << prefix << "HotEvidence\":{\"nonce\":\"" << nonce << "\",\"fileBytes\":" << hotPrefix.size()
             << ",\"sha256\":" << Quote(Hex(Hash(hotPrefix.data(),hotPrefix.size()).data()))
             << ",\"acceptedRoot\":" << (accepted ? Quote(std::to_string(nonce)) : "null") << "}";
      const bool privateLate=diagnosticRaw && !timely && waited==VK_SUCCESS;
      if ((!matched && !privateLate) || !attempt.current()) {
        fields << ",\"" << prefix << "ColdCapture\":\"NO_SAFE_READ_WINDOW\"";
        reportGpu(true);
        coldFailure=matched;
        return false;
      }
      // No next root is recorded while these original buffer ranges are pinned
      // on the owner. The copy has its own submission and fence after acceptance.
      stage="T877_COLD_COPY";
      if(privateLate) fields << ",\"t955PrivateLateRaw\":true";
      Check(reset(device,commandPool,0),"vkResetCommandPool.T877_COLD");
      vk.DestroyFence(device,fence,nullptr);
      fence=VK_NULL_HANDLE;
      Check(vk.CreateFence(device,&fenceInfo,nullptr,&fence),"vkCreateFence.T877_COLD");
      ++t877FenceGeneration;
      Check(vk.BeginCommandBuffer(command,&begin),"vkBeginCommandBuffer.T877_COLD");
      Barrier(kCompute|kTransfer,VK_ACCESS_SHADER_WRITE_BIT|VK_ACCESS_TRANSFER_WRITE_BIT,
              kTransfer,VK_ACCESS_TRANSFER_READ_BIT|VK_ACCESS_TRANSFER_WRITE_BIT);
      mark(ColdCopiesBegin,VK_PIPELINE_STAGE_TRANSFER_BIT);
      VkDeviceSize at=fieldsAt;
      for (size_t j=0;j<captured.size();++j) {
        const VkBufferCopy region{0,at,bytes[j]};
        vk.CmdCopyBuffer(command,buffers[captured[j]].handle,staging.handle,1,&region);
        at+=bytes[j];
      }
      mark(ColdCopiesEnd,VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
      Barrier(kTransfer,VK_ACCESS_TRANSFER_WRITE_BIT,VK_PIPELINE_STAGE_HOST_BIT,VK_ACCESS_HOST_READ_BIT);
      Check(vk.EndCommandBuffer(command),"vkEndCommandBuffer.T877_COLD");
      if (!attempt.current()) {
        fields << ",\"" << prefix << "ColdCapture\":\"NO_SAFE_READ_WINDOW\"";
        // ColdCopiesEnd was recorded, but the cold command buffer was not submitted.
        written[ColdCopiesBegin]=written[ColdCopiesEnd]=false;
        reportGpu(true);
        coldFailure=true;
        return false;
      }
      const auto coldSubmitEnter=pc::QpcTicks();
      const auto queued=vk.QueueSubmit(queue,1,&submit,fence);
      const auto coldSubmitExit=pc::QpcTicks();
      VkResult retired=VK_NOT_READY;
      uint64_t coldWaitEnter=0,coldWaitExit=0;
      if (queued==VK_SUCCESS) {
        inFlight=true;
        coldWaitEnter=pc::QpcTicks();
        retired=vk.WaitForFences(device,1,&fence,VK_TRUE,kCandidateJobNs);
        coldWaitExit=pc::QpcTicks();
        if (retired==VK_SUCCESS || retired==VK_ERROR_DEVICE_LOST) inFlight=false;
      } else {
        written[ColdCopiesBegin]=written[ColdCopiesEnd]=false;
      }
      std::ostringstream cold;
      cold << "{\"stage\":\"T877_COLD_COPY\",\"nonce\":\"" << nonce
           << "\",\"fence\":\"" << reinterpret_cast<uintptr_t>(fence) << "\",\"copyBytes\":" << offset-fieldsAt
           << ",\"copyCount\":" << captured.size() << ",\"submitEnterQpc\":\"" << coldSubmitEnter
           << "\",\"submitExitQpc\":\"" << coldSubmitExit << "\",\"submitVkResult\":" << int(queued)
           << ",\"waitEnterQpc\":\"" << coldWaitEnter << "\",\"waitExitQpc\":\"" << coldWaitExit
           << "\",\"waitTimeoutNs\":" << (coldWaitEnter ? kCandidateJobNs : 0)
           << ",\"fenceVkResult\":" << int(retired) << '}';
      publish(cold.str());
      if (queued!=VK_SUCCESS || retired!=VK_SUCCESS) {
        if (inFlight) {
          t877PendingNonce=nonce;
          const uint64_t coldMask=(uint64_t{1}<<ColdCopiesBegin)|(uint64_t{1}<<ColdCopiesEnd);
          t877Late={device,queue,fence,t877Queries,readQueries,nonce,id.run,t877FenceGeneration,
                    writtenMask()&coldMask,coldWaitExit,hardware.timestampBits,
                    hardware.properties.limits.timestampPeriod,"COLD_COPY_FENCE",source,true};
        }
        reportGpu(queued!=VK_ERROR_DEVICE_LOST && retired!=VK_ERROR_DEVICE_LOST);
        coldFailure=true;
        return false;
      }
      reportGpu(true,true);
      raw.resize(size_t(offset));
      mapped=nullptr;
      Check(vk.MapMemory(device,staging.memory,0,VK_WHOLE_SIZE,0,&mapped),"vkMapMemory.T877_COLD");
      const VkMappedMemoryRange coldRange{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,nullptr,staging.memory,0,VK_WHOLE_SIZE};
      const auto coldInvalidate=(staging.flags&VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) ? VK_SUCCESS :
                                vk.InvalidateMappedMemoryRanges(device,1,&coldRange);
      if (coldInvalidate==VK_SUCCESS) std::memcpy(raw.data(),mapped,raw.size());
      vk.UnmapMemory(device,staging.memory);
      Check(coldInvalidate,"vkInvalidateMappedMemoryRanges.T877_COLD");
      Require(std::equal(hotPrefix.begin(),hotPrefix.end(),raw.begin()),"T877_HOT_PREFIX_CHANGED");
    } else {
      reportGpu(true);
    }
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
           << (accepted ? Quote(std::to_string(nonce)) : "null");
    if(diagnosticRaw) {
      fields << ",\"completionSHA\":" << Quote(Hex(Hash(raw.data(),sizeof(Completion)).data()))
             << ",\"segmentSHA\":[";
      size_t at=fieldsAt;
      for(size_t j=0;j<bytes.size();++j) {
        if(j) fields << ',';
        fields << Quote(Hex(Hash(raw.data()+at,bytes[j]).data()));
        at+=bytes[j];
      }
      fields << ']';
    }
    fields << '}';
    evidence[source ? 0 : 1]=std::move(raw);
    evidenceIsHot[source ? 0 : 1]=false;
    return matched;
  };
  const auto exportEvidence=[&](uint32_t tick=0) {
    for (size_t i=0;i<evidence.size();++i) {
      auto &raw=evidence[i];
      if (raw.empty()) continue;
      auto path=attempt.t833CandidateFile+(i==0 ? L".source" : L".quantum");
      if (i && tick) path+=L"."+std::to_wstring(tick);
      if (evidenceIsHot[i]) path+=L".hot";
      const auto writeEnter=pc::QpcTicks();
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
      if (t877) {
        std::ostringstream row;
        row << "{\"stage\":\"T877_FILE_WRITTEN\",\"source\":" << (i==0 ? "true" : "false")
            << ",\"seriesStep\":" << tick << ",\"bytes\":" << raw.size()
            << ",\"enterQpc\":\"" << writeEnter << "\",\"exitQpc\":\"" << pc::QpcTicks()
            << "\",\"sha256\":" << Quote(Hex(Hash(raw.data(),raw.size()).data())) << '}';
        publish(row.str());
      }
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
      try {
        quantum=root(false,k_quantum,base,1-base,4-base,tick,0,tick);
      } catch (...) {
        exportEvidence(tick+1);
        throw;
      }
      exportEvidence(tick+1);
      if (!attempt.t833RevokeAt.empty() && !attempt.current() && !quantum)
        return Receipt("T833_CANDIDATE_REVOKED","");
      const bool kept=publish(Receipt(quantum ? "T833_SERIES_ROOT" : "FAILED",
                                     quantum ? "" : "T833_F03_ROOT_NOT_ACCEPTED"));
      if (!quantum || !kept)
        return Receipt("FAILED",coldFailure ? "T877_COLD_EVIDENCE_FAILED" : "T833_F03_SERIES_INCOMPLETE");
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
  if (coldFailure) return Receipt("FAILED","T877_COLD_EVIDENCE_FAILED");
  if (!attempt.t833RevokeAt.empty() && !attempt.current() && !source && !quantum)
    return Receipt("T833_CANDIDATE_REVOKED","");
  if (sourceFirst && !source) return Receipt("FAILED","T833_SOURCE_NOT_ACCEPTED");
  if (!quantum) return Receipt("FAILED","T833_QUANTUM_NOT_ACCEPTED");
  return Receipt(privateProject ? "T833_PRIVATE_PROJECT_CAPTURED" : "T833_CANDIDATE_CAPTURED","");
}
