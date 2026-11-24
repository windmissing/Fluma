#include "host_device.h"
#include "../../../entry/src/main/cpp/gpu_resident/g11vis/parent_adapter.h"
#include "../../../entry/src/main/cpp/gpu_resident/g9dm/input_gate.h"
#include "../../../entry/src/main/cpp/gpu_resident/g9spf/input_gate.h"
#include "../../../entry/src/main/cpp/gpu_resident/g9dm/progress_clock_v3.h"
#include "../../../entry/src/main/cpp/gpu_resident/angles_generated.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string_view>

namespace grs::pc::sand {
#ifdef PC_SAND_G11_R1
PresentedMapping PresentR1(HostDevice &,const std::filesystem::path &,const VulkanOwner::AcceptedProof &,
                          const R1Scene &);
#endif
namespace {
using namespace g11vis;
#ifdef PC_SAND_G11_R1
constexpr ParentKind kind=ParentKind::R1;
constexpr auto &visual=kVisualResources[0];
constexpr char family[]="R1";
using PhysicalPermit=g9spf::Permit;
using Gate=g9spf::InputGate;
using PhysicalConfig=g9spf::Config;
#else
constexpr ParentKind kind=ParentKind::DM;
constexpr char family[]="DM2";
using PhysicalPermit=g9dm::Permit;
using Gate=g9dm::InputGate;
using PhysicalConfig=g9dm::Config;
#endif
using Clock=g9dm::ProgressClockV3;
constexpr auto storageUsage=VK_BUFFER_USAGE_STORAGE_BUFFER_BIT|VK_BUFFER_USAGE_TRANSFER_SRC_BIT|
                            VK_BUFFER_USAGE_TRANSFER_DST_BIT|VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

struct Options {
  std::filesystem::path modules, source, archive, witness;
  uint32_t steps=0;
  int32_t angle=0;
  bool full=false,present=false,sourceTimestamps=false,sourceDetail=false,sourcePrewarm=false;
};

Options Args(int argc,wchar_t **argv) {
  RequireHost(argc>=3 && std::wstring_view(argv[1])==L"--modules","REQUIRE_MODULES");
  Options out;
  out.modules=argv[2];
  const auto num=[](const wchar_t *value) {
    std::wstring text(value);
    size_t end=0;
    auto parsed=std::stol(text,&end);
    RequireHost(end==text.size(),"INVALID_NUMBER");
    return parsed;
  };
  for (int i=3;i<argc;++i) {
    const std::wstring_view key=argv[i];
    if (key==L"--full") { RequireHost(!out.full,"DUPLICATE_FULL"); out.full=true; }
    else if (key==L"--present") { RequireHost(!out.present,"DUPLICATE_PRESENT"); out.present=true; }
    else if (key==L"--source-timestamps") {
      RequireHost(!out.sourceTimestamps,"DUPLICATE_SOURCE_TIMESTAMPS");
      out.sourceTimestamps=true;
    }
    else if (key==L"--source-timestamps-detail") out.sourceDetail=true;
    else if (key==L"--source-prewarm") out.sourcePrewarm=true;
    else {
      RequireHost(i+1<argc,"MISSING_ARGUMENT");
      const auto *value=argv[++i];
      if (key==L"--source") out.source=value;
      else if (key==L"--archive") out.archive=value;
      else if (key==L"--witness") out.witness=value;
      else if (key==L"--steps") {
        const auto count=num(value);
        RequireHost(count>=0 && count<=120,"STEP_RANGE");
        out.steps=uint32_t(count);
      } else if (key==L"--angle-index") {
        const auto angle=num(value);
        RequireHost(angle>=-7680 && angle<=7680,"ANGLE_OUT_OF_RANGE");
        out.angle=int32_t(angle);
      } else RequireHost(false,"UNKNOWN_ARGUMENT");
    }
  }
#ifdef PC_SAND_G11_R1
  RequireHost(!out.full && out.archive.empty() &&
             (out.witness.empty() || out.steps) && (!out.present || out.steps || !out.source.empty()),
             "R1_CAPABILITIES");
#else
  RequireHost(out.witness.empty() && !out.present &&
              (!out.full || (out.steps && !out.archive.empty())) &&
              (out.archive.empty() || out.steps || !out.source.empty()),"DM2_CAPABILITIES");
#endif
  RequireHost(!(out.sourceTimestamps || out.sourceDetail || out.sourcePrewarm) || !out.source.empty(),"SOURCE_DIAGNOSTIC_NEEDS_SOURCE");
  return out;
}

std::string HexBytes(const uint8_t *bytes,size_t count) {
  constexpr char hex[]="0123456789abcdef";
  std::string result;
  result.reserve(count*2);
  for (size_t i=0;i<count;++i) {
    result+=hex[bytes[i]>>4];
    result+=hex[bytes[i]&15];
  }
  return result;
}

Input128 ReadSource(const std::filesystem::path &path) {
  Input128 raw{};
  if (path.empty()) return raw;
  std::ifstream in(path,std::ios::binary);
  RequireHost(bool(in.read(reinterpret_cast<char*>(raw.data()),sizeof(raw))) &&
              in.peek()==std::char_traits<char>::eof() && raw[40]==1 &&
              accept647::Pair(raw.data(),41)!=0,"SOURCE_NEEDS_G9_INPUT128_BODY");
  std::fill(raw.begin(),raw.begin()+40,0);
  return raw;
}

Slice Subspan(Slice source,size_t offset,size_t bytes) {
  RequireHost(offset<=source.bytes && bytes<=source.bytes-offset,"SLICE_RANGE");
  source.offset+=offset;
  source.bytes=bytes;
  return source;
}

Common Uniforms(const ParentLayout &l,const PhysicalPermit &p,int32_t angle,
                const std::array<uint32_t,3> &visualBanks,uint64_t visualGeneration,uint64_t visualTicks) {
  Common data{};
  const auto wide=[&](size_t at,uint64_t value) {
    PutCommonWord(data,at,uint32_t(value));
    PutCommonWord(data,at+4,uint32_t(value>>32));
  };
#define U(name,value) PutCommonWord(data,k_##name,uint32_t(value))
#define W(name,value) wide(k_##name,value)
  U(uK,l.k); U(uStateWords,l.stateBytes/4); U(uColumn,l.state[3].begin/4);
  U(uBed,l.state[4].begin/4); U(uFlow,l.state[5].begin/4); U(uPacket,l.state[6].begin/4);
  U(uSources,l.state[7].begin/4);
  U(uFaces,l.work[0].begin/4); U(uColumnStage,l.work[1].begin/4); U(uHamilton,l.work[2].begin/4);
  U(uOriginAudit,l.work[3].begin/4); U(uInput,l.work[5].begin/4); U(uSummary,l.work[6].begin/4);
  U(uControls,l.work[7].begin/4); U(uCompletion,l.work[8].begin/4); U(uConfig,l.work[9].begin/4);
  U(uWideLedger,l.work[11].begin/4); U(uContacts,l.work[12].begin/4); U(uStageDiagnostic,l.work[14].begin/4);
  U(uRootKind,p.kind); U(uCandidateBank,p.candidateBank); U(uBaseBank,p.baseBank); U(uArena,p.arena);
  W(uNonce,p.nonce); W(uRun,p.identity.run); W(uOwner,p.identity.owner); W(uContext,p.identity.context);
  W(uSurface,p.identity.surface); W(uGeometry,p.geometry); W(uConfigId,p.identity.config);
  W(uInputGeneration,p.identity.inputEpoch); W(uSession,p.identity.pageSession); W(uPlan,p.plan);
  W(uBaseCommit,p.baseCommit); W(uArenaGeneration,p.arenaGeneration);
  U(uEventCapacity,l.eventCapacity); U(uPathCapacity,l.pathCapacity); U(uPathWord,l.eventCapacity*64);
  U(uAcceptedAngle,angle);
  const auto at=size_t(angle+7680)*4;
  U(uAngleSin,kAngleWords[at+1]); U(uAngleCos,kAngleWords[at+2]);
#ifdef PC_SAND_G11_R1
  U(uRequiredPasses,p.kind==1 ? 19 : 2543); U(uRequiredAudits,0xcffff);
  U(uVisMarkerCurrent,visual.visual[visualBanks[0]]/4);
  U(uVisMarkerActual,visual.visual[visualBanks[1]]/4);
  U(uVisMarkerReplay,visual.visual[visualBanks[2]]/4);
  U(uVisHeaderCurrent,visualBanks[0]); U(uVisHeaderActual,visualBanks[1]); U(uVisHeaderReplay,visualBanks[2]);
  U(uVisGlyph,visual.visual[kSegGlyph]/4); U(uVisPlan,visual.visual[kSegMaterialPlan]/4);
  U(uVisControl,visual.visual[kSegControl]/4); U(uVisMaps,visual.visual[kSegFourMaps]/4); U(uVisV,visual.v);
  W(uVisBaseTicks,visualTicks); W(uVisBaseGeneration,visualGeneration);
  W(uVisTicks,visualTicks+(p.kind==2 ? 256 : 0));
#else
  (void)visualBanks; (void)visualGeneration; (void)visualTicks;
  U(uRequiredPasses,p.kind==1 ? 12 : 2404); U(uRequiredAudits,p.kind==1 ? 0xffff : 0x3ffff);
  U(uDmBase,(l.workBytes-24576)/4);
#endif
#undef U
#undef W
  return data;
}

class PulseClock {
 public:
  Clock clock;
  g9dm::RunControl run;
  g9dm::PlanGrantV3 grant;

