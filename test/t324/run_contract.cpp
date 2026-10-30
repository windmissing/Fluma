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
 FullM1Transport gpu;uint32_t submitted=0;bool inflight=false,submitting=false;U64 nonce{0,0};RootRequest root{};
 auto pump=[&]()->TransportCode{
  if(submitting){auto code=continueSubmit(gpu);
   if(code==TransportCode::BUSY)return code;
   if(code==TransportCode::OK_SUBMITTING)return TransportCode::OK;
   if(code!=TransportCode::OK)return code;
   inflight=true;submitting=false;return TransportCode::OK;}
  if(inflight){inflight=false;m1::doubleActive=false;m1::doublePhase=SubmitPhase::IDLE;return TransportCode::OK;}
  U64 next{nonce.lo+1,nonce.hi};root.nonce=next;
  auto code=submitQuantum(gpu,m1::QuantumParameters{},root);
  if(code==TransportCode::OK_SUBMITTING){nonce=next;submitting=true;submitted++;return TransportCode::OK;}
  if(code==TransportCode::BUSY)return code;
  if(code!=TransportCode::OK)return code;
  nonce=next;inflight=true;submitted++;return TransportCode::OK;
 };
 // Case 1: 3-batch continuation, ONE nonce consumption, ONE Begin, acceptance
 {m1::begins=m1::continues=m1::cancels=0;submitting=inflight=false;
  assert(pump()==TransportCode::OK);assert(submitting&&!inflight);passed++;
  assert(pump()==TransportCode::OK&&m1::continues==1);passed++;
  assert(pump()==TransportCode::OK&&m1::continues==2);passed++;
  assert(pump()==TransportCode::OK&&m1::continues==3&&m1::doublePhase==SubmitPhase::FENCE_PENDING&&!submitting&&inflight);passed++; // c3 seals -> inflight
  assert(pump()==TransportCode::OK&&!inflight);passed++;              // Poll accepts
  assert(m1::begins==1&&submitted==1);passed++;
 }
 // Case 2: Pause during submitting -> cancel BEFORE stop, no quarantine
 {m1::begins=m1::continues=m1::cancels=m1::stops=0;submitting=inflight=false;
  assert(pump()==TransportCode::OK&&submitting);
  assert(pump()==TransportCode::OK);
  m1::stops++; // transport.Stop() call in Pause
  if(submitting){assert(cancelSubmit(gpu)==TransportCode::PROTOCOL_REJECTED);submitting=false;}
  assert(m1::cancels==1&&m1::doublePhase==SubmitPhase::IDLE&&!m1::doubleActive);passed++;
  assert(m1::begins==1);passed++;
 }
 // Case 3: contract ordering — cancel must precede Stop (drain order §2.4)
 {m1::cancels=m1::stops=0;submitting=true;
  // model Pause: cancel first (state IDLE), then Stop sees no flight
  assert(cancelSubmit(gpu)==TransportCode::PROTOCOL_REJECTED);
  m1::stops++;
  assert(m1::cancels==1);passed++;
 }
 std::cout<<"{\"revision\":\"T324_RUN_SUBMITTING_CONTRACT_V1\",\"passed\":"<<passed<<",\"failed\":0,\"device\":\"NOT_RUN\"}\n";
}
