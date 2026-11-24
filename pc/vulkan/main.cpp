#include "boot.h"
#include "m1_d1_plan.h"
#include "m1_compact.h"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cwchar>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <thread>
#include <utility>
#include <windowsx.h>

namespace {
constexpr UINT kOwnerFinished = WM_APP + 1;
constexpr UINT kFirstPresent = WM_APP + 2;
constexpr UINT kSetM1Gravity = WM_APP + 3;
constexpr UINT kM1GravityCaption = WM_APP + 4;
constexpr UINT_PTR kSourceProbeTimer = 2;
struct Window {
  HWND hwnd = nullptr;
  std::mutex mutex, output;
  std::condition_variable wake;
  uint64_t generation = 0, eventSeq = 0;
  int width = 0, height = 0;
  UINT dpi = 96;
  bool closing = false;
  bool once = false, bootOnly = false, steady = false;
  std::string steadyRole;
  bool disableOptimization = false;
  std::wstring m1SourceModule;
  std::wstring m1QuantumCandidate, m1QuantumCommit, m1ColdFile;
  bool m1Present = false;
  bool dualSource = false;
  bool m1Preview=false, m1GravityEnabled=false;
  bool gravityClick=false,m1StageTiming=false,g13TailTiming=false,m1InventoryV1=false,g13SealV1=false;
  bool m1AngleWitness=false;
  uint32_t m1Kappa=24;
  bool m1OneQuantumGrant=false;
  grs::pc::M1Gravity gravity;
  grs::pc::M1GravityDisplay gravityDisplay;
  std::wstring gravityCaption;
  enum class M1IntakeState { Normal, Retiring, Fault };
  M1IntakeState m1IntakeState=M1IntakeState::Normal;
  grs::pc::M1Observation observation;
  uint32_t material=0;
  std::wstring t849WriteFile;
  bool t854PrivateWrites = false;
  std::wstring t833CandidateFile;
  std::string t833RevokeAt;
  std::string t833Case;
  std::wstring t833ConfigFile,t833SeedFile;
  uint32_t t833ForcedJ=0;
  bool t833F05Rest=false;
  std::array<grs::pc::CandidatePulse,32> g13PulseHistory{};
  std::string t877Mode;
  bool t899SlotSnapshot=false;
  bool g13CompileTrace=false;
  bool m1Compact=false;
  bool m1Integrated=false,m1AuditBreakdown=false;
  bool g13SplitOp48=false;
  bool m1ParallelInventory=false;
  bool g13SealStages=false;
  std::string g13CycleScope;
  std::wstring m1PrivateReference,m1PrivateOutput;
  bool m1PrivateObserve=false;
  bool compactRequestAtLimit=false;
  grs::pc::CompactView compactShown;
  uint64_t compactRevision=0;
  uint64_t compactAnchorAfter=0;
  bool compactResample=false;
  enum class CompactIntake { Normal, Updating, Outside, Bed, Capacity };
  CompactIntake compactIntake=CompactIntake::Normal;
  size_t g13PulseWrite=0;
  bool t833Allowed=true;
  std::atomic<uint64_t> sourcePulse{0};
  bool pointerReady = false;
  uint64_t pointerReadyQpc=0;
  grs::g13vk::PointerSample held;
  grs::g13vk::SourceScope scope;
  grs::g13vk::M1Ingress ingress;
  grs::g13vk::M1HotCapture hotCapture;
  grs::g13vk::M1PulseQueue m1Pulses;
  std::condition_variable outputReady;
  std::array<std::string,128> outputRows;
  size_t outputRead=0,outputSize=0;
  uint64_t outputDropped=0;
  uint64_t outputLossTotal=0;
  bool outputDone=false;
  std::string outputFinal;
  bool outputFinalWritten=false;
  std::mutex sourceTraceLock;
  grs::g13vk::M1D1Intake d1Intake;
  grs::g13vk::SourceScope d1ReadyScope;
  uint64_t d1ReadyNs=0;
  uint64_t d1Queued=0;
  std::chrono::steady_clock::time_point d1ReadyTime;
  bool captureHot=false, captureCold=false, sourceTrace=false;
  bool rootCapture=false;
  std::wstring d1Mode,d1ReplayPrefix;
  bool d1ColdReanchor=false;
  std::condition_variable d1Ready;
  std::atomic<unsigned> m1Shown{0};
  uint64_t captureRun=0, captureReserved=0, droppedTrace=0;
  size_t coldOffset=0;
  uint64_t sampleTicks=0, offerSeq=0;
  grs::g13vk::M1Pulse sourceAnchor{};
  uint64_t sourceAnchorHold=0;
  struct HeldRecovery {
    uint64_t hold=0,geometry=0,run=0,oldSession=0,newSession=0,boundary=0;
    uint32_t material=0;
  } heldRecovery;
  int result = 1;