  PulseClock(const Scope &scope,int32_t angle) {
    g9dm::ControlIdentity id;
    id.run=scope.run; id.owner=scope.owner; id.context=scope.context;
    id.inputEpoch=scope.inputEpoch; id.pageSession=scope.pageSession; id.permission=scope.permission;
    id.window=scope.window; id.config=scope.config; id.surface=scope.surface;
    RequireHost(clock.ResetRunLocked({id},run)==g9dm::ControlResult::OK,"CLOCK_INIT");
    clock.ready=clock.uiFresh=clock.gravityFresh=true;
    clock.target=angle;
    worker_=std::thread([&,id] {
      uint64_t seq=0;
      auto target=std::chrono::steady_clock::now();
      while (!stop_.load(std::memory_order_relaxed)) {
        const auto callback=HostDevice::Now();
        {
          std::lock_guard lock(mutex_);
          const auto receipt=HostDevice::Now();
          clock.PulseLocked({id,++seq,callback},receipt);
        }
        target=std::max(target+std::chrono::milliseconds(12),std::chrono::steady_clock::now());
        std::this_thread::sleep_until(target);
      }
    });
  }
  ~PulseClock() { stop_=true; worker_.join(); }
  struct FreshSample { uint64_t observed=0,sequence=0,callback=0,receipt=0; unsigned fault=0; bool fresh=false; };
  bool Fresh(FreshSample *sample=nullptr) {
    std::lock_guard lock(mutex_);
    const auto observed=HostDevice::Now();
    const bool fresh=clock.Fresh(observed);
    if (sample) *sample={observed,clock.counters.pulseSequence,clock.counters.uiNs,clock.counters.receiveNs,
                        unsigned(clock.fault),fresh};
    return fresh;
  }
  unsigned Fault() {
    std::lock_guard lock(mutex_);
    return unsigned(clock.fault);
  }
  bool BeginRoot(bool source) {
    std::lock_guard lock(mutex_);
    if (clock.fault!=g9dm::ControlResult::OK) return false;
    if (!run.active && !clock.ClaimLocked(HostDevice::Now(),source,run,grant)) return false;
    if (!source && !clock.activeRemaining) return false;
    run.jobLive=clock.jobLive=true;
    run.workspaceRetired=false;
    clock.activeRootKind=source ? 1 : 2;
    return true;
  }
  template<class Accept>
  std::optional<VulkanOwner::AcceptedProof> Accept(bool source,Accept commit,std::array<uint32_t,256> &quota) {
    std::lock_guard lock(mutex_);
    auto result=commit([&] { return clock.Fresh(HostDevice::Now()); });
    if (!result) return {};
    if (!source && clock.activeRemaining) { --clock.activeRemaining; ++clock.counters.accepted; }
    run.jobLive=clock.jobLive=false;
    run.workspaceRetired=true;
    clock.activeRootKind=0;
    if (!clock.activeRemaining) clock.RetireSourcePlanLocked(grant.id,run);
    RequireHost(clock.Balanced(),"CLOCK_QUOTA_BALANCE");
    quota=clock.SnapshotLocked(HostDevice::Now(),false,run);
    return result;
  }
  bool Close() {
    std::lock_guard lock(mutex_);
    clock.RevokeLocked({g9dm::ControlResult::STALE_REJECTED});
    return clock.Balanced();
  }
 private:
  std::mutex mutex_;
  std::atomic<bool> stop_{false};
  std::thread worker_;
};

void SourceTimeline(const HostDevice &gpu,const VulkanOwner &owner,const VulkanOwner::RootIdentity &id,
                    const StorageSlots &storage,const Slice &common,const Slice &tail,
                    uint64_t grant,uint64_t deadline,uint64_t frozen,uint64_t expected,
                    const PulseClock::FreshSample &pulse,const VulkanOwner::RootReceipt &raw,
                    const VulkanOwner::AcceptTiming &gate,uint64_t promoteEnter,uint64_t promoteExit,bool accepted) {
  const auto &t=raw.timing;
  std::ostringstream row;
  row << "{\"stage\":\"ROOT_TIMELINE\",\"family\":\"" << family << "\",\"rootKind\":\"SOURCE\",\"run\":\""
      << id.run << "\",\"nonce\":\"" << id.nonce << "\",\"plan\":\"" << id.plan
      << "\",\"roleSetSHA\":\"" << owner.RoleSetSHA()
      << "\",\"device\":\"" << reinterpret_cast<uintptr_t>(gpu.device)
      << "\",\"queue\":\"" << reinterpret_cast<uintptr_t>(gpu.queue)
      << "\",\"fence\":\"" << reinterpret_cast<uintptr_t>(raw.fence) << "\",\"fenceGeneration\":\"" << raw.generation
      << "\",\"grantNs\":\"" << grant << "\",\"deadlineNs\":\"" << deadline
      << "\",\"inputFrozenNs\":\"" << frozen << "\",\"expectedReadyNs\":\"" << expected
      << "\",\"recordEnterNs\":\"" << t.recordEnter << "\",\"recordExitNs\":\"" << t.recordExit
      << "\",\"descriptorPrewarmed\":" << (t.descriptorPrewarmed ? "true" : "false")
      << ",\"descriptorRecordNs\":" << t.descriptorNs << ",\"commandResetNs\":" << t.commandResetNs
      << ",\"uploadRecordNs\":" << t.uploadRecordNs << ",\"dispatchRecordNs\":" << t.dispatchRecordNs
      << ",\"copyRecordNs\":" << t.copyRecordNs << ",\"endCommandNs\":" << t.endCommandNs
      << ",\"submitEnterNs\":\"" << t.submitEnter << "\",\"submitExitNs\":\"" << t.submitExit
      << "\",\"submitVkResult\":" << int(t.submit) << ",\"stopNs\":\"" << t.stopNs
      << "\",\"firstStop\":\"" << t.stop << "\",\"clock\":{\"observedNs\":\"" << pulse.observed
      << "\",\"seq\":\"" << pulse.sequence << "\",\"callbackNs\":\"" << pulse.callback
      << "\",\"receiptNs\":\"" << pulse.receipt << "\",\"ageNs\":"
      << (pulse.receipt && pulse.observed>=pulse.receipt ? std::to_string(pulse.observed-pulse.receipt) : "null")
      << ",\"fresh\":" << (pulse.fresh ? "true" : "false") << ",\"fault\":" << pulse.fault
      << "},\"pollCount\":" << t.calls << ",\"polls\":[";
  const auto wait=[&](const VulkanOwner::RootReceipt::Timeline::Wait &w) {
    row << "{\"enterNs\":\"" << w.enter << "\",\"exitNs\":\"" << w.exit << "\",\"timeoutNs\":\""
        << w.timeout << "\",\"vkResult\":" << int(w.result) << '}';
  };
  for (size_t i=0;i<std::min(t.calls,t.waits.size());++i) {
    if (i) row << ',';
    wait(t.waits[i]);
  }
  row << "],\"lastPoll\":";
  if (t.calls) wait(t.lastWait); else row << "null";
  row << ",\"readbackEnterNs\":\"" << t.readEnter << "\",\"readbackExitNs\":\"" << t.readExit
      << "\",\"readbackFenceVkResult\":" << int(t.readFence)
      << ",\"invalidateVkResult\":" << int(t.invalidate)
      << ",\"gateEnterNs\":\"" << gate.gateEnter << "\",\"gateExitNs\":\"" << gate.gateExit
      << "\",\"gateResult\":\"" << gate.result << "\",\"promoteEnterNs\":\"" << promoteEnter
      << "\",\"promoteExitNs\":\"" << promoteExit << "\",\"acceptedRoot\":"
      << (accepted ? std::to_string(id.nonce) : "null") << ",\"ranges\":[";
  const auto range=[&](const Slice &s) {
    row << "{\"buffer\":\"" << reinterpret_cast<uintptr_t>(s.buffer) << "\",\"generation\":\"" << s.generation
        << "\",\"offset\":" << s.offset << ",\"bytes\":" << s.bytes << '}';
  };
  for (size_t i=0;i<storage.size();++i) {
    if (i) row << ',';
    range(storage[i]);
  }
  row << "],\"common\":"; range(common);
  row << ",\"staging\":"; range(tail);
  std::cout << row.str() << "}\n";
}

void WriteWords(const std::filesystem::path &path,const uint32_t *words,size_t bytes) {
  RequireHost(!std::filesystem::exists(path),"DESTINATION_EXISTS");
  std::ofstream out(path,std::ios::binary);
  out.write(reinterpret_cast<const char*>(words),std::streamsize(bytes));
  RequireHost(bool(out),"WRITE_ORIGINAL");
}

#ifndef PC_SAND_G11_R1
std::string ArchiveSHA(const std::filesystem::path &path) {
  std::ifstream stream(path,std::ios::binary);
  RequireHost(bool(stream),"READ_CLOSED_ARCHIVE");
  p011gpu::Sha256 hash;
  std::vector<uint8_t> block(64*1024);
  while (stream) {
    stream.read(reinterpret_cast<char*>(block.data()),std::streamsize(block.size()));
    if (stream.gcount()>0) hash.update(block.data(),size_t(stream.gcount()));
  }
  RequireHost(stream.eof() && !stream.bad(),"HASH_CLOSED_ARCHIVE");
  const auto bytes=hash.finish();
  Digest digest{};
  std::memcpy(digest.data(),bytes.data(),bytes.size());
  return HexDigest(digest);
}
#endif

int Run(const Options &opt) {
  const auto body=ReadSource(opt.source);
  const auto parent=ParentAdapter::Research(kind,192,opt.full);
  RequireHost(parent.has_value(),"FROZEN_K192_PROFILE");
  const auto &l=parent->Layout();
  HostDevice gpu;
  gpu.Open(opt.present);
  VulkanOwner owner;
  RequireVk(owner.Open({gpu.device,gpu.queue,gpu.family,gpu.deviceProc,gpu.properties.limits,
                        gpu.timestampBits,opt.sourceTimestamps || opt.sourceDetail,opt.sourceDetail},opt.modules),
            "VULKAN_ROLE_PIPELINES");
  Scope scope{HostDevice::Now(),GetCurrentThreadId(),reinterpret_cast<uintptr_t>(gpu.device),
              1,1,1,gpu.window ? reinterpret_cast<uintptr_t>(gpu.window) : 1,1,
              gpu.surface ? reinterpret_cast<uintptr_t>(gpu.surface) : 1};
  PhysicalPermit permit;
  permit.identity={scope.run,scope.owner,scope.context,scope.inputEpoch,scope.pageSession,scope.permission,
                   scope.window,scope.config,scope.surface};
  permit.geometry=1;
  permit.candidateBank=2;
  permit.parameterSha=parent->ParameterHash();
  permit.layoutSha=parent->PhysicalLayoutHash();
  permit.shaderSha=HexDigest(owner.RoleSetSHA());
  permit.kind=1;
  std::vector<uint32_t> seed;
  Digest seedSHA;
  RequireHost(parent->FillSeed(seed,scope,seedSHA),"SEED_K192");
  std::array<uint32_t,128> config{};
  parent->RootConfig(config,ParentPermit(permit),seedSHA);
  const auto common=gpu.Allocate(kCommonBytes,VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT).slice;
  const auto tail=gpu.Allocate(1024,VK_BUFFER_USAGE_TRANSFER_DST_BIT,true).Readback();
  StorageSlots storage{};
#ifdef PC_SAND_G11_R1
  auto visualConfig=VisualConfig(*parent,2048,256);
  const auto visualConfigSHA=accept647::HashWords(visualConfig.data(),visualConfig.size());
  constexpr auto totalWork=visual.workBytes;
  const auto boot=gpu.Allocate(512+512+128*2048+8192+256,VK_BUFFER_USAGE_TRANSFER_DST_BIT,true).Readback();
  const auto witness=!opt.witness.empty() ?
      gpu.Allocate(kR1WitnessBytes,VK_BUFFER_USAGE_TRANSFER_DST_BIT,true).Readback() : VulkanOwner::Readback{};
#else
  const auto totalWork=l.workBytes;
  const auto boot=gpu.Allocate(8,VK_BUFFER_USAGE_TRANSFER_DST_BIT,true).Readback();
  const auto archive=!opt.archive.empty() ?
      gpu.Allocate(opt.full ? kDmColdFullBytes : kDmColdRootBytes,VK_BUFFER_USAGE_TRANSFER_DST_BIT,true).Readback() :
      VulkanOwner::Readback{};
  std::array<Slice,6> discovery{};
  if (opt.full) for (auto &r:discovery) r=gpu.Allocate(l.arenaBytes,storageUsage).slice;
#endif
  for (size_t i=0;i<storage.size();++i)
    storage[i]=gpu.Allocate(i<5 ? l.stateBytes : i==5 ? totalWork : l.arenaBytes,storageUsage).slice;
  std::array<uint32_t,3> banks{0,1,2};
  uint64_t visGeneration=0,visTicks=0;
  const auto uniform=Uniforms(l,permit,0,banks,visGeneration,visTicks);
  std::vector<uint32_t> work(totalWork/4);
  std::copy(config.begin(),config.end(),work.begin()+l.work[9].begin/4);
#ifdef PC_SAND_G11_R1
  const auto visualControl=visual.visual[kSegControl];
  std::copy(visualConfig.begin(),visualConfig.end(),work.begin()+(visualControl+kControlVisualConfig.byteStart)/4);
#endif
  std::vector<uint32_t> zeroArena(l.arenaBytes/4);
  std::vector<VulkanOwner::Upload> uploads;
  for (size_t i=0;i<5;++i) uploads.push_back({storage[i],seed.data()});
  uploads.push_back({storage[5],work.data()});
  for (size_t i=6;i<storage.size();++i) uploads.push_back({storage[i],zeroArena.data()});
  std::array<uint32_t,kCommonBytes/4> uniformWords{};
  std::memcpy(uniformWords.data(),uniform.data(),uniform.size());
  uploads.push_back({common,uniformWords.data()});
  const auto surface=[&] { return gpu.Current(); };
  const auto bootStarted=HostDevice::Now();
  const auto bootAudit=owner.Qualify(storage,common,boot,uploads.data(),uploads.size(),surface,2000000000);
  const auto bootRetired=HostDevice::Now();
  std::cout << "{\"stage\":\"BOOT\",\"family\":\"" << family << "\",\"roleSetSHA\":\"" << owner.RoleSetSHA()
            << "\",\"vendor\":" << gpu.properties.vendorID << ",\"device\":" << gpu.properties.deviceID
            << ",\"driver\":" << gpu.properties.driverVersion << ",\"run\":\"" << scope.run
            << "\",\"deviceType\":" << gpu.properties.deviceType << ",\"queueFamily\":" << gpu.family
            << ",\"deviceUUID\":\"" << HexBytes(gpu.deviceId.deviceUUID,VK_UUID_SIZE)
            << "\",\"driverUUID\":\"" << HexBytes(gpu.deviceId.driverUUID,VK_UUID_SIZE)
            << "\",\"luidValid\":" << (gpu.deviceId.deviceLUIDValid ? "true" : "false")
            << ",\"luid\":\"" << HexBytes(gpu.deviceId.deviceLUID,VK_LUID_SIZE)
            << "\",\"beginNs\":\"" << bootStarted << "\",\"retiredNs\":\"" << bootRetired
            << "\",\"vkResult\":" << int(bootAudit.result) << ",\"error\":" << bootAudit.error
            << ",\"seal\":" << bootAudit.seal << ",\"eligible\":" << (bootAudit.eligible?"true":"false") << "}\n";
  RequireHost(bootAudit.eligible,"BOOT_GPU_AUDIT_REJECTED");
#ifdef PC_SAND_G11_R1
  std::array<uint32_t,128> seedReceipt{};
  if (!opt.witness.empty()) {
    constexpr size_t rawBytes=512+128*2048+8192+256;
    const auto *seedWords=boot.words+512/4;
    RequireHost(accept647::Pair(seedWords,16)==scope.owner && accept647::Pair(seedWords,18)==scope.context &&
                accept647::Pair(seedWords,20)==scope.run && accept647::Pair(seedWords,22)==scope.surface &&
                std::equal(visualConfigSHA.begin(),visualConfigSHA.end(),seedWords+24) &&
                std::equal(permit.shaderSha.begin(),permit.shaderSha.end(),seedWords+32) &&
                std::equal(visualConfig.begin(),visualConfig.end(),seedWords+(rawBytes-256)/4),
                "R1_BOOTSTRAP_SCOPE_OR_CONFIGURATION");
    auto *r=seedReceipt.data();
    r[0]=0x53423147; r[1]=1; r[2]=4; r[3]=1; r[4]=kCompositeR1Code; r[5]=192; r[6]=2048;
    std::memcpy(r+8,bootAudit.visualSeedSHA.data(),bootAudit.visualSeedSHA.size());
    const Digest hashes[]={parent->ParameterHash(),HexDigest(kCompositeLayoutSHA),permit.shaderSha,
                          visualConfigSHA,HexDigest(kVisualRecipeSHA)};
    for (size_t i=0;i<std::size(hashes);++i) std::copy(hashes[i].begin(),hashes[i].end(),r+16+i*8);
    const uint64_t ids[]={1,scope.owner,scope.context,scope.run,scope.surface,scope.pageSession,scope.inputEpoch};
    for (size_t i=0;i<std::size(ids);++i) g9spf::Put(r,56+i*2,ids[i]);
    r[73]=4;
    g9spf::Put(r,82,bootStarted);
    g9spf::Put(r,84,bootStarted+2000000000);
    g9spf::Put(r,90,bootRetired);
    constexpr uint32_t segmentBytes[]={512,128*2048,8192,256};
    const uint32_t offsets[]={uint32_t(visualControl),uint32_t(visual.visual[kSegMarkerCurrent]),
        uint32_t(visualControl+kControlPacketStyle.byteStart),uint32_t(visualControl+kControlVisualConfig.byteStart)};
    uint32_t packed=0;
    for (size_t i=0;i<4;++i) {
      const auto at=96+i*4;
      r[at]=offsets[i]; r[at+1]=packed; r[at+2]=segmentBytes[i]; r[at+3]=uint32_t(i+1);
      packed+=segmentBytes[i];
    }
    g9spf::Put(r,112,packed);
    g9spf::Put(r,114,packed);
    r[119]=1;
    auto path=opt.witness;
    path+=L".seed.u32le";
    WriteWords(path,seedWords,rawBytes);
    Digest nativeSeed{};
    std::copy_n(r+8,8,nativeSeed.begin());
    std::cout << "{\"stage\":\"VISUAL_SEED\",\"backend\":\"VULKAN\",\"bytes\":" << rawBytes
              << ",\"sha256\":\"" << HexDigest(nativeSeed) << "\"}\n";
  }
#endif
  const uint32_t total=opt.steps+uint32_t(!opt.source.empty());
  if (!total) return 0;
  std::vector<uint32_t>().swap(work);
  std::vector<uint32_t>().swap(zeroArena);
  uploads.clear();
  Gate gate;
  gate.Configure(std::get<PhysicalConfig>(parent->Configuration()),permit.identity);
  if (opt.sourcePrewarm) {
    const auto enter=HostDevice::Now();
    const auto warmed=owner.PrewarmSource(storage,common);
    std::cout << "{\"stage\":\"SOURCE_DESCRIPTOR_PREWARM\",\"family\":\"" << family << "\",\"run\":\""
              << scope.run << "\",\"enterNs\":\"" << enter << "\",\"exitNs\":\"" << HostDevice::Now()
              << "\",\"vkResult\":" << int(warmed) << "}\n";
    RequireVk(warmed,"SOURCE_DESCRIPTOR_PREWARM");
  }
  PulseClock pacing(scope,opt.angle);
  uint64_t commit=0,quanta=0;
  for (uint32_t n=0;n<total;++n) {
    const bool source=n==0 && !opt.source.empty(), last=n+1==total;
    const auto root=source ? Root::Source : Root::Quantum;
    const auto claimLimit=HostDevice::Now()+1000000000;
    while (!pacing.BeginRoot(source)) {
      RequireHost(surface() && HostDevice::Now()<claimLimit,"WAIT_NATIVE_GRANT");
      Sleep(1);
    }
    PulseClock::FreshSample pulse;
    const auto current=[&] { return surface() && pacing.Fresh(source ? &pulse : nullptr); };
    permit.nonce=permit.frame=permit.control=uint64_t(n)+1;
    permit.plan=pacing.grant.id;
    const auto rootBegin=pacing.grant.grantedNs;
    permit.deadline=rootBegin+Clock::RootNs;
#ifndef PC_SAND_G11_R1
    permit.rootBeginNs=rootBegin;
#endif
    permit.baseCommit=commit; permit.quanta=quanta;
    permit.kind=source ? 1u : 2u;
    permit.ordinal=source ? 0 : ++pacing.run.quantumOrdinal;
    permit.arena=uint32_t(commit&1);
    permit.arenaGeneration=commit+1;
    permit.shaderSha=HexDigest(owner.RootModuleSHA(root));
    const auto now=HostDevice::Now();
    Input128 input{}, command{};
    const auto angle=pacing.grant.target;
    if (source) {
      RequireHost(gate.ModelEnvelope(body,angle,now,command) && int(gate.Offer(command,now))==0,
                  "SOURCE_MODEL_ENVELOPE");
    } else gate.Pose(angle,now);
    RequireHost(int(gate.Freeze(source ? &command : nullptr,permit.nonce,now,input))==0,"FROZEN_INPUT");
    const auto frozenNs=source ? HostDevice::Now() : 0;
    permit.inputSha=accept647::HashWords(input.data(),input.size());
    parent->RootConfig(config,ParentPermit(permit),seedSHA);
    const auto unif=Uniforms(l,permit,angle,banks,visGeneration,visTicks);
    std::memcpy(uniformWords.data(),unif.data(),unif.size());
    uploads={{common,uniformWords.data()},{Subspan(storage[5],l.work[5].begin,sizeof(input)),input.data()},
             {Subspan(storage[5],l.work[9].begin,sizeof(config)),config.data()}};
    accept647::ProgramFacts facts;
    facts.parameterRevision=parent->ParameterRevision();
    facts.parameter=parent->ParameterHash();
    facts.physicalLayout=parent->PhysicalLayoutHash();
    facts.physicalSpec=parent->PhysicalSpecHash();
    facts.physicalSeed=seedSHA;
    VulkanOwner::Capture capture{};
#ifdef PC_SAND_G11_R1
    facts.compositeLayout=HexDigest(kCompositeLayoutSHA);
    facts.visualConfig=visualConfigSHA;
    facts.visualRecipe=HexDigest(kVisualRecipeSHA);
    const CompositePermit typed{ParentPermit(permit),rootBegin,visGeneration,visTicks,
                                kCompositeR1Code,visualConfigSHA,HexDigest(owner.RoleSetSHA())};
    const auto expected=owner.Prepare(typed,facts);
    RequireHost(expected.has_value(),"EXPECTED_R1_224");
    std::array<uint32_t,232> expectedWire{};
    std::copy(expected->words.begin(),expected->words.end(),expectedWire.begin());
    std::copy(expected->hash.begin(),expected->hash.end(),expectedWire.begin()+224);
    uploads.push_back({Subspan(storage[5],visualControl+kControlReserved.byteStart,sizeof(expectedWire)),expectedWire.data()});
    const bool captureOn=last && !opt.witness.empty();
    if (captureOn) capture={witness,{0,3,2}};
#else
    const auto expected=owner.Prepare(permit,facts);
    RequireHost(expected.has_value(),"EXPECTED_DM2_224");
    const bool captureOn=last && opt.full;
    if (captureOn) capture={discovery.data()};
#endif
    const auto expectedNs=source ? HostDevice::Now() : 0;
    const VulkanOwner::RootIdentity diagnostic{scope.run,permit.nonce,permit.plan};
    const auto raw=owner.Run(root,storage,common,tail,uploads.data(),uploads.size(),current,
                             permit.deadline,HostDevice::Now,captureOn ? &capture : nullptr,source ? &diagnostic : nullptr);
    const auto before=storage;
    const auto beforeBanks=banks;
    std::array<uint32_t,256> quota{};
    VulkanOwner::AcceptTiming gateTiming;
    uint64_t promoteEnter=0;
    const auto accepted=pacing.Accept(source,[&](const auto &fresh) {
      auto proof=owner.AcceptCurrent([&] { return surface() && fresh(); },HostDevice::Now,source ? &gateTiming : nullptr);
      if (proof) {
        if (source) promoteEnter=HostDevice::Now();
        commit=accept647::Pair(proof->Words().data(),30);
        quanta=accept647::Pair(proof->Words().data(),54);
#ifdef PC_SAND_G11_R1
        visGeneration=accept647::Pair(proof->Words().data(),210);
        visTicks=accept647::Pair(proof->Words().data(),214);
        if (proof->Words()[6]!=2) banks={banks[1],banks[2],banks[0]};
#endif
        if (proof->Words()[6]!=2) std::swap(storage[0],storage[2]);
      }
      return proof;
    },quota);
    const auto promoteExit=promoteEnter ? HostDevice::Now() : 0;
    std::cout << "{\"stage\":\"ROOT\",\"family\":\"" << family << "\",\"kind\":\"" << (source?"SOURCE":"QUANTUM")
              << "\",\"run\":\"" << scope.run << "\",\"nonce\":\"" << permit.nonce << "\",\"plan\":\"" << permit.plan
              << "\",\"fence\":\"" << reinterpret_cast<uintptr_t>(raw.fence) << "\",\"fenceGeneration\":\""
              << raw.generation << "\",\"deadlineNs\":\"" << permit.deadline << "\",\"vkResult\":" << int(raw.result)
              << ",\"dispatches\":" << raw.dispatches << ",\"tailRead\":" << (raw.read?"true":"false")
              << ",\"outcome\":" << raw.tail[6] << ",\"reason\":" << raw.tail[7]
              << ",\"clockFault\":" << pacing.Fault()
              << ",\"accepted\":" << (accepted?"true":"false") << ",\"inputSHA\":\"" << HexDigest(permit.inputSha)
              << "\",\"inputOrigin\":\"SYNTHETIC_RESEARCH\"}\n";
    if (source) {
      owner.FinishSourceDiagnostic(raw,accepted.has_value(),Clock::RootNs);
      SourceTimeline(gpu,owner,diagnostic,before,common,tail.buffer,rootBegin,permit.deadline,frozenNs,expectedNs,
                     pulse,raw,gateTiming,promoteEnter,promoteExit,accepted.has_value());
    }
    if (!accepted) {
      if (!source && owner.HasPendingWork()) std::cout << "{\"stage\":\"RETIRE\",\"vkResult\":"
                                            << int(owner.Retire(Clock::RootNs)) << "}\n";
      RequireHost(false,"ROOT_NOT_ACCEPTED");
    }
#ifdef PC_SAND_G11_R1
    (void)quota;
    if (captureOn) {
      const auto view=owner.ReadR1Witness(*accepted,surface);
      RequireHost(view.has_value(),"WORK_WITNESS_NOT_EXECUTED_OR_MISMATCH");
      std::vector<uint32_t> witnessWords(kR1WitnessBytes/4);
      auto *w=witnessWords.data();
      w[0]=0x57313147; w[1]=2; w[2]=192; w[3]=2048;
      g9spf::Put(w,4,view->nonce);
      w[6]=view->selection.micro; w[7]=view->selection.phase; w[8]=view->selection.role; w[9]=1;
      g9spf::Put(w,10,rootBegin); g9spf::Put(w,12,permit.nonce);
      w[16]=1; w[17]=accepted->Words()[6]; w[18]=raw.captured;
      const uint64_t ids[]={scope.run,scope.owner,scope.context,scope.inputEpoch,scope.pageSession,
                            scope.permission,scope.window,scope.config,scope.surface};
      for (size_t i=0;i<9;++i) g9spf::Put(w,32+i*2,ids[i]);
      const Digest hashes[]={facts.parameter,facts.physicalLayout,HexDigest(owner.RoleSetSHA()),visualConfigSHA,
                              facts.visualRecipe,facts.compositeLayout};
      for (size_t i=0;i<6;++i) std::copy(hashes[i].begin(),hashes[i].end(),w+64+i*8);
      std::copy(seedReceipt.begin(),seedReceipt.end(),w+128);
      std::copy_n(view->bankAndPlan,256,w+256);
      std::copy(accepted->Words().begin(),accepted->Words().end(),w+512);
      std::copy_n(view->payload,view->payloadBytes/4,w+1024);
      WriteWords(opt.witness,w,kR1WitnessBytes);
      std::cout << "{\"stage\":\"WORK_WITNESS\",\"revision\":\"G11_PHASE_WITNESS_V2\",\"state\":\"READY\",\"id\":\""
                << permit.nonce << "\",\"bytes\":" << kR1WitnessBytes
                << ",\"sha256\":\"" << HexDigest(accept647::HashWords(w,witnessWords.size())) << "\"}\n";
    }
    if (last && opt.present) {
      const R1Scene scene{{scope.run,scope.owner,scope.context,scope.window,scope.surface,permit.geometry,
                          scope.permission,1},&owner,permit.nonce,commit,quanta,visGeneration,visTicks,1,before[2],
        Subspan(before[5],visual.visual[beforeBanks[1]],128*visual.v),
        Subspan(before[5],visual.visual[kSegGlyph],32*visual.v),
        Subspan(before[5],visualControl+kControlPacketStyle.byteStart,kControlPacketStyle.bytes)};
      const auto shown=PresentR1(gpu,opt.modules,*accepted,scene);
      std::cout << "{\"stage\":\"PRESENT\",\"root\":\"" << shown.nonce << "\",\"lease\":\""
                << shown.leaseSerial << "\",\"presentEnterQpc\":\"" << shown.enterQpc << "\",\"presentExitQpc\":\""
                << shown.exitQpc << "\",\"imageIndex\":" << shown.imageIndex << "}\n";
    }
#else
    (void)beforeBanks;
    if (last && !opt.archive.empty()) {
      RequireHost(!std::filesystem::exists(opt.archive),"ARCHIVE_DESTINATION_EXISTS");
      auto temp=opt.archive;
      temp+=L".pending-"+std::to_wstring(GetCurrentProcessId());
      RequireHost(!std::filesystem::exists(temp),"ARCHIVE_TEMP_EXISTS");
      std::ofstream out(temp,std::ios::binary);
      const DmColdSources originals{before[1],before[2],before[7],before[6],before[5],discovery};
      const DmColdMetadata meta{permit.nonce,permit.nonce,HostDevice::Now(),input,config,quota};
      const bool sealed=owner.CaptureDmArchive(out,*accepted,originals,opt.full,archive,meta,surface,2000000000);
      out.close();
      if (!sealed || !out || !MoveFileExW(temp.c_str(),opt.archive.c_str(),MOVEFILE_WRITE_THROUGH)) {
        DeleteFileW(temp.c_str());
        RequireHost(false,"DM2_COLD_ARCHIVE");
      }
      std::cout << "{\"stage\":\"DM2_ARCHIVE\",\"revision\":\"G9_SPF_DM_COLD_V2\",\"nonce\":\""
                << permit.nonce << "\",\"bytes\":" << std::filesystem::file_size(opt.archive)
                << ",\"sha256\":\"" << ArchiveSHA(opt.archive) << "\"}\n";
    }
#endif
  }
  RequireHost(pacing.Close(),"CLOSE_QUOTA_BALANCE");
  std::cout << "{\"stage\":\"RETIRED\",\"run\":\"" << scope.run << "\",\"commit\":\"" << commit
            << "\",\"acceptedQuanta\":\"" << quanta << "\",\"quotaBalanced\":true}\n";
  return 0;
}
}
}

int wmain(int argc,wchar_t **argv) {
  using namespace grs::pc::sand;
  if (argc==2 && std::wstring_view(argv[1])==L"--help") {
    std::cout << "pc_sand_" << family << " --modules DIR [--source INPUT128.u32le [--source-timestamps|--source-timestamps-detail] [--source-prewarm]] [--steps N] [--angle-index N] "
              << (kind==grs::g11vis::ParentKind::DM ? "[--archive FILE [--full]]" : "[--witness FILE] [--present]") << '\n';
    return 0;
  }
  try { std::cout << std::unitbuf; return Run(Args(argc,argv)); }
  catch (const std::exception &e) { std::cerr << family << " " << e.what() << '\n'; return 1; }
}
