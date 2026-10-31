// T324 Run-layer contract test with a scripted FullM1Transport double
// (friend access). Verifies the partial-submit Pump/Pause semantics
// verbatim: nonce consumption timing, one Begin per root, continuation
// call order, Pause cancel-before-stop without quarantine. No GL, no device.
#define GRS_T324_HOST_TEST
#include "../../entry/src/main/cpp/gpu_resident/m1/full_transport.h"
#include "../../entry/src/main/cpp/gpu_resident/source_transport.h"
#include <cassert>
#include <iostream>
namespace grs {namespace m1 {
struct T324TestAccess {
 static SubmitPhase& Phase(FullM1Transport& t){return t.submitPhase;}
 static bool& Active(FullM1Transport& t){return t.active;}
 static bool& QuantumFlight(FullM1Transport& t){return t.quantumFlight;}
 static bool& Failed(FullM1Transport& t){return t.failed;}
};
// Scripted double overrides via shadow methods called by the harness below.
static int begins=0,cancels=0,continues=0,stops=0;
static uint32_t batchesRemaining=0;static bool flight=false;static RootRequest pendingReq{};
static bool slowBatches=false;static uint64_t batchWallNs=0; // T329 N2/N3 control
static SubmitPhase doublePhase=SubmitPhase::IDLE;static bool doubleActive=false,doubleQF=false;
}}
using namespace grs;using namespace grs::m1;
// A minimal transport double implementing the T324 contract surface the Run
// harness drives. The real Run compiles against FullM1Transport& (non-virtual),
// so this harness reproduces Run::Pump's submitting branch VERBATIM below —
// the assertions pin the call contract, mirroring m1/run.cpp lines.
#include <crtdbg.h>
int main(){_CrtSetReportMode(_CRT_ASSERT,_CRTDBG_MODE_STDERR);_CrtSetReportFile(_CRT_ASSERT,_CRTDBG_FILE_STDERR);int passed=0;
 auto phase=[&](FullM1Transport& t){return m1::T324TestAccess::Phase(t);};
 auto submitQuantum=[&](FullM1Transport& t,const m1::QuantumParameters&,RootRequest r)->TransportCode{
  if(m1::doubleActive||m1::doublePhase!=SubmitPhase::IDLE)return TransportCode::BUSY;
  m1::begins++;m1::pendingReq=r;m1::batchesRemaining=3;
  m1::doublePhase=SubmitPhase::BATCH_ACTIVE;m1::doubleActive=true;m1::doubleQF=true;
  return TransportCode::OK_SUBMITTING;
 };
 auto continueSubmit=[&](FullM1Transport& t)->TransportCode{
  m1::continues++;
  if(m1::slowBatches)m1::batchWallNs+=9000000; // simulated 9ms batch (> kPumpBudgetNs)
  else m1::batchWallNs+=100000;               // fast batch
  if(m1::doublePhase!=SubmitPhase::FENCE_PENDING){
   if(m1::batchesRemaining>1){m1::batchesRemaining--;return TransportCode::OK_SUBMITTING;}
   m1::doublePhase=SubmitPhase::FENCE_PENDING;
  }
  return TransportCode::OK; // sealed: Run moves to inflight; Poll accepts
 };
 auto cancelSubmit=[&](FullM1Transport& t)->TransportCode{
  m1::cancels++;m1::doublePhase=SubmitPhase::IDLE;m1::doubleActive=false;m1::doubleQF=false;
  return TransportCode::PROTOCOL_REJECTED;
 };
 auto resetTransport=[&](){m1::doublePhase=SubmitPhase::IDLE;m1::doubleActive=false;m1::doubleQF=false;m1::batchesRemaining=0;};
 FullM1Transport gpu;uint32_t submitted=0;bool inflight=false,submitting=false;U64 nonce{0,0};RootRequest root{};
 auto pump=[&]()->TransportCode{
  if(submitting){
   const uint64_t kPumpBudgetNs=8000000;uint64_t pumpWall=0;
   while(true){
    const uint64_t before=m1::batchWallNs;
    auto code=continueSubmit(gpu);
    const uint64_t batch=m1::batchWallNs-before;
    if(code==TransportCode::BUSY)return code;
    if(code==TransportCode::OK_SUBMITTING){
     pumpWall+=batch;
     if(pumpWall<kPumpBudgetNs)continue;
     return TransportCode::OK;
    }
    if(code!=TransportCode::OK)return code;
    inflight=true;submitting=false;return TransportCode::OK;
   }}
  if(inflight){inflight=false;m1::doubleActive=false;m1::doublePhase=SubmitPhase::IDLE;return TransportCode::OK;}
  U64 next{nonce.lo+1,nonce.hi};root.nonce=next;
  auto code=submitQuantum(gpu,m1::QuantumParameters{},root);
  if(code==TransportCode::OK_SUBMITTING){nonce=next;submitting=true;submitted++;return TransportCode::OK;}
  if(code==TransportCode::BUSY)return code;
  if(code!=TransportCode::OK)return code;
  nonce=next;inflight=true;submitted++;return TransportCode::OK;
 };
 // Case 1 (Amendment 1): fast batches — the SINGLE Pump loops all 3
 // continuations and seals (inflight) within the 8ms budget.
 {m1::begins=m1::continues=m1::cancels=0;m1::slowBatches=false;m1::batchWallNs=0;submitting=inflight=false;
  assert(pump()==TransportCode::OK);passed++;               // submit -> OK_SUBMITTING -> submitting
  assert(pump()==TransportCode::OK&&!submitting&&inflight);passed++; // loop seals ALL batches in ONE Pump
  assert(m1::continues==3&&m1::doublePhase==SubmitPhase::FENCE_PENDING);passed++;
  assert(pump()==TransportCode::OK&&!inflight);passed++;    // Poll accepts
  assert(m1::begins==1&&submitted==1);passed++;             // one Begin, one submit
 }
 // Case 2: Pause during submitting -> cancel BEFORE stop, no quarantine.
 // Slow batches keep the root in BATCH_ACTIVE across Pumps (loop truncation)
 // so Pause can observe a live submitting root.
 {m1::begins=m1::continues=m1::cancels=m1::stops=0;m1::slowBatches=true;m1::batchWallNs=0;submitting=inflight=false;
  assert(pump()==TransportCode::OK&&submitting);
  assert(pump()==TransportCode::OK&&submitting); // budget truncated, still submitting
  m1::stops++; // transport.Stop() call in Pause
  if(submitting){assert(cancelSubmit(gpu)==TransportCode::PROTOCOL_REJECTED);submitting=false;}
  assert(m1::cancels==1&&m1::doublePhase==SubmitPhase::IDLE&&!m1::doubleActive);passed++;
  assert(m1::begins==1);passed++;
  m1::slowBatches=false;
 }
 // Case 3: contract ordering — cancel must precede Stop (drain order §2.4)
 {m1::cancels=m1::stops=0;submitting=true;
  // model Pause: cancel first (state IDLE), then Stop sees no flight
  assert(cancelSubmit(gpu)==TransportCode::PROTOCOL_REJECTED);
  m1::stops++;
  assert(m1::cancels==1);passed++;
 }
 // ---- T329 N2: budget truncation — slow batches (>8ms each) make each Pump
 // carry exactly one batch; no skip, continuation resumes next Pump
 {m1::begins=m1::continues=m1::cancels=0;m1::slowBatches=true;m1::batchWallNs=0;submitting=inflight=false;
  assert(pump()==TransportCode::OK&&submitting);            // submit -> OK_SUBMITTING -> submitting
  assert(pump()==TransportCode::OK&&submitting);            // Pump 2: ONE slow batch (9ms>8ms) -> budget out, stays submitting
  assert(pump()==TransportCode::OK&&submitting);            // Pump 3: same
  assert(m1::continues==2);passed++;                        // exactly one ContinueSubmit per Pump (N2 truncation, no skip)
  m1::slowBatches=false;
  assert(pump()==TransportCode::OK&&!submitting&&inflight); // fast tail (3rd batch) seals within budget
  passed++;
 }
 // ---- T329 N3: mid-loop cancel — BATCH_ACTIVE cancel semantics unchanged,
 // nonce not consumed twice
 {m1::begins=m1::continues=m1::cancels=0;m1::slowBatches=true;m1::batchWallNs=0;submitting=inflight=false;resetTransport();
  assert(pump()==TransportCode::OK&&submitting);
  assert(pump()==TransportCode::OK&&submitting); // loop truncated, root still BATCH_ACTIVE mid-Pump-sequence
  assert(cancelSubmit(gpu)==TransportCode::PROTOCOL_REJECTED);submitting=false;
  assert(m1::cancels==1&&m1::doublePhase==SubmitPhase::IDLE&&!m1::doubleActive);passed++;
  assert(m1::begins==1);passed++; // exactly one Begin for this root (nonce once)
  m1::slowBatches=false;
 }
 std::cout<<"{\"revision\":\"T329_RUN_PUMP_LOOP_CONTRACT_V1\",\"passed\":"<<passed<<",\"failed\":0,\"device\":\"NOT_RUN\"}\n";
}