  void Emit(const std::string &json) {
    std::lock_guard lock(output);
    if (outputSize==outputRows.size() || json.size()>64*1024) { ++outputDropped; ++outputLossTotal; }
    else outputRows[(outputRead+outputSize++)%outputRows.size()]=json;
    outputReady.notify_one();
  }
  void WriteLogs() {
    for (;;) {
      std::unique_lock lock(output);
      outputReady.wait(lock,[&] { return outputDone || outputSize || outputDropped || !outputFinal.empty(); });
      if (!outputSize && !outputDropped && outputFinal.empty()) return;
      std::string row;
      bool final=false;
      if (outputSize) {
        row=std::move(outputRows[outputRead]);
        outputRead=(outputRead+1)%outputRows.size();
        --outputSize;
      } else if (!outputFinal.empty()) {
        row=std::move(outputFinal);
        outputFinal.clear();
        final=true;
      }
      const auto dropped=std::exchange(outputDropped,0);
      lock.unlock();
      if (dropped) std::cout << "{\"stage\":\"PC_TRACE_INCOMPLETE\",\"dropped\":" << dropped << "}\n";
      if (!row.empty()) std::cout << row << '\n';
      std::cout.flush();
      if (final) {
        std::lock_guard done(output);
        outputFinalWritten=true;
        outputReady.notify_all();
      }
    }
  }
  grs::g13vk::M1PulseQueue::Receipt ReceiveM1Pulse(uint64_t ticks,uint64_t ns,uint64_t hz,
      uint32_t action=0,uint64_t *receivedQpc=nullptr) {
    const auto receipt=grs::pc::QpcTicks();
    if (receivedQpc) *receivedQpc=receipt;
    return m1Pulses.ReceiveWithReceipt(generation,{0,ticks,receipt,ns,grs::pc::QpcNs(receipt,hz),0,action});
  }
  bool D1CaptureEnabled() const { return d1Mode==L"ON" || d1Mode==L"REPLAY_ON"; }
  void DrainSourceTrace() {
    if (!sourceTrace) return;
    std::lock_guard drainLock(sourceTraceLock);
    std::array<grs::g13vk::M1Ingress::Trace,64> batch;
    uint64_t dropped=0;
    size_t count;
    do {
      count=ingress.ReadTrace(batch.data(),batch.size(),dropped);
      for (size_t i=0;i<count;++i) {
        const auto &record=batch[i];
        const auto &o=record.offer;
        std::ostringstream row;
        if (o.current.seq) {
          row << "{\"stage\":\"M1_RESERVATION_" << (record.terminal ? "TERMINAL" : "RETURN")
              << "\",\"serial\":\"" << record.serial << "\",\"run\":\"" << o.scope.run
              << "\",\"geometry\":\"" << o.scope.geometry << "\",\"offer\":\"" << o.seq
              << "\",\"hold\":\"" << o.sample.hold << "\",\"authorityEpoch\":\"" << o.scope.permission
              << "\",\"material\":" << o.sample.material
              << ",\"session\":\"" << o.current.session << "\",\"previousSeq\":\"" << o.previous.seq
              << "\",\"currentSeq\":\"" << o.current.seq << "\",\"deltaNs\":\"" << o.deltaNs
              << "\",\"previousCallbackQpc\":\"" << o.previous.callbackTicks
              << "\",\"currentCallbackQpc\":\"" << o.current.callbackTicks
              << "\",\"previousReceiptQpc\":\"" << o.previous.receiptTicks
              << "\",\"currentReceiptQpc\":\"" << o.current.receiptTicks
              << "\",\"result\":" << int(record.result.code) << ",\"root\":\"" << record.result.root << "\"";
          if (record.reason) row << ",\"reason\":\"" << record.reason << "\"";
          Emit(row.str()+"}");
          continue;
        }
        row << "{\"stage\":\"M1_OFFER_" << (record.terminal ? "TERMINAL" : "RETURN")
            << "\",\"serial\":\"" << record.serial << "\",\"run\":\"" << o.scope.run
            << "\",\"geometry\":\"" << o.scope.geometry << "\",\"offer\":\"" << o.seq
            << "\",\"actionId\":" << o.action
            << ",\"sample\":\"" << o.sample.sequence << "\",\"hold\":\"" << o.sample.hold
            << "\",\"pulseTicks\":\"" << o.pulseTicks << "\",\"sampleTicks\":\"" << o.sampleTicks
            << "\",\"frequency\":\"" << o.frequency << "\",\"pulseNs\":\"" << o.pulseNs
            << "\",\"sampleNs\":\"" << o.sample.timeNs << "\",\"pixelX\":" << o.sample.clientX
            << ",\"pixelY\":" << o.sample.clientY << ",\"material\":" << o.sample.material
            << ",\"result\":" << int(record.result.code) << ",\"root\":\"" << record.result.root << "\""
            << ",\"deltaNs\":\"" << o.deltaNs << "\"";
        if (record.reason) row << ",\"reason\":\"" << record.reason << "\"";
        if (record.checkedNs) row << ",\"checkedNs\":\"" << record.checkedNs << "\",\"ageNs\":\""
                                  << (record.checkedNs>=o.pulseNs ? record.checkedNs-o.pulseNs : 0) << "\"";
        Emit(row.str()+"}");
      }
    } while (count==batch.size());
    if (droppedTrace!=dropped) {
      droppedTrace=dropped;
      Emit("{\"stage\":\"M1_OFFER_TRACE_INCOMPLETE\",\"dropped\":"+std::to_string(dropped)+"}");
    }
  }
  void ReportD1(const std::string &details,bool healthy) {
    if (!d1Intake.MarkReported()) return;
    DrainSourceTrace();
    const auto sourceLoss=ingress.TraceLoss();
    grs::g13vk::SourceScope ready;
    uint64_t producerQueued=0;
    {
      std::lock_guard lock(mutex);
      ready=d1ReadyScope;
      producerQueued=d1Queued;
    }
    const auto ledger=m1Pulses.Ledger(ready.run,ready.geometry);
    const auto count=[](uint64_t bits) {
      unsigned result=0;
      for (;bits;bits&=bits-1) ++result;
      return result;
    };
    uint64_t recordedQueued=0;
    for (size_t i=0;i<ledger.size;++i) recordedQueued+=count(ledger.sessions[i].queued);
    const bool sealed=d1Intake.Snapshot().ns!=0;
    std::lock_guard lock(output);
    const auto loss=sourceLoss+outputLossTotal;
    std::ostringstream coverage;
    coverage << ",\"ownerCoverage\":{\"revision\":\"M1_D1_OWNER_COVERAGE_V1\",\"complete\":"
             << (ledger.Complete() && producerQueued==recordedQueued && sealed && !loss ? "true" : "false")
             << ",\"loss\":" << ledger.lost << ",\"producerQueued\":" << producerQueued << ",\"sessions\":[";
    for (size_t i=0;i<ledger.size;++i) {
      const auto &s=ledger.sessions[i];
      if (i) coverage << ',';
      coverage << "{\"session\":\"" << s.scope.session << "\",\"queued\":" << count(s.queued)
               << ",\"accepted\":" << count(s.accepted) << ",\"rejected\":" << count(s.rejected)
               << ",\"revoked\":" << count(s.revoked) << ",\"queuedMask\":\"" << s.queued
               << "\",\"terminalMask\":\"" << (s.accepted|s.rejected|s.revoked) << "\"}";
    }
    coverage << "]}";
    outputFinal="{\"stage\":\"M1_D1_DRAIN\",\"m1D1Drain\":{" + details +
                ",\"retired\":" + (healthy && !loss ? "true" : "false") +
                ",\"traceLoss\":" + std::to_string(loss) + coverage.str() + "}}";
    outputReady.notify_one();
  }
  void RunD1() {
    using namespace grs::g13vk;
    SourceScope readyScope;
    const auto begin=std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point anchor;
    uint64_t ready=0;
    {
      std::unique_lock lock(mutex);
      d1Ready.wait(lock,[&] { return closing || d1ReadyNs; });
      if (closing) return;
      readyScope=d1ReadyScope;
      ready=d1ReadyNs;
      anchor=d1ReadyTime;
    }
    const bool enabled=D1CaptureEnabled();
    const std::string mode(d1Mode.begin(),d1Mode.end());
    uint64_t reservation=0,lastPulseSeq=0,lastPulseSession=0;
    M1Pulse previous;
    D1Action pending{};
    uint64_t createdNs=0,createdTicks=0;
    Emit("{\"stage\":\"M1_D1_READY\",\"readyNs\":\""+std::to_string(ready)+
         "\",\"run\":\""+std::to_string(readyScope.run)+"\",\"mode\":\""+mode+
         "\",\"qpcFrequency\":\""+std::to_string(grs::pc::QpcFrequency())+
         "\",\"waitMs\":"+std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(anchor-begin).count())+"}");
    for (const auto &action:M1D1Plan()) {
      std::this_thread::sleep_until(anchor+std::chrono::nanoseconds(action.ns));
      const auto ticks=grs::pc::QpcTicks(),hz=grs::pc::QpcFrequency();
      const auto ns=grs::pc::QpcNs(ticks,hz);
      SourceOffer offer;
      M1Pulse nativePulse;
      const char *nativeState="NOT_SUBMITTED";
      uint64_t receiptQpc=0;
      uint32_t sourceRejected=0;
      bool current;
      {
        std::lock_guard lock(mutex);
        current=!closing && generation==readyScope.geometry && pointerReady;
        if (current && action.kind==D1Kind::Source) {
          pending=action;
          createdNs=ns;
          createdTicks=ticks;
        }
        if (current && action.kind==D1Kind::Pulse) {
          sourcePulse.store(ns,std::memory_order_relaxed);
          const auto received=ReceiveM1Pulse(ticks,ns,hz,action.id,&receiptQpc);
          nativePulse=received.pulse;
          nativeState=received.state;
          if (nativePulse.seq) {
            ++d1Queued;
            lastPulseSeq=nativePulse.seq;
            lastPulseSession=nativePulse.session;
          }
          if (pending.id && AdjacentSourcePulses(previous,nativePulse)) {
            const auto serial=++eventSeq;
            held={serial,serial,createdNs,64.5,pending.y,int(64.5*width/128.),
                  int((1.-pending.y/256.)*height),0};
            scope.permission=held.hold;
            ingress.Authorize(scope,held.hold);
            offer={scope,++offerSeq,ns,ticks,createdTicks,grs::pc::QpcFrequency(),held,2,pending.id,
                   ns-previous.uiNs};
          } else if (pending.id) {
            sourceRejected=pending.id;
          }
          pending={};
          previous=nativePulse;
        }
      }
      const char *state="REVOKED";
      if (current) {
        switch (action.kind) {
        case D1Kind::Pulse:
          state=nativePulse.seq ? "DELIVERED" : nativeState;
          if (offer.seq) {
            const auto result=ingress.OfferSource(offer);
            if (result.code!=M1Ingress::Code::Pending)
              Emit("{\"stage\":\"M1_D1_SOURCE_TERMINAL\",\"actionId\":"+std::to_string(offer.action)+
                   ",\"offerResult\":"+std::to_string(int(result.code))+",\"state\":\"NOT_ADMITTED\"}");
          }
          break;
        case D1Kind::Source: state="AWAIT_PULSE"; break;
        case D1Kind::Reserve:
          reservation=action.id;
          state=!enabled ? "OFF_NOOP" :
            hotCapture.Reserve(readyScope,reservation,ns)==M1HotCapture::Code::Reserved ? "RESERVED" : "NOT_BUSY";
          break;
        case D1Kind::End:
          state=enabled ? "NO_PAGE" : "OFF_NOOP";
          if (enabled) if (const auto page=hotCapture.Read(readyScope,reservation,ns)) {
            std::ostringstream record;
            record << "{\"stage\":\"M1_D1_HOT_PAGE\",\"request\":" << reservation << ",\"root\":\"" << page->busyRoot
                   << "\",\"safePoint\":\"" << page->safePoint << "\",\"completion\":[";
            for (size_t i=0;i<page->completion.size();++i) {
              if (i) record << ',';
              record << page->completion[i];
            }
            Emit(record.str()+"]}");
            hotCapture.End(readyScope,reservation);
            state="ENDED";
          }
          break;
        }
      }
      if (sourceRejected)
        Emit("{\"stage\":\"M1_D1_SOURCE_TERMINAL\",\"actionId\":"+std::to_string(sourceRejected)+
             ",\"state\":\"NO_ADJACENT_QUEUED_PULSE\",\"pulseActionId\":"+std::to_string(action.id)+"}");
      Emit("{\"stage\":\"M1_D1_ACTION\",\"id\":"+std::to_string(action.id)+
           ",\"kind\":"+std::to_string(int(action.kind))+",\"scriptNs\":\""+std::to_string(action.ns)+
           "\",\"actualNs\":\""+std::to_string(ns)+"\",\"callbackQpc\":\""+std::to_string(ticks)+
           "\",\"nativeReceiptQpc\":\""+std::to_string(receiptQpc)+
           "\",\"nativeSeq\":\""+std::to_string(nativePulse.seq)+
           "\",\"session\":\""+std::to_string(nativePulse.session)+"\",\"state\":\""+state+"\"}");
    }
    std::lock_guard lock(mutex);
    if (pending.id)
      Emit("{\"stage\":\"M1_D1_SOURCE_TERMINAL\",\"actionId\":"+std::to_string(pending.id)+
           ",\"state\":\"REVOKED_END_INTAKE\"}");
    ingress.EndIntake();
    const auto cancelled=hotCapture.EndIntake();
    const auto ended=grs::pc::TimeNs();
    Emit("{\"stage\":\"END_INTAKE\",\"run\":\""+std::to_string(readyScope.run)+
         "\",\"geometry\":\""+std::to_string(readyScope.geometry)+"\",\"endNs\":\""+std::to_string(ended)+
         "\",\"lastPulseSeq\":\""+std::to_string(lastPulseSeq)+"\",\"lastOffer\":\""+std::to_string(offerSeq)+
         "\",\"lastPulseSession\":\""+std::to_string(lastPulseSession)+
         "\",\"lastAction\":52,\"cancelledCapture\":\""+std::to_string(cancelled)+"\"}");
    d1Intake.Seal({ended,lastPulseSeq,offerSeq});
  }

