#pragma once
#include "../submit_observer.h"
#include "quantum_candidate.h"
#include <chrono>
namespace grs {namespace m1 {
// T324 §2.1 partial-submit phases. IDLE: no root in progress. BATCH_ACTIVE:
// >=1 batch dispatched this root, table not exhausted, no fence yet.
// FENCE_PENDING: all passes dispatched, fence created, awaiting GPU signal.
enum class SubmitPhase:uint8_t {IDLE=0,BATCH_ACTIVE=1,FENCE_PENDING=2};
// Full M1 roots, still isolated from product routing/scene. All calls belong
// to the same current native GL owner. Only Poll can accept a sealed root.
class FullM1Transport {
public:
 SubmitObserver submitObserver{};
 TransportCode Create(uint32_t K,uint32_t Kw,const Identity&,const QuantumParameters& cold=QuantumParameters{},uint32_t emitQ=16u*1048576u);
 TransportCode SubmitSource(const std::array<uint32_t,32>&,RootRequest);
 TransportCode SubmitQuantum(const QuantumParameters&,RootRequest);
 // T324 §2.2/2.3: same-nonce continuation. BATCH_ACTIVE: dispatch the next
 // batch (one per call). FENCE_PENDING: fence wait only — OK leaves the
 // sealed root for the existing Poll path. Never re-Begins the root.
 TransportCode ContinueSubmit();
 // T324 §2.4: revoke an unsealed root. BATCH_ACTIVE: owner.Stop + unpin +
 // IDLE (dispatched GPU work cannot be rolled back; the root never reaches
 // inflight/accepted). FENCE_PENDING: glDeleteSync(fence) + same reset.
 TransportCode CancelSubmit();
 TransportCode Poll(AcceptedRoot&);ControlCode Stop();void ContextLost();TransportCode Release();
 ControlCode Acquire(uint32_t,U64,PublicationLease&,GLuint&);ControlCode ReleasePublication(uint32_t,U64,GLsync);
 TransportCode Freeze(bool);TransportCode Capture(std::vector<uint32_t>& bank,std::vector<uint32_t>& events);
 const SourceRootOwner& Owner()const{return source.Owner();}const Words256& LastCompletion()const{return tail;}
 uint64_t AllocatedBytes()const{return source.Stats().allocatedBytes+quantum.AllocatedBytes()+extraBytes;}
 SourceTransportStats Stats()const;
 const TransportTraceRing& Trace()const{return quantum.trace;} // last quantum root, reset before submission
 // T324 §4 telemetry (bounded, read-only).
 SubmitPhase Phase()const{return submitPhase;}
 uint32_t SubmitBatches()const{return batchesIssued;}
 uint32_t SubmitBatchPasses()const{return lastBatchPasses;}
 uint64_t SubmitBatchWallNs()const{return lastBatchWallNs;}
 uint64_t SubmitTotalWallNs()const{return submitFirstNs&&submitPhase!=SubmitPhase::IDLE?uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count())-submitFirstNs:submitTotalWallNs;}
 // T329 (Amendment 1 §2.5): wall clock of the CURRENT Pump's continuation
 // loop (last ContinueSubmit batch), separate from submitTotalWallNs (Begin
 // -> now). Used to verify the single-Pump seal expectation (J1).
 uint64_t SubmitPumpWallNs()const{return submitPumpWallNs;}
 uint32_t SubmitCancelled()const{return submitCancelled;}
private:
 friend struct FullM1HostAccess;
 // T324 §2.2 dual batch limit. Budget 8ms per contract. The op cap uses
 // TOP-LEVEL ops (a Sort counts as one op): the actual top-level count is 38
 // (not the contract's illustrative "27+"); 38/8=5 batches x ~8ms bridge
 // cadence = 40ms would exceed the unchanged 33ms anchor window and cancel
 // every root, so the cap is 10 ops/batch => 4 batches, matching the
 // contract's own "~4 batches inside one window" arithmetic. A single op
 // still owns its batch when it alone exceeds the budget (§5 boundary).
 static constexpr uint64_t kBatchBudgetNs=8000000;
 static constexpr uint32_t kBatchMaxOps=10;
 TransportCode ExecuteBatch();
 void FailSubmit();
 void TestHook(uint32_t stage){
#ifdef GRS_T280_HOST_TEST
  if(testHook)testHook(*this,stage);
#else
  (void)stage;
#endif
 }
#ifdef GRS_T280_HOST_TEST
  void (*testHook)(FullM1Transport&,uint32_t)=nullptr;
#endif
 SourceCoreTransport source;QuantumCandidate quantum;GLuint eventArenas[2]{},results[2]{};uint64_t extraBytes=0;uint32_t spareAt=0,replayFrozen=0;
 bool active=false,quantumFlight=false,failed=false,lost=false,frozen=false;RootRequest pending{};Words256 tail{};SourceTransportStats stats{};
 U64 lastFrame{},arenaFrame[2]{};uint32_t used[2]{},lastOrdinal=0,lastPublication=2;bool frameHasSource=false;
 QuantumParameters coldParams{};
 // T324 partial-submit state. submitPhase gates Submit*/Freeze/Release;
 // passCursor is the top-level op index into quantum's pass table;
 // tail uniforms are staged here and applied when the cursor crosses the
 // stage-20 boundary (original Set order preserved bit-for-bit).
 SubmitPhase submitPhase=SubmitPhase::IDLE;uint32_t passCursor=0,batchesIssued=0,lastBatchPasses=0,submitCancelled=0;
#ifdef GRS_T324_HOST_TEST
 // Test-double access to the state machine internals (host build only).
 friend struct T324TestAccess;
#endif
 uint64_t lastBatchWallNs=0,submitFirstNs=0,submitTotalWallNs=0,submitPumpWallNs=0;
 uint32_t tailSpare=0,tailReplay=0,tailBank=0,tailSlot=0,tailArena=0,tailCarry=0,tailDispatches=0;
};
const char* FullSourceKernel();const char* QuantumCommitKernel();const char* FullM1ShaderSha();
}}