  void HoldRecoveryTrace(const char *event,uint64_t session,uint64_t qpc,
                         const grs::g13vk::M1Pulse &baseline={}) {
    Emit("{\"stage\":\"M1_HOLD_RECOVERY\",\"event\":\""+std::string(event)+
         "\",\"reason\":\"CLOCK_GAP\",\"run\":\""+std::to_string(heldRecovery.run)+
         "\",\"geometry\":\""+std::to_string(heldRecovery.geometry)+
         "\",\"hold\":\""+std::to_string(heldRecovery.hold)+
         "\",\"authorityEpoch\":\""+std::to_string(scope.permission)+
         "\",\"material\":"+std::to_string(heldRecovery.material)+
         ",\"session\":\""+std::to_string(session)+"\",\"qpc\":\""+std::to_string(qpc)+
         "\",\"baselineSeq\":\""+std::to_string(baseline.seq)+
         "\",\"baselineCallbackQpc\":\""+std::to_string(baseline.callbackTicks)+
         "\",\"baselineReceiptQpc\":\""+std::to_string(baseline.receiptTicks)+"\"}");
  }

  void ClearSourceAnchor() {
    sourceAnchor={};
    sourceAnchorHold=0;
  }

  void CancelHeld() {
    heldRecovery={};
    held={};
    compactResample=false;
    compactAnchorAfter=0;
    scope.permission=0;
    ClearSourceAnchor();
    ingress.Authorize(scope,0);
  }

  void SuspendHeld(uint64_t session) {
    pointerReady=false;
    if (heldRecovery.oldSession==session) return;
    heldRecovery={held.hold,generation,scope.run,session,0,0,held.material};
    HoldRecoveryTrace("SUSPEND",session,grs::pc::QpcTicks());
    scope.permission=0;
    ClearSourceAnchor();
    ingress.Authorize(scope,0);
  }

  void ResumeHeld(uint64_t session,uint64_t retired,const grs::g13vk::M1Pulse &baseline) {
    if (!heldRecovery.oldSession || !heldRecovery.hold || session<=heldRecovery.oldSession || !retired ||
        !grs::g13vk::SourcePulseAfter(baseline,session,retired)) {
      Emit("{\"stage\":\"M1_INPUT_WAIT_NEW_DOWN\",\"geometry\":\""+std::to_string(generation)+
           "\",\"session\":\""+std::to_string(session)+"\",\"retiredQpc\":\""+std::to_string(retired)+"\"}");
      CancelHeld();
      return;
    }
    HoldRecoveryTrace("RETIRED",heldRecovery.oldSession,retired);
    heldRecovery.newSession=session;
    heldRecovery.boundary=baseline.receiptTicks;
    HoldRecoveryTrace("NEW_BASELINE",session,baseline.receiptTicks,baseline);
  }

  void RevalidateHeld(const grs::g13vk::M1Pulse &pulse) {
    if (!heldRecovery.newSession || scope.permission || !pulse.seq || pulse.session!=heldRecovery.newSession) return;
    if (!held.hold || held.hold!=heldRecovery.hold || generation!=heldRecovery.geometry ||
        scope.run!=heldRecovery.run || held.material!=heldRecovery.material || material!=held.material ||
        held.clientX<0 || held.clientX>=width || held.clientY<0 || held.clientY>=height || GetCapture()!=hwnd) {
      HoldRecoveryTrace("WAIT_NEW_DOWN",pulse.session,grs::pc::QpcTicks());
      CancelHeld();
      return;
    }
    scope.permission=++eventSeq;
    heldRecovery.boundary=grs::pc::QpcTicks(); // Excludes the pulse used for UI capture revalidation.
    ClearSourceAnchor();
    ingress.Authorize(scope,held.hold);
    HoldRecoveryTrace("HOLD_REVALIDATED",pulse.session,heldRecovery.boundary);
  }

  void Size() {
    RECT r{};
    if (!GetClientRect(hwnd, &r)) return;
    std::lock_guard lock(mutex);
    width = r.right - r.left;
    height = r.bottom - r.top;
    if (IsIconic(hwnd)) width = height = 0;
    dpi = GetDpiForWindow(hwnd);
    ++generation;
    compactShown={};
    compactIntake=m1Integrated ? CompactIntake::Updating : CompactIntake::Normal;
    m1IntakeState=M1IntakeState::Normal;
    gravity={generation};
    compactRequestAtLimit=false;
    pointerReady = false;
    CancelHeld();
    ingress.Authorize({},0);
    hotCapture.Authorize({});
    m1Pulses.Close();
    wake.notify_one();
  }

  void Input(const char *kind, int x, int y) {
    std::unique_lock lock(mutex);
    if (closing) return;
    const auto sequence = ++eventSeq;
    const auto ticks = grs::pc::QpcTicks();
    double mappedX=0,mappedY=0;
    const bool mapped=!m1Integrated || (compactShown.mapped && compactShown.geometry==generation);
    const bool inside = x >= 0 && y >= 0 && x < width && y < height &&
        (m1Integrated ? grs::pc::MapCompactPointer(compactShown,x,y,mappedX,mappedY) :
         !m1Compact || grs::pc::CompactPointer(x,y,width,height,gravity.angle,mappedX,mappedY));
    const std::string action(kind);
    if (!d1Mode.empty() && (action=="DOWN" || action=="MOVE" || action=="UP")) return;
    if (action=="FOCUS_LOST" && m1GravityEnabled) {
      gravity={generation,sequence,ticks,0};
      compactRequestAtLimit=false;
      if (m1Compact) PostMessageW(hwnd,kM1GravityCaption,0,0);
    }
    if (action == "DOWN") {
      CancelHeld();
      if (pointerReady && inside && GetCapture()==hwnd) {
        held.hold = sequence;
        held.material = material;
        scope.permission=sequence;
        compactAnchorAfter=ticks;
      }
    } else if (action!="MOVE" || (mapped && !inside)) {
      CancelHeld();
    }
    if (held.hold && mapped) {
      held.sequence = sequence;
      held.timeNs = grs::pc::TimeNs();
      held.x = m1Compact ? mappedX : (double(x)+0.5)*128/width;
      held.y = m1Compact ? mappedY : (1-(double(y)+0.5)/height)*256;
      held.clientX = x;
      held.clientY = y;
      if (m1Integrated) {
        held.transformRevision=compactShown.revision;
        held.frameSerial=compactShown.serial;
      }
      sampleTicks = ticks;
    }
    if (m1Integrated && (action=="DOWN" || (action=="MOVE" && !inside && GetCapture()==hwnd))) {
      compactIntake=!mapped ? CompactIntake::Updating : !inside ? CompactIntake::Outside : CompactIntake::Normal;
      PostMessageW(hwnd,kM1GravityCaption,0,0);
    }
    const auto frameRevision=compactShown.revision,frameSerial=compactShown.serial;
    ingress.Authorize(scope,scope.permission ? held.hold : 0);
    const auto geometry=generation, hold=held.hold;
    const auto inputReady=pointerReady;
    const auto readyQpc=pointerReadyQpc;
    const auto selected=material;
    const auto bound=held.material;
    const auto eventDpi=dpi;
    lock.unlock();
    Emit("{\"stage\":\"WIN32_POINTER\",\"kind\":\"" + action +
         "\",\"geometry\":\"" + std::to_string(geometry) +
         "\",\"seq\":\"" + std::to_string(sequence) +
         "\",\"qpc\":\"" + std::to_string(ticks) +
         "\",\"x\":" + std::to_string(x) + ",\"y\":" + std::to_string(y) +
         ",\"dpi\":" + std::to_string(eventDpi) + ",\"hold\":\"" + std::to_string(hold) +
         "\",\"inputReady\":" + (inputReady ? "true" : "false") +
         ",\"readyQpc\":\"" + std::to_string(readyQpc) + "\",\"selectedMaterial\":" + std::to_string(selected) +
         ",\"boundMaterial\":" + (hold ? std::to_string(bound) : "null") +
         ",\"transformRevision\":\"" + std::to_string(frameRevision) + "\",\"presentedSerial\":\"" +
         std::to_string(frameSerial) + "\"" +
         ",\"reject\":" + (action!="DOWN" || hold ? "null" :
                      !mapped ? "\"PRESENTED_MAPPING_UNAVAILABLE\"" :
                      !inputReady ? "\"NOT_READY\"" : "\"OUTSIDE_CLIENT\"") + "}");
  }

  void SelectMaterial(uint32_t next) {
    std::lock_guard lock(mutex);
    if (closing || material==next) return;
    const auto revoked=held.hold;
    const auto old=material;
    material=next;
    CancelHeld();
    Emit("{\"stage\":\"M1_SOURCE_MODE\",\"kind\":\"MODE_CHANGED\",\"geometry\":\""+std::to_string(generation)+
         "\",\"seq\":\""+std::to_string(++eventSeq)+"\",\"qpc\":\""+std::to_string(grs::pc::QpcTicks())+
         "\",\"revokedHold\":\""+std::to_string(revoked)+"\",\"oldMaterial\":"+std::to_string(old)+
         ",\"material\":"+std::to_string(next)+",\"inputReady\":"+(pointerReady ? "true" : "false")+"}");
  }

  void SetGravity(int32_t angle,uint64_t expectedGeometry,bool relative=false) {
    std::lock_guard lock(mutex);
    const auto seq=++eventSeq;
    const auto ticks=grs::pc::QpcTicks();
    const auto target=relative ? gravity.angle+angle : angle;
    if (relative) angle=std::clamp(target,-grs::pc::kM1AngleLimit,grs::pc::kM1AngleLimit);
    const char *result=!m1GravityEnabled ? "GRAVITY_DISABLED" :
        closing || (expectedGeometry && expectedGeometry!=generation) ? "STALE_GEOMETRY" :
        !grs::pc::ValidM1Angle(angle) ? "ANGLE_OUT_OF_RANGE" : "ACCEPTED_INPUT";
    if (std::string_view(result)=="ACCEPTED_INPUT") {
      gravity={generation,seq,ticks,angle};
      compactRequestAtLimit=m1Compact && target!=angle;
      PostMessageW(hwnd,kM1GravityCaption,0,0);
    }
    Emit("{\"stage\":\"M1_GRAVITY_INPUT\",\"geometry\":\""+std::to_string(generation)+
         "\",\"seq\":\""+std::to_string(seq)+"\",\"qpc\":\""+std::to_string(ticks)+
         "\",\"requestedAngle256\":"+std::to_string(angle)+",\"result\":\""+result+"\"}");
  }

  bool ResetGravityButton(int x,int y) {
    uint64_t geometry=0;
    {
      std::lock_guard lock(mutex);
      if (!m1GravityEnabled || closing || (m1Integrated && (!compactShown.mapped || compactShown.geometry!=generation)) ||
          !grs::pc::M1ResetHit(x,y,width,height)) return false;
      geometry=generation;
    }
    SetGravity(0,geometry);
    return true;
  }

  uint64_t SetCompactPresented(grs::pc::CompactView &view) {
    std::lock_guard lock(mutex);
    if (closing || view.geometry!=generation || view.run!=scope.run ||
        view.window!=reinterpret_cast<uintptr_t>(hwnd)) return 0;
    view.mapped=view.mapped && view.clientWidth==uint32_t(width) && view.clientHeight==uint32_t(height) && view.dpi==dpi;
    const bool changed=!grs::pc::SameCompactMapping(compactShown,view);
    view.revision=changed ? ++compactRevision : compactShown.revision;
    compactShown=view;
    if (changed) {
      compactIntake=view.mapped ? CompactIntake::Normal : CompactIntake::Updating;
      if (held.hold) {
        scope.permission=0;
        ClearSourceAnchor();
        ingress.Authorize(scope,0);
        compactResample=true;
      }
      PostMessageW(hwnd,kM1GravityCaption,0,0);
    }
    return view.revision;
  }

  void CompactSourceResult(const grs::g13vk::SourceOffer &offer,uint32_t reason,bool admitted) {
    std::lock_guard lock(mutex);
    if (closing || !scope.permission || !(scope==offer.scope) || held.hold!=offer.sample.hold ||
        held.sequence!=offer.sample.sequence || compactShown.revision!=offer.sample.transformRevision) return;
    const auto status=admitted ? CompactIntake::Normal : reason==4 ? CompactIntake::Bed :
        reason==5 || reason==6 || reason==7 || reason==11 ? CompactIntake::Capacity : compactIntake;
    if(status==compactIntake)return;
    compactIntake=status;
    PostMessageW(hwnd,kM1GravityCaption,0,0);
  }

  bool ResampleCompactHold() {
    if (!compactResample) return true;
    if (!compactShown.mapped) return false;
    POINT point{};
    double x=0,y=0;
    if (!held.hold || GetCapture()!=hwnd || !GetCursorPos(&point) || !ScreenToClient(hwnd,&point) ||
        !grs::pc::MapCompactPointer(compactShown,point.x,point.y,x,y)) {
      CancelHeld();
      compactIntake=CompactIntake::Outside;
      PostMessageW(hwnd,kM1GravityCaption,0,0);
      return false;
    }
    if (heldRecovery.oldSession && !heldRecovery.newSession) return false;
    sampleTicks=grs::pc::QpcTicks();
    compactAnchorAfter=sampleTicks;
    held.sequence=++eventSeq;
    held.timeNs=grs::pc::QpcNs(sampleTicks,grs::pc::QpcFrequency());
    held.x=x;held.y=y;held.clientX=point.x;held.clientY=point.y;
    held.transformRevision=compactShown.revision;held.frameSerial=compactShown.serial;
    scope.permission=held.sequence;
    ingress.Authorize(scope,held.hold);
    ClearSourceAnchor();
    compactResample=false;
    Emit("{\"stage\":\"M1_COMPACT_RESAMPLE\",\"hold\":\""+std::to_string(held.hold)+
         "\",\"seq\":\""+std::to_string(held.sequence)+"\",\"qpc\":\""+std::to_string(sampleTicks)+
         "\",\"transformRevision\":\""+std::to_string(held.transformRevision)+
         "\",\"presentedSerial\":\""+std::to_string(held.frameSerial)+
         "\",\"x\":"+std::to_string(point.x)+",\"y\":"+std::to_string(point.y)+"}");
    return true;
  }

  void SetGravityDisplay(grs::pc::M1GravityDisplay value) {
    std::lock_guard lock(mutex);
    if (!m1GravityEnabled || closing || value.geometry!=generation) return;
    if (gravityDisplay.geometry==value.geometry && gravityDisplay.sequence==value.sequence &&
        gravityDisplay.actual==value.actual && gravityDisplay.accepted==value.accepted &&
        gravityDisplay.limited==value.limited && gravityDisplay.stale==value.stale) return;
    gravityDisplay=value;
    PostMessageW(hwnd,kM1GravityCaption,0,0);
  }

  void UpdateGravityCaption() {
    if (m1QuantumCandidate.empty()) return;
    std::wostringstream title;
    title.setf(std::ios::fixed,std::ios::floatfield);
    title.precision(1);
    {
      std::lock_guard lock(mutex);
      if (m1GravityEnabled) {
        const bool accepted=gravityDisplay.accepted && gravityDisplay.geometry==generation;
        title << (m1Integrated ? L"盒体  请求 " : m1Compact ? L"盒体预览 " : L"重力  请求 ")
              << (gravity.geometry==generation ? gravity.angle/256. : 0.) << L"°";
        if (compactRequestAtLimit) title << L"  请求到边界";
        if (accepted || (m1Integrated && compactShown.geometry==generation))
          title << L"  实际 " << (accepted ? gravityDisplay.actual : compactShown.angle)/256. << L"°";
        if (accepted && gravityDisplay.stale) title << L"  上一有效帧";
        if (accepted && gravityDisplay.limited) title << L"  床形限制";
        if ((!m1Integrated || gravity.sequence) && (!accepted || gravityDisplay.sequence!=gravity.sequence))
          title << L"  待应用";
        title << L"  |  R 重力回正";
      } else title << L"HelloWorld Vulkan M1";
      if (m1IntakeState==M1IntakeState::Retiring) title << L"  |  GPU 退役中，暂停入料";
      if (m1IntakeState==M1IntakeState::Fault) title << L"  |  GPU 退役故障，请重启";
      if (m1Integrated) {
        if (compactIntake==CompactIntake::Updating) title << L"  |  画面更新中";
        if (compactIntake==CompactIntake::Outside) title << L"  |  盒外，请重新按下";
        if (compactIntake==CompactIntake::Bed) title << L"  |  床面遮挡";
        if (compactIntake==CompactIntake::Capacity) title << L"  |  入料容量已满";
      }
    }
    const auto caption=title.str();
    if (caption!=gravityCaption) {
      gravityCaption=caption;
      SetWindowTextW(hwnd,caption.c_str());
    }
  }

  void Pulse() {
    const auto ticks=grs::pc::QpcTicks(), hz=grs::pc::QpcFrequency();
    const auto ns=grs::pc::QpcNs(ticks,hz);
    sourcePulse.store(ns,std::memory_order_relaxed);
    if (!t833CandidateFile.empty()) {
      std::lock_guard lock(mutex);
      const auto receipt=grs::pc::QpcTicks();
      g13PulseHistory[g13PulseWrite%g13PulseHistory.size()]={g13PulseWrite+1,ticks,receipt,ns,
                                                           grs::pc::QpcNs(receipt,hz)};
      ++g13PulseWrite;
      return;
    }
    grs::g13vk::SourceOffer offer;
    {
      std::lock_guard lock(mutex);
      const auto pulse=!closing && !m1SourceModule.empty() ? ReceiveM1Pulse(ticks,ns,hz).pulse : grs::g13vk::M1Pulse{};
      if (closing || !pointerReady) return;
      RevalidateHeld(pulse);
      if (m1Integrated && !ResampleCompactHold()) return;
      offer.scope=scope;
      if (sourceAnchorHold!=held.hold) { sourceAnchorHold=held.hold; sourceAnchor={}; }
      if (held.hold && scope.permission && pulse.seq && (!m1Integrated || pulse.callbackTicks>compactAnchorAfter) &&
          (!heldRecovery.newSession ||
           grs::g13vk::SourcePulseAfter(pulse,heldRecovery.newSession,heldRecovery.boundary))) {
        if (grs::g13vk::AdjacentSourcePulses(sourceAnchor,pulse) && pulse.uiNs>held.timeNs &&
            (!m1Integrated || (compactShown.mapped && held.transformRevision==compactShown.revision)))
          offer={scope,++offerSeq,ns,ticks,sampleTicks,hz,held,1,0,pulse.uiNs-sourceAnchor.uiNs,sourceAnchor,pulse};
        sourceAnchor=pulse;
      }
    }
    if (offer.seq) ingress.OfferLatest(offer);
    auto captureScope=offer.scope;
    captureScope.permission=0;
    const auto request=captureScope.run;
    if (!(captureHot || captureCold) || !request || captureRun==request) return;
    if (rootCapture && !m1Shown.load(std::memory_order_relaxed)) return;
    using Code=grs::g13vk::M1HotCapture::Code;
    const auto state=hotCapture.Reserve(captureScope,request,ns);
    if (state==Code::Reserved && captureReserved!=request) {
      captureReserved=request;
      coldOffset=0;
      Emit("{\"stage\":\"M1_HOT_CAPTURE_RESERVED\",\"request\":\""+std::to_string(request)+
           "\",\"requestNs\":\""+std::to_string(ns)+"\"}");
    } else if (state==Code::Cancelled || state==Code::TimedOut || state==Code::Failed) {
      captureRun=request;
      Emit("{\"stage\":\"M1_HOT_CAPTURE_TERMINAL\",\"request\":\""+std::to_string(request)+
           "\",\"state\":\""+(state==Code::Cancelled ? "CANCELLED" :
                                   state==Code::TimedOut ? "TIMED_OUT" : "FAILED")+"\"}");
    }
    if (captureCold && (state==Code::ColdFrozen || state==Code::ColdRead)) {
      std::array<uint8_t,grs::g13vk::M1HotCapture::kPageLimit> bytes;
      if (const auto page=hotCapture.ReadCold(captureScope,request,coldOffset,bytes.data(),bytes.size(),ns)) {
        Emit("{\"stage\":\"M1_COLD_PAGE\",\"request\":\""+std::to_string(request)+
             "\",\"root\":\""+std::to_string(page->lineage.busyRoot)+"\",\"offset\":"+
             std::to_string(page->offset)+",\"count\":"+std::to_string(page->count)+",\"total\":"+
             std::to_string(page->total)+",\"sha\":\""+page->sha+"\"}");
        coldOffset+=page->count;
        if (coldOffset==page->total) {
          hotCapture.End(captureScope,request);
          captureRun=request;
        }
      }
    }
    if (!captureCold && state==Code::Frozen) {
      if (const auto page=hotCapture.Read(captureScope,request,ns)) {
        std::string row="{\"stage\":\"M1_HOT_CAPTURE\",\"request\":\""+std::to_string(request)+
          "\",\"root\":\""+std::to_string(page->busyRoot)+"\",\"requestedNs\":\""+
          std::to_string(page->requestedNs)+"\",\"frozenNs\":\""+std::to_string(page->frozenNs)+
          "\",\"safePoint\":\""+std::to_string(page->safePoint)+"\",\"blocker\":"+
          std::to_string(int(page->blocker))+",\"completion\":[";
        for (size_t i=0;i<page->completion.size();++i) {
          if (i) row+=',';
          row+=std::to_string(page->completion[i]);
        }
        hotCapture.End(captureScope,request);
        Emit(row+"]}");
        captureRun=request;
      }
    }
  }

  void Close() {
    std::lock_guard lock(mutex);
    closing = true;
    CancelHeld();
    ingress.Authorize({},0);
    hotCapture.Authorize({});
    m1Pulses.Close();
    wake.notify_one();
    d1Ready.notify_all();
  }

  void Owner() {
    uint64_t previous = 0;
    std::unique_lock lock(mutex);
    for (;;) {
      wake.wait(lock, [&] { return closing || (generation != previous && width > 0 && height > 0); });
      if (closing) break;
      previous = generation;
      m1Shown.store(0,std::memory_order_relaxed);
      const auto snapshotDpi = dpi;
      const auto snapshotWidth = width, snapshotHeight = height;
      lock.unlock();
      auto current = [&] {
        std::lock_guard guard(mutex);
        return !closing && generation == previous && t833Allowed;
      };
      MONITORINFOEXW display{};
      display.cbSize = sizeof(display);
      DEVMODEW mode{};
      mode.dmSize = sizeof(mode);
      const auto monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
      const bool haveMode = GetMonitorInfoW(monitor, &display) &&
                            EnumDisplaySettingsW(display.szDevice, ENUM_CURRENT_SETTINGS, &mode);
      Emit("{\"stage\":\"WINDOW_AVAILABLE\",\"geometry\":\"" + std::to_string(previous) +
           "\",\"hwnd\":\"" + std::to_string(reinterpret_cast<uintptr_t>(hwnd)) +
           "\",\"width\":" + std::to_string(snapshotWidth) + ",\"height\":" +
           std::to_string(snapshotHeight) + ",\"dpi\":" + std::to_string(snapshotDpi) +
           ",\"modeHz\":" + (haveMode ? std::to_string(mode.dmDisplayFrequency) : "null") + "}");
      const auto terminal = grs::g13vk::RunBoot({previous,1,previous,hwnd,
                                               m1PrivateReference.empty() && (!bootOnly || m1Present),current,steady,
                                               steadyRole, disableOptimization, m1SourceModule,
                                               [&] { return sourcePulse.load(std::memory_order_relaxed); },
                                               m1QuantumCandidate, m1QuantumCommit, m1ColdFile,
                                               [&](const std::string &readyReceipt) {
                                                 {
                                                   std::lock_guard guard(mutex);
                                                   if (closing || generation != previous) return false;
                                                   Emit(readyReceipt);
                                                   pointerReadyQpc=grs::pc::QpcTicks();
                                                   pointerReady = true;
                                                   m1IntakeState=M1IntakeState::Normal;
                                                   PostMessageW(hwnd,kM1GravityCaption,0,0);
                                                   if (!d1Mode.empty() && !d1ReadyNs) {
                                                     d1ReadyScope=scope;
                                                     d1ReadyNs=grs::pc::TimeNs();
                                                     d1ReadyTime=std::chrono::steady_clock::now();
                                                     d1Intake.Start();
                                                   }
                                                   d1Ready.notify_one();
                                                 }
                                                 return true;
                                               },
                                               [&] {
                                                 std::lock_guard guard(mutex);
                                                 return !closing && generation == previous ? held :
                                                     grs::g13vk::PointerSample{};
                                               },
                                               [&](uint64_t hold, const std::function<void()> &accept) {
                                                 std::lock_guard guard(mutex);
                                                 if (closing || generation != previous || !t833Allowed ||
                                                     (hold && held.hold != hold))
                                                   return false;
                                                 accept();
                                                 return true;
                                               }, once, t849WriteFile, t854PrivateWrites,
                                               [&](grs::g13vk::SourceScope next) {
                                                 std::lock_guard guard(mutex);
                                                 if (closing || generation != previous) return;
                                                 CancelHeld();
                                                 scope=next;
                                                 ingress.Authorize(scope,0);
                                                 hotCapture.Authorize(next);
                                               }, &ingress, (captureHot || captureCold || D1CaptureEnabled()) ? &hotCapture : nullptr,
                                               dualSource,captureCold,t833CandidateFile,t833RevokeAt,[&] {
                                                 std::lock_guard guard(mutex);
                                                 t833Allowed=false;
                                               },t833Case,t833ConfigFile,t833SeedFile,t833ForcedJ,rootCapture,d1ReplayPrefix,
                                               !d1Mode.empty(),[&](uint64_t after) {
                                                 std::lock_guard guard(mutex);
                                                 std::vector<grs::pc::CandidatePulse> samples;
                                                 const auto count=std::min(g13PulseWrite,g13PulseHistory.size());
                                                 for (size_t i=g13PulseWrite-count;i<g13PulseWrite;++i) {
                                                   const auto pulse=g13PulseHistory[i%g13PulseHistory.size()];
                                                   if (pulse.sequence>after) samples.push_back(pulse);
                                                 }
                                                 return samples;
                                               },t833F05Rest,&m1Pulses,[&] {
                                                 std::lock_guard guard(mutex);
                                                 if (generation!=previous) return;
                                                 pointerReady=false;
                                                 CancelHeld();
                                               },sourceTrace,t877Mode,&d1Intake,[&](const std::string &details,bool healthy) {
                                                 ReportD1(details,healthy);
                                               },m1Preview || (!d1Mode.empty() && !d1ColdReanchor),[&] {
                                                 std::lock_guard guard(mutex);
                                                 return m1GravityEnabled && !closing && previous==generation ?
                                                     gravity : grs::pc::M1Gravity{};
                                               },observation,m1OneQuantumGrant,t899SlotSnapshot,
                                               m1GravityEnabled ? std::function<void(grs::pc::M1GravityDisplay)>(
                                                   [&](grs::pc::M1GravityDisplay value) { SetGravityDisplay(value); }) : nullptr,
                                               m1Kappa,m1StageTiming,g13TailTiming,m1InventoryV1,g13SealV1,m1AngleWitness,
                                               [&](uint64_t session) {
                                                 std::lock_guard guard(mutex);
                                                 if (!closing && generation==previous) SuspendHeld(session);
                                               },
                                               [&](uint64_t session,uint64_t retired,const grs::g13vk::M1Pulse &baseline) {
                                                 std::lock_guard guard(mutex);
                                                 if (!closing && generation==previous) ResumeHeld(session,retired,baseline);
                                               },
                                               [&](const grs::g13vk::SourceOffer &offer,const std::function<void()> &act) {
                                                 std::lock_guard guard(mutex);
                                                 if (closing || generation!=previous || !t833Allowed || !pointerReady ||
                                                     !scope.permission || !(offer.scope==scope) || offer.sample.hold!=held.hold ||
                                                     offer.sample.material!=held.material ||
                                                     (m1Integrated && (!compactShown.mapped ||
                                                       offer.sample.transformRevision!=compactShown.revision)) ||
                                                     (heldRecovery.newSession && offer.current.session!=heldRecovery.newSession))
                                                   return false;
                                                 act();
                                                 return true;
                                               },g13CompileTrace,m1Compact,m1Integrated,m1AuditBreakdown,
                                               m1PrivateReference,m1PrivateOutput,m1PrivateObserve,
                                               [&](grs::pc::CompactView &view) {return SetCompactPresented(view);},
                                               [&](const grs::g13vk::SourceOffer &offer,uint32_t reason,bool admitted) {
                                                 CompactSourceResult(offer,reason,admitted);
                                               },g13SplitOp48,m1ParallelInventory,g13SealStages,g13CycleScope},
          [&](const std::string &report) {
            Emit(report);
            const bool retiring=report.find("\"stage\":\"M1_RETIRE_PENDING\"")!=std::string::npos;
            const bool retireFailed=report.find("\"stage\":\"M1_RETIRE_FAILED\"")!=std::string::npos;
            if (retiring || retireFailed) {
              std::lock_guard guard(mutex);
              if (generation==previous && !closing) {
                m1IntakeState=retiring ? M1IntakeState::Retiring : M1IntakeState::Fault;
                PostMessageW(hwnd,kM1GravityCaption,0,0);
              }
            }
            if (report.find("\"state\":\"M1_QUANTUM_PRESENT_QUEUED\"")!=std::string::npos)
              m1Shown.fetch_add(1,std::memory_order_relaxed);
            if (report.find("\"state\":\"BOOT_DIAGNOSTIC_PRESENT_QUEUED\"") != std::string::npos ||
                report.find("\"state\":\"M1_DRY_SOURCE_PRESENT_QUEUED\"") != std::string::npos ||
                report.find("\"state\":\"M1_QUANTUM_PRESENT_QUEUED\"") != std::string::npos) {
              result = 0;
              PostMessageW(hwnd, kFirstPresent, 0, 0);
            }
            return current();
          });
      if (!m1SourceModule.empty()) {
        std::lock_guard guard(mutex);
        if (generation == previous) {
          pointerReady = false;
          CancelHeld();
          ingress.Authorize({},0);
          hotCapture.Authorize({});
          m1Pulses.Close();
        }
      }
      Emit(terminal);
      if (!m1PrivateReference.empty()) {
        result=terminal.find("\"state\":\"M1_PRIVATE_OFF_COMPLETE\"")!=std::string::npos ? 0 : 1;
        break;
      }
      if (!d1Mode.empty()) {
        d1Intake.Await();
        ReportD1("\"state\":\"OWNER_TERMINATED_BEFORE_DRAIN_PROOF\"",false);
        std::unique_lock sink(output);
        outputReady.wait_for(sink,std::chrono::seconds(1),[&] { return outputFinalWritten; });
        break;
      }
      const char *qualified = !t833CandidateFile.empty() ?
                              (t833RevokeAt.empty() ? (t833Case=="F02" ?
                               "\"state\":\"T833_PRIVATE_PROJECT_CAPTURED\"" :
                               "\"state\":\"T833_CANDIDATE_CAPTURED\"") :
                               "\"state\":\"T833_CANDIDATE_REVOKED\"") :
                              t854PrivateWrites ? "\"state\":\"T854_PRIVATE_WRITES_CAPTURED\"" :
                              !t849WriteFile.empty() ? "\"state\":\"T849_PRIVATE_WRITES_CAPTURED\"" :
                              m1Present ? "\"state\":\"M1_DRY_SOURCE_PRESENT_RETIRED\"" :
                              !m1SourceModule.empty() ? "\"state\":\"M1_DRY_SOURCE_ACCEPTED\"" :
                              !steadyRole.empty() ? "\"state\":\"STEADY_ROLE_QUALIFIED\"" :
                              steady ? "\"state\":\"STEADY_ROLES_QUALIFIED\"" :
                                      "\"state\":\"BOOT_NUMERIC_QUALIFIED\"";
      if (bootOnly && terminal.find(qualified) != std::string::npos) result = 0;
      const bool failed = terminal.find("\"state\":\"FAILED\"") != std::string::npos ||
                          terminal.find("\"state\":\"WSI_FAILED\"") != std::string::npos;
      if (bootOnly || (once && failed)) break;
      lock.lock();
    }
    PostMessageW(hwnd, kOwnerFinished, 0, 0);
  }
};

LRESULT CALLBACK Dispatch(HWND hwnd, UINT message, WPARAM w, LPARAM l) {
  auto *window = reinterpret_cast<Window *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
  if (message == WM_NCCREATE) {
    window = static_cast<Window *>(reinterpret_cast<CREATESTRUCTW *>(l)->lpCreateParams);
    window->hwnd = hwnd;
    SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
  }
  if (!window) return DefWindowProcW(hwnd, message, w, l);
  switch (message) {
  case WM_SIZE: window->Size(); return 0;
  case WM_DPICHANGED: {
    const auto &r = *reinterpret_cast<RECT *>(l);
    SetWindowPos(hwnd, nullptr, r.left, r.top, r.right-r.left, r.bottom-r.top, SWP_NOACTIVATE | SWP_NOZORDER);
    window->Size();
    return 0;
  }
  case WM_LBUTTONDOWN:
    window->gravityClick=window->ResetGravityButton(GET_X_LPARAM(l),GET_Y_LPARAM(l));
    SetCapture(hwnd);
    if (!window->gravityClick) window->Input("DOWN", GET_X_LPARAM(l), GET_Y_LPARAM(l));
    return 0;
  case WM_MOUSEMOVE:
    if ((w & MK_LBUTTON) && !window->gravityClick) window->Input("MOVE", GET_X_LPARAM(l), GET_Y_LPARAM(l));
    return 0;
  case WM_LBUTTONUP:
    if (!window->gravityClick) window->Input("UP", GET_X_LPARAM(l), GET_Y_LPARAM(l));
    ReleaseCapture();
    window->gravityClick=false;
    return 0;
  case WM_KEYDOWN:
    if (window->m1GravityEnabled && (w==VK_LEFT || w==VK_RIGHT || w==VK_HOME || w=='R')) {
      const auto step=window->m1Compact ? grs::pc::kCompactAngleStep : grs::pc::kM1AngleStep;
      window->SetGravity(w==VK_LEFT ? -step : w==VK_RIGHT ? step : 0,
                         0,w==VK_LEFT || w==VK_RIGHT);
      return 0;
    }
    if (window->dualSource && (w=='1' || w=='2')) {
      window->SelectMaterial(w=='2' ? 1u : 0u);
      return 0;
    }
    break;
  case kSetM1Gravity:
    window->SetGravity(static_cast<int32_t>(w),static_cast<uint64_t>(l));
    return 0;
  case kM1GravityCaption: window->UpdateGravityCaption(); return 0;
  case WM_CANCELMODE: window->Input("CANCEL", 0, 0); if (GetCapture() == hwnd) ReleaseCapture(); return 0;
  case WM_CAPTURECHANGED:
    if (!window->gravityClick) window->Input("CAPTURE_LOST", 0, 0);
    window->gravityClick=false;
    return 0;
  case WM_KILLFOCUS: window->Input("FOCUS_LOST", 0, 0); if (GetCapture() == hwnd) ReleaseCapture(); return 0;
  case WM_CLOSE: window->Close(); return 0;
  case kFirstPresent:
    if (window->once) SetTimer(hwnd, 1, 700, nullptr);
    return 0;
  case WM_TIMER:
    if (w == kSourceProbeTimer) {
      if (window->d1Mode.empty()) window->Pulse();
      window->DrainSourceTrace();
      return 0;
    }
    KillTimer(hwnd, 1);
    window->Close();
    return 0;
  case kOwnerFinished:
    window->DrainSourceTrace();
    if (!window->d1Mode.empty())
      window->Emit("{\"stage\":\"M1_D1_CLOSE\",\"qpc\":\""+std::to_string(grs::pc::QpcTicks())+"\"}");
    DestroyWindow(hwnd);
    return 0;
  case WM_DESTROY: PostQuitMessage(0); return 0;
  }
  return DefWindowProcW(hwnd, message, w, l);
}
}

int wmain(int argc, wchar_t **argv) {
  Window window;
  for (int i = 1; i < argc; ++i) {
    if (!std::wcscmp(argv[i], L"--once")) window.once = true;
    else if (!std::wcscmp(argv[i], L"--boot-only")) window.bootOnly = true;
    else if (!std::wcscmp(argv[i], L"--qualify-steady")) {
      window.bootOnly = true;
      window.steady = true;
    }
    else if (!std::wcscmp(argv[i], L"--qualify-steady-role") && i+1 < argc) {
      const std::wstring name(argv[++i]);
      window.steadyRole.assign(name.begin(), name.end());
      window.bootOnly = window.steady = true;
    }
    else if (!std::wcscmp(argv[i], L"--disable-optimization")) {
      window.disableOptimization = true;
    }
    else if (!std::wcscmp(argv[i], L"--qualify-t849-writes") && i+1 < argc) {
      window.t849WriteFile = argv[++i];
      window.bootOnly = window.steady = true;
    }
    else if (!std::wcscmp(argv[i], L"--qualify-t854-writes") && i+1 < argc) {
      window.t849WriteFile = argv[++i];
      window.t854PrivateWrites = true;
      window.bootOnly = window.steady = true;
    }
    else if (!std::wcscmp(argv[i],L"--g13-candidate") && i+1 < argc) {
      window.t833CandidateFile=argv[++i];
      window.bootOnly=true;
    }
    else if (!std::wcscmp(argv[i],L"--g13-revoke-before")) window.t833RevokeAt="before";
    else if (!std::wcscmp(argv[i],L"--g13-revoke-after")) window.t833RevokeAt="after";
    else if (!std::wcscmp(argv[i],L"--g13-fixture") && i+3 < argc) {
      const std::wstring name=argv[++i];
      if (name!=L"F02" && name!=L"F03" && name!=L"F05") return 2;
      window.t833Case.assign(name.begin(),name.end());
      window.t833ConfigFile=argv[++i];
      window.t833SeedFile=argv[++i];
    }
    else if (!std::wcscmp(argv[i],L"--g13-trial") && i+1 < argc) {
      const std::wstring trial=argv[++i];
      if (trial!=L"1" && trial!=L"2" && trial!=L"4") return 2;
      window.t833ForcedJ=uint32_t(trial[0]-L'0');
    }
    else if (!std::wcscmp(argv[i],L"--g13-f05-rest")) window.t833F05Rest=true;
    else if (!std::wcscmp(argv[i],L"--g13-diagnostics") && i+1<argc) {
      const std::wstring mode=argv[++i];
      if (mode!=L"full" && mode!=L"hot") return 2;
      window.t877Mode=mode==L"full" ? "full" : "hot";
    }
    else if (!std::wcscmp(argv[i], L"--m1-window") && i+3 < argc) {
      window.m1SourceModule = argv[++i];
      window.m1QuantumCandidate = argv[++i];
      window.m1QuantumCommit = argv[++i];
      window.m1Present = true;
    }
    else if ((!std::wcscmp(argv[i],L"--m1-cold") || !std::wcscmp(argv[i],L"--m1-cold-on-busy")) && i+1 < argc) {
      window.captureCold=!std::wcscmp(argv[i],L"--m1-cold-on-busy");
      window.m1ColdFile = argv[++i];
    }
    else if (!std::wcscmp(argv[i],L"--m1-root-on-busy") && i+1 < argc) {
      window.rootCapture=window.captureCold=true;
      window.m1ColdFile=argv[++i];
    }
    else if (!std::wcscmp(argv[i], L"--m1-capture-hot")) window.captureHot=true;
    else if (!std::wcscmp(argv[i], L"--m1-dual-source")) window.dualSource=true;
    else if (!std::wcscmp(argv[i], L"--m1-empty-preview")) window.m1Preview=true;
    else if (!std::wcscmp(argv[i], L"--m1-compact-demo")) {
      window.m1Compact=true;
      window.m1Preview=true;
      window.m1GravityEnabled=true;
    }
    else if (!std::wcscmp(argv[i], L"--m1-manual-gravity")) window.m1GravityEnabled=true;
    else if (!std::wcscmp(argv[i], L"--m1-one-quantum-grant")) window.m1OneQuantumGrant=true;
    else if (!std::wcscmp(argv[i], L"--g13-slot-snapshot")) window.t899SlotSnapshot=true;
    else if (!std::wcscmp(argv[i], L"--g13-compile-trace")) window.g13CompileTrace=true;
    else if (!std::wcscmp(argv[i], L"--g13-tail-timing")) window.g13TailTiming=true;
    else if (!std::wcscmp(argv[i], L"--g13-seal-v1")) window.g13SealV1=true;
    else if (!std::wcscmp(argv[i],L"--g13-seal-stages")) window.g13SealStages=true;
    else if (!std::wcscmp(argv[i],L"--g13-cycle-scope") && i+1<argc) {
      const std::wstring scope=argv[++i];
      if(scope!=L"actual" && scope!=L"replay") return 2;
      window.g13CycleScope.assign(scope.begin(),scope.end());
    }
    else if (!std::wcscmp(argv[i], L"--g13-split-op48")) window.g13SplitOp48=true;
    else if (!std::wcscmp(argv[i], L"--m1-stage-timing")) window.m1StageTiming=true;
    else if (!std::wcscmp(argv[i],L"--m1-inventory-v1")) window.m1InventoryV1=true;
    else if (!std::wcscmp(argv[i],L"--m1-integrated")) window.m1Integrated=true;
    else if (!std::wcscmp(argv[i],L"--m1-parallel-inventory")) window.m1ParallelInventory=true;
    else if (!std::wcscmp(argv[i],L"--m1-audit-breakdown")) {
      window.m1Integrated=true;
      window.m1StageTiming=true;
      window.m1AuditBreakdown=true;
    }
    else if (!std::wcscmp(argv[i],L"--m1-private-off") && i+3<argc && window.m1PrivateReference.empty()) {
      window.m1PrivateReference=argv[++i];
      window.m1PrivateOutput=argv[++i];
      const std::wstring mode=argv[++i];
      if (mode!=L"OFF" && mode!=L"ON") return 2;
      window.m1PrivateObserve=mode==L"ON";
    }
    else if (!std::wcscmp(argv[i],L"--m1-angle-witness-v1")) window.m1AngleWitness=true;
    else if (!std::wcscmp(argv[i], L"--m1-observe-kappa-zero")) window.m1Kappa=0;
    else if (!std::wcscmp(argv[i],L"--m1-observe-birth") && i+2<argc) {
      if (window.observation.Enabled()) return 2;
      const std::wstring frames=argv[++i];
      if (frames.empty() || frames.size()>2 || frames.find_first_not_of(L"0123456789")!=std::wstring::npos)
        return 2;
      window.observation.birthFrames=uint32_t(std::stoul(frames));
      window.observation.archivePrefix=argv[++i];
      if (!window.observation.birthFrames || window.observation.birthFrames>12 ||
          window.observation.archivePrefix.empty()) return 2;
    }
    else if ((!std::wcscmp(argv[i], L"--m1-observe") || !std::wcscmp(argv[i], L"--m1-observe-series") ||
              !std::wcscmp(argv[i], L"--m1-observe-packet")) && i+2<argc) {
      if (window.observation.Enabled()) return 2;
      const bool packet=!std::wcscmp(argv[i], L"--m1-observe-packet");
      const bool series=packet || !std::wcscmp(argv[i],L"--m1-observe-series");
      std::ifstream request{std::filesystem::path(argv[++i])};
      if (!(request >> window.observation.frame) || !window.observation.frame ||
          window.observation.frame>1000000) return 2;
      if (series && (!(request >> window.observation.series) || !window.observation.series ||
                     window.observation.series>120)) return 2;
      if (packet) {
        if (!(request >> window.observation.packetId >> window.observation.packetGeneration) ||
            !window.observation.packetId || !window.observation.packetGeneration) return 2;
      } else {
        for (auto &r:window.observation.regions)
          if (!(request >> r.x >> r.y >> r.width >> r.height) || !grs::pc::Fits(r,INT_MAX,INT_MAX)) return 2;
      }
      std::string extra;
      if (request >> extra) return 2;
      window.observation.archivePrefix=argv[++i];
      if (window.observation.archivePrefix.empty()) return 2;
    }
    else if (!std::wcscmp(argv[i], L"--m1-source-trace")) window.sourceTrace=true;
    else if (!std::wcscmp(argv[i],L"--m1-d1") && i+1<argc) {
      window.d1Mode=argv[++i];
      window.sourceTrace=true;
    }
    else if (!std::wcscmp(argv[i],L"--m1-d1-replay-prefix") && i+1<argc) window.d1ReplayPrefix=argv[++i];
    else if (!std::wcscmp(argv[i],L"--m1-d1-cold-reanchor")) window.d1ColdReanchor=true;
    else if ((!std::wcscmp(argv[i], L"--probe-m1-source") ||
              !std::wcscmp(argv[i], L"--present-m1-source")) && i+1 < argc) {
      window.m1Present = !std::wcscmp(argv[i], L"--present-m1-source");
      window.m1SourceModule = argv[++i];
      window.bootOnly = true;
    }
    else {
      std::cerr << "usage: hello_world_pc [--once] [--boot-only | --qualify-steady | "
                   "--qualify-steady-role NAME [--disable-optimization] | --probe-m1-source SPV | "
                   "--present-m1-source SPV | --m1-window SOURCE_SPV CANDIDATE_SPV COMMIT_SPV "
                   "[--m1-cold FILE | --m1-cold-on-busy FILE | --m1-root-on-busy FILE] "
                   "[--m1-capture-hot] [--m1-source-trace] [--m1-d1 OFF|ON|REPLAY_OFF|REPLAY_ON] "
                   "[--m1-d1-replay-prefix PATH] "
                   "[--m1-dual-source] | --qualify-t849-writes FILE | --g13-candidate FILE "
                   "[--m1-empty-preview] [--m1-manual-gravity] [--m1-compact-demo] [--m1-observe REQUEST PREFIX] [--m1-observe-series REQUEST PREFIX] [--m1-observe-birth FRAMES PREFIX] "
                   "[--m1-one-quantum-grant] [--m1-stage-timing] [--m1-observe-kappa-zero] [--m1-inventory-v1] [--m1-angle-witness-v1] "
                   "[--g13-fixture F02|F03|F05 CONFIG STATE] [--g13-trial 1|2|4] [--g13-f05-rest] "
                   "[--g13-revoke-before | --g13-revoke-after] [--g13-diagnostics full|hot] [--g13-slot-snapshot] [--g13-tail-timing] [--g13-seal-v1]] [--g13-compile-trace]\n";
      return 2;
    }
  }
  if (window.disableOptimization && window.steadyRole.empty()) return 2;
  if (!window.t849WriteFile.empty() && !window.steadyRole.empty()) return 2;
  if (window.steady && !window.m1SourceModule.empty()) return 2;
  if (window.captureHot && window.m1QuantumCandidate.empty()) return 2;
  if (window.captureHot && window.captureCold) return 2;
  if (window.rootCapture && window.once) return 2;
  if ((window.m1Preview || window.m1GravityEnabled || window.observation.frame || window.m1OneQuantumGrant) &&
      (window.m1QuantumCandidate.empty() || !window.d1Mode.empty() || !window.m1ColdFile.empty() || window.once))
    return 2;
  const bool d1Replay=window.d1Mode==L"REPLAY_OFF" || window.d1Mode==L"REPLAY_ON";
  if (d1Replay!=!window.d1ReplayPrefix.empty()) return 2;
  if (!window.d1Mode.empty() && ((!d1Replay && window.d1Mode!=L"OFF" && window.d1Mode!=L"ON") ||
      window.m1QuantumCandidate.empty() || window.dualSource || window.once ||
      window.captureHot || !window.m1ColdFile.empty())) return 2;
  if (!window.t833RevokeAt.empty() && window.t833CandidateFile.empty()) return 2;
  if (!window.t833Case.empty() && window.t833CandidateFile.empty()) return 2;
  if (window.g13SplitOp48 && window.t833CandidateFile.empty()) return 2;
  if (window.t833ForcedJ && window.t833Case!="F03") return 2;
  if (window.t833F05Rest && window.t833Case!="F05") return 2;
  if (!window.t877Mode.empty() && window.t833CandidateFile.empty()) return 2;
  if ((window.g13TailTiming || window.g13SealV1) && window.t877Mode.empty()) return 2;
  if ((window.g13SealStages || !window.g13CycleScope.empty()) &&
      (window.t877Mode!="hot" || !window.g13TailTiming ||
       (window.t833Case!="F02" && window.t833Case!="F03"))) return 2;
  if (window.g13SealStages && window.g13SealV1) return 2;
  if ((window.m1StageTiming || window.m1InventoryV1) && window.m1QuantumCandidate.empty()) return 2;
  if (window.m1AngleWitness && (window.m1QuantumCandidate.empty() || window.m1InventoryV1)) return 2;
  if (window.m1Integrated && !window.m1PrivateReference.empty()) window.m1Compact=true;
  if (window.m1Compact && (window.m1AngleWitness || (window.observation.packetId && !window.m1Integrated))) return 2;
  if (window.m1Integrated && (!window.m1Compact || window.m1InventoryV1 || !window.d1Mode.empty())) return 2;
  if (window.m1ParallelInventory && (!window.m1Integrated || window.m1QuantumCandidate.empty())) return 2;
  if (window.observation.birthFrames && (!window.m1Integrated || !window.m1Preview ||
      window.m1QuantumCandidate.empty() || !window.d1Mode.empty() || window.captureHot ||
      !window.m1ColdFile.empty() || !window.d1ReplayPrefix.empty())) return 2;
  if (!window.m1PrivateReference.empty() &&
      (window.m1QuantumCandidate.empty() || window.m1Preview || window.dualSource ||
       !window.m1ColdFile.empty() || !window.d1Mode.empty() || window.captureHot ||
       window.observation.Enabled() || window.m1InventoryV1 || window.m1AngleWitness ||
       window.m1GravityEnabled || window.once || window.m1PrivateOutput.empty())) return 2;
  if (window.observation.series && !window.d1Mode.empty()) return 2;
  if (window.observation.packetId && !window.dualSource) return 2;
  if (window.m1Kappa==0 && (!window.dualSource || !window.observation.frame)) return 2;
  if (window.t899SlotSnapshot && window.t877Mode.empty()) return 2;
  if (!window.t833CandidateFile.empty() && (window.steady || !window.m1SourceModule.empty())) return 2;
  if (window.sourceTrace) window.ingress.EnableTrace();
  if (window.d1ColdReanchor && window.d1Mode.empty()) return 2;
  if (!window.d1Mode.empty()) {
    window.m1Pulses.Observe(grs::pc::QpcTicks,[&](const grs::g13vk::M1PulseQueue::Terminal &e) {
      using Outcome=grs::g13vk::M1PulseQueue::Outcome;
      const auto &p=e.pulse;
      std::ostringstream row;
      row << "{\"stage\":\"M1_D1_PULSE_OWNER\",\"actionId\":" << p.action
          << ",\"run\":\"" << e.scope.run << "\",\"geometry\":\"" << e.scope.geometry
          << "\",\"session\":\"" << p.session << "\",\"seq\":\"" << p.seq
          << "\",\"callbackQpc\":\"" << p.callbackTicks << "\",\"nativeReceiptQpc\":\"" << p.receiptTicks
          << "\",\"ownerQpc\":\"" << e.qpc << "\",\"outcome\":\""
          << (e.outcome==Outcome::Accepted ? "ACCEPTED" : e.outcome==Outcome::Rejected ? "REJECTED" : "REVOKED")
          << "\",\"state\":\"" << e.reason << '"';
      if (e.clockObserved) row << ",\"generated\":" << e.generated << ",\"pending\":" << e.pending
                               << ",\"carry\":" << e.carry << ",\"overload\":" << e.overload;
      window.Emit(row.str()+"}");
    });
  }
  if (window.dualSource && (window.m1QuantumCandidate.empty() || !window.m1ColdFile.empty())) return 2;
  if ((!window.m1ColdFile.empty() && window.m1QuantumCandidate.empty()) ||
      (!window.m1QuantumCandidate.empty() && window.bootOnly)) return 2;
  SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
  HINSTANCE instance = GetModuleHandleW(nullptr);
  WNDCLASSW type{};
  type.lpfnWndProc = Dispatch;
  type.hInstance = instance;
  type.lpszClassName = L"HelloWorldPcVulkanBoot";
  type.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
  if (!RegisterClassW(&type)) return 3;
  HWND hwnd = CreateWindowExW(0, type.lpszClassName,
      window.m1QuantumCandidate.empty() ? L"HelloWorld Vulkan BOOT diagnostic" : L"HelloWorld Vulkan M1",
      WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, window.m1Compact ? 640 : 800,
      window.m1Compact ? 470 : 500, nullptr, nullptr, instance, &window);
  if (!hwnd) return 4;
  if ((!window.m1SourceModule.empty() || !window.t833CandidateFile.empty()) &&
      !SetTimer(hwnd,kSourceProbeTimer,10,nullptr)) return 5;
  if (window.m1PrivateReference.empty() && (!window.bootOnly || window.m1Present)) ShowWindow(hwnd, SW_SHOW);
  window.Size();
  std::thread logger([&] { window.WriteLogs(); });
  std::thread owner([&] { window.Owner(); });
  std::thread d1;
  if (!window.d1Mode.empty()) d1=std::thread([&] { window.RunD1(); });
  MSG message{};
  while (GetMessageW(&message, nullptr, 0, 0) > 0) {
    TranslateMessage(&message);
    DispatchMessageW(&message);
  }
  window.Close();
  owner.join();
  if (d1.joinable()) d1.join();
  {
    std::lock_guard lock(window.output);
    window.outputDone=true;
    window.outputReady.notify_one();
  }
  logger.join();
  return window.result;
}
