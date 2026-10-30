#include "full_transport.h"
#include "../angles_generated.h"
#include <cstring>
#include <cstdio>
namespace grs {namespace m1 {
SourceTransportStats FullM1Transport::Stats()const{auto r=source.Stats();r.allocatedBytes=AllocatedBytes();r.dispatches+=stats.dispatches;r.barriers+=stats.barriers;r.fences+=stats.fences;r.polls+=stats.polls;r.mappedBytes+=stats.mappedBytes;r.uploadedBytes+=stats.uploadedBytes;r.coldMappedBytes+=stats.coldMappedBytes;if(!r.firstGlError)r.firstGlError=stats.firstGlError;return r;}
TransportCode FullM1Transport::Create(uint32_t K,uint32_t Kw,const Identity& id,const QuantumParameters& cold,uint32_t emitQ){
 if(active||source.ready||quantum.context!=EGL_NO_CONTEXT||submitPhase!=SubmitPhase::IDLE)return TransportCode::BUSY;if(emitQ>1073741824u||cold.gravity<120u||cold.gravity>1073741824u||cold.settle==0u||cold.settle>1073741824u||cold.kAir>960u||cold.kFlight>960u||cold.kWet==0u||cold.kWet>960u||cold.kappa>960u||cold.soft>=605394u)return TransportCode::PROTOCOL_REJECTED;coldParams=cold;lost=failed=frozen=false;lastFrame={};lastOrdinal=0;frameHasSource=false;lastPublication=2;arenaFrame[0]=arenaFrame[1]={};used[0]=used[1]=0;stats={};extraBytes=0;tail={};batchesIssued=lastBatchPasses=submitCancelled=0;lastBatchWallNs=submitFirstNs=submitTotalWallNs=0;
 source.submitObserver=quantum.submitObserver=submitObserver;auto c=source.CreateUsing(K,Kw,id,FullM1ShaderSha(),FullSourceKernel());if(c!=TransportCode::OK)return c;source.emitQ=emitQ;source.owner.fullWaterContract=true;c=quantum.Create(K,Kw);if(c!=TransportCode::OK){Release();return c;}
 glDeleteProgram(quantum.program);quantum.program=0;GLuint sh=glCreateShader(GL_COMPUTE_SHADER);const char* text=QuantumCommitKernel();glShaderSource(sh,1,&text,nullptr);submitObserver.Emit(SubmitStage::ShaderEnter);glCompileShader(sh);GLint good;glGetShaderiv(sh,GL_COMPILE_STATUS,&good);submitObserver.Emit(SubmitStage::ShaderExit,0,uint32_t(good));if(!good){glDeleteShader(sh);Release();return TransportCode::SHADER_FAILED;}quantum.program=glCreateProgram();glAttachShader(quantum.program,sh);submitObserver.Emit(SubmitStage::ProgramEnter);glLinkProgram(quantum.program);glDeleteShader(sh);glGetProgramiv(quantum.program,GL_LINK_STATUS,&good);submitObserver.Emit(SubmitStage::ProgramExit,0,uint32_t(good));if(!good){Release();return TransportCode::SHADER_FAILED;}
 SubmitScope resources(submitObserver,SubmitStage::ResourceEnter,SubmitStage::ResourceExit);spareAt=quantum.workWords;replayFrozen=spareAt+quantum.words;uint32_t expanded=replayFrozen+256;glBindBuffer(GL_SHADER_STORAGE_BUFFER,quantum.workspace);glBufferData(GL_SHADER_STORAGE_BUFFER,expanded*4,nullptr,GL_DYNAMIC_COPY);glBufferSubData(GL_SHADER_STORAGE_BUFFER,quantum.angles*4,sizeof(kAngleWords),kAngleWords);quantum.allocated+=uint64_t(expanded-quantum.workWords)*4;quantum.workWords=expanded;quantum.deferFence=true;
 eventArenas[0]=quantum.events;glGenBuffers(1,&eventArenas[1]);glBindBuffer(GL_SHADER_STORAGE_BUFFER,eventArenas[1]);glBufferData(GL_SHADER_STORAGE_BUFFER,GLsizeiptr(quantum.eventCapacity)*256,nullptr,GL_DYNAMIC_COPY);extraBytes+=uint64_t(quantum.eventCapacity)*256;glGenBuffers(2,results);for(GLuint b:results){glBindBuffer(GL_SHADER_STORAGE_BUFFER,b);glBufferData(GL_SHADER_STORAGE_BUFFER,1024,nullptr,GL_DYNAMIC_COPY);extraBytes+=1024;}
 if(glGetError()!=GL_NO_ERROR){Release();return TransportCode::MEMORY_PROFILE_UNSUPPORTED;}if(!quantum.uniformLocations.Prepare(quantum.program)){Release();return TransportCode::SHADER_FAILED;}resources.End(0);if(PrepareProgram(quantum.program,source.banks[0],quantum.candidate,quantum.workspace,eventArenas[0],submitObserver)!=GL_NO_ERROR){Release();return TransportCode::GL_FAILED;}return TransportCode::OK;
}
TransportCode FullM1Transport::SubmitSource(const std::array<uint32_t,32>& command,RootRequest r){
 SubmitScope observed(submitObserver,SubmitStage::SubmitEnter,SubmitStage::SubmitExit,((uint64_t(r.nonce.hi)<<32)|r.nonce.lo));
 auto result=[&]()->TransportCode {
 if(lost)return TransportCode::STATE_LOST;if(active||frozen||submitPhase!=SubmitPhase::IDLE)return TransportCode::BUSY;if(failed)return TransportCode::PROTOCOL_REJECTED;
 if(r.kind!=1||Compare64(r.frame,lastFrame)<=0||r.publicationSlot>1||(source.Owner().AcceptedCount()&&r.publicationSlot==lastPublication))return TransportCode::PROTOCOL_REJECTED;
 auto c=source.Submit(command,r);if(c!=TransportCode::OK)return c;source.owner.pubs[r.publicationSlot].available=false;pending=source.pending;active=true;quantumFlight=false;lastFrame=r.frame;lastOrdinal=0;frameHasSource=true;return c;

 }();observed.End(uint32_t(result));return result;
}
// T324 §2.2 first batch. All original validation + Retire + PinCandidate +
// BeginQuantum happen here exactly once (nonce is consumed by the Run layer
// only after OK_SUBMITTING, so a BUSY before this point retries the same
// nonce without a duplicate Begin — the contract's core fix). Returns
// OK_SUBMITTING while the root is unsealed.
TransportCode FullM1Transport::SubmitQuantum(const QuantumParameters& p,RootRequest r){
 SubmitScope observed(submitObserver,SubmitStage::SubmitEnter,SubmitStage::SubmitExit,((uint64_t(r.nonce.hi)<<32)|r.nonce.lo));
 auto result=[&]()->TransportCode {
 if(lost)return TransportCode::STATE_LOST;if(!quantum.Current())return TransportCode::WRONG_OWNER;if(active||frozen||submitPhase!=SubmitPhase::IDLE)return TransportCode::BUSY;if(failed)return TransportCode::PROTOCOL_REJECTED;
 int order=Compare64(r.frame,lastFrame);if(r.kind!=2||order<0||r.quantumOrdinal!=(order==0?lastOrdinal+1:1)||r.quantumOrdinal>4||r.publicationSlot>1||r.eventArena>1||r.eventArena!=(r.frame.lo&1u)||(source.Owner().AcceptedCount()&&r.publicationSlot==lastPublication))return TransportCode::PROTOCOL_REJECTED;
 if(p.gravity!=coldParams.gravity||p.settle!=coldParams.settle||p.kAir!=coldParams.kAir||p.kFlight!=coldParams.kFlight||p.kWet!=coldParams.kWet||p.kappa!=coldParams.kappa||p.soft!=coldParams.soft||p.flux!=coldParams.flux)return TransportCode::PROTOCOL_REJECTED;
 if(p.gravity<120||p.gravity>1073741824||p.settle==0||p.settle>1073741824||p.kAir>960||p.kFlight>960||p.kWet==0||p.kWet>960||p.kappa>960||p.soft>=605394||p.requestedAngle< -7680||p.requestedAngle>7680)return TransportCode::PROTOCOL_REJECTED;
 uint32_t a=r.eventArena,prefix=0;GLuint prefixBuffer=0;if(order==0&&frameHasSource&&lastOrdinal==0){prefix=source.tail[53];prefixBuffer=source.events[source.pending.eventArena];}else if(Compare64(arenaFrame[a],r.frame)==0){prefix=used[a];prefixBuffer=eventArenas[a];}
 r.eventStart=prefix;r.eventCapacity=quantum.EventCapacity();r.dispatchCount=quantum.ExpectedDispatches()+8;r.barrierCount=r.dispatchCount;r.uploadedBytes=0;
 source.Retire();if(source.fatal)return TransportCode::GL_FAILED;auto pin=source.PinCandidate();if(pin!=TransportCode::OK)return pin;auto begin=source.owner.BeginQuantum(r,p.requestedAngle);if(begin!=ControlCode::OK){source.candidatePinned=false;return begin==ControlCode::BACKPRESSURE?TransportCode::BUSY:TransportCode::PROTOCOL_REJECTED;}
 quantum.events=eventArenas[a];quantum.traceIdentity=source.identity;auto c=quantum.Enqueue(source.banks[source.owner.Bank()],source.work,96+4096*64+source.words,prefixBuffer,prefix,p,r);if(c!=TransportCode::OK){source.owner.Stop();failed=true;source.candidatePinned=false;return c;}
 // The sealed-state transfer fields must be set BEFORE the first batch: the
 // last batch (fence/stats/pending transfer) and CancelSubmit read pending,
 // and Poll gates on active+FENCE_PENDING. This mirrors the original
 // monolithic submit's final block, applied when the root actually seals.
 pending=r;lastFrame=r.frame;lastOrdinal=r.quantumOrdinal;frameHasSource=false;arenaFrame[a]=r.frame;
 // Stage the tail uniforms and binding targets exactly as the original
 // monolithic submit did right after Enqueue (same Set order); they are
 // applied when the batch cursor crosses the stage-20 boundary.
 tailSpare=spareAt;tailReplay=replayFrozen;tailBank=source.owner.Bank();tailSlot=r.publicationSlot;tailArena=r.eventArena;tailCarry=r.carry;tailDispatches=r.dispatchCount;
 quantum.bindTargets[0]=source.banks[1-tailBank];quantum.bindTargets[1]=source.pubs[tailSlot];quantum.bindTargets[2]=results[tailSlot];quantum.pubWords=source.pubWords;
 active=true;quantumFlight=true;
 submitPhase=SubmitPhase::BATCH_ACTIVE;passCursor=0;batchesIssued=0;submitFirstNs=uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());submitTotalWallNs=0;
 submitObserver.Emit(SubmitStage::SubmitRootBegin,((uint64_t(r.nonce.hi)<<32)|r.nonce.lo));
 auto batch=ExecuteBatch();
 if(batch!=TransportCode::OK_SUBMITTING){FailSubmit();return batch;}
 return TransportCode::OK_SUBMITTING;

 }();
 return result;
}
// T324 §2.2: dispatch one batch from the cursor. Invoked by SubmitQuantum
// (first batch) and ContinueSubmit (subsequent). Op counting is TOP-LEVEL:
// each table entry is one op, Sort entries each count once. Batch ends at
// kBatchMaxOps or budget exhaustion or table end. Fence+flush only on the
// final batch, at the original position (after Pass(27) + FinalizeRoot).
TransportCode FullM1Transport::ExecuteBatch(){
 if(submitPhase!=SubmitPhase::BATCH_ACTIVE)return TransportCode::NOT_READY;
 if(!quantum.Current())return TransportCode::WRONG_OWNER;
 const uint32_t total=quantum.PlannedPasses();
 if(passCursor>=total)return TransportCode::NOT_READY;
 auto start=std::chrono::steady_clock::now();
 uint32_t ops=0,from=passCursor;bool tailApplied=false;
 while(passCursor<total&&ops<kBatchMaxOps){
  // INV-4/§5: defensive re-bind at each batch start (first op only).
  if(passCursor==from&&ops==0){glUseProgram(quantum.program);}
  // Apply staged tail uniforms when crossing the stage-20 boundary — the
  // original monolithic code ran them right after Enqueue returned, i.e.
  // between pass 20 and pass 21. Same Set order (contract INV-4).
  if(!tailApplied&&passCursor>0&&quantum.passTable[passCursor-1].stage==20){
   quantum.Set("uSpare",tailSpare);quantum.Set("uReplayFrozen",tailReplay);quantum.Set("uBaseBank",tailBank);quantum.Set("uPublicationSlot",tailSlot);quantum.Set("uEventArena",tailArena);quantum.Set("uCarry",tailCarry);quantum.Set("uActualDispatches",tailDispatches);tailApplied=true;
  }
  const auto& op=quantum.passTable[passCursor];
  quantum.RunPassRange(passCursor,passCursor+1);passCursor++;ops++;
  // TestHook points stay at original stages 20/25/27 boundaries.
  if(passCursor<total&&quantum.passTable[passCursor-1].stage==20)TestHook(20);
  if(quantum.passTable[passCursor-1].stage==25)TestHook(25);
  if(quantum.passTable[passCursor-1].stage==27)TestHook(27);
  if(uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now()-start).count())>=kBatchBudgetNs&&passCursor<total)break;
 }
 lastBatchPasses=ops;batchesIssued++;
 lastBatchWallNs=uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now()-start).count());
 if(passCursor>=total){
  // Last batch: the original tail — owner pending upload, FinalizeRoot,
  // fence + flush, stats, GL error latch. uUploaded was set inside
  // RunPassRange at stage 27 (original position, original value
  // submittedUniformBytes+8 measured at that moment).
  const auto& r=pending;pending.uploadedBytes=quantum.uploadedStage33;
  source.owner.pending.uploadedBytes=quantum.uploadedStage33;quantum.trace.FinalizeRoot(r);
  submitObserver.Emit(SubmitStage::FenceEnter,((uint64_t(r.nonce.hi)<<32)|r.nonce.lo));quantum.fence=glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE,0);submitObserver.Emit(SubmitStage::FenceExit,((uint64_t(r.nonce.hi)<<32)|r.nonce.lo),quantum.fence?0:1);glFlush();
  stats.dispatches+=quantum.dispatches;stats.barriers+=quantum.dispatches;stats.fences++;stats.uploadedBytes+=quantum.uploadedStage33;
  auto gl=glGetError();if(!gl)gl=quantum.firstGlError;if(gl&&!stats.firstGlError)stats.firstGlError=gl;
  submitPhase=SubmitPhase::FENCE_PENDING;submitTotalWallNs=uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count())-submitFirstNs;
  if(!quantum.fence||gl){FailSubmit();return TransportCode::GL_FAILED;}
  return TransportCode::OK_SUBMITTING;
 }
 return TransportCode::OK_SUBMITTING;
}
// T324 §2.3: same-nonce continuation called by Run while submitting.
TransportCode FullM1Transport::ContinueSubmit(){
 // T324 §2.2: FENCE_PENDING means the root is sealed (all batches + fence);
 // OK hands it to the Run layer as inflight so the existing Poll path owns
 // acceptance. OK_SUBMITTING here would keep Run in submitting forever.
 if(submitPhase==SubmitPhase::FENCE_PENDING)return TransportCode::OK;
 if(submitPhase!=SubmitPhase::BATCH_ACTIVE)return TransportCode::NOT_READY;
 SubmitScope observed(submitObserver,SubmitStage::SubmitEnter,SubmitStage::SubmitExit,((uint64_t(pending.nonce.hi)<<32)|pending.nonce.lo));
 auto result=ExecuteBatch();
 if(result!=TransportCode::OK_SUBMITTING&&result!=TransportCode::OK)FailSubmit();
 observed.End(uint32_t(result));return result;
}
// T324 §2.4: revoke an unsealed root. Dispatched GPU work cannot be rolled
// back (GL semantics); the candidate/owner side resets so the root can never
// reach inflight/accepted. FENCE_PENDING additionally deletes the fence.
TransportCode FullM1Transport::CancelSubmit(){
 if(submitPhase==SubmitPhase::IDLE)return TransportCode::NOT_READY;
 if(!quantum.Current())return TransportCode::WRONG_OWNER;
 submitObserver.Emit(SubmitStage::SubmitCancel,((uint64_t(pending.nonce.hi)<<32)|pending.nonce.lo),uint32_t(submitPhase));
 if(submitPhase==SubmitPhase::FENCE_PENDING&&quantum.fence){glDeleteSync(quantum.fence);quantum.fence=nullptr;}
 source.owner.Stop();source.candidatePinned=false;failed=true;submitCancelled++;
 submitTotalWallNs=uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count())-submitFirstNs;
 submitPhase=SubmitPhase::IDLE;passCursor=0;active=false;quantumFlight=false;
 return TransportCode::PROTOCOL_REJECTED;
}
void FullM1Transport::FailSubmit(){if(submitPhase==SubmitPhase::FENCE_PENDING&&quantum.fence){glDeleteSync(quantum.fence);quantum.fence=nullptr;}source.owner.Stop();source.candidatePinned=false;failed=true;submitPhase=SubmitPhase::IDLE;passCursor=0;active=false;quantumFlight=false;}
TransportCode FullM1Transport::Poll(AcceptedRoot& out){
 SubmitScope observed(submitObserver,SubmitStage::PollEnter,SubmitStage::PollExit,((uint64_t(pending.nonce.hi)<<32)|pending.nonce.lo));
 auto result=[&]()->TransportCode {
 out={};if(lost)return TransportCode::STATE_LOST;if(!active)return TransportCode::NOT_READY;
 if(!quantumFlight){auto c=source.Poll(out);if(c==TransportCode::BUSY)return c;active=false;tail=source.LastCompletion();if(c==TransportCode::OK)lastPublication=out.publication;else failed=true;return c;}
 if(submitPhase!=SubmitPhase::FENCE_PENDING)return TransportCode::NOT_READY;
 if(!quantum.Current())return TransportCode::WRONG_OWNER;stats.polls++;auto f=glClientWaitSync(quantum.fence,0,0);if(f==GL_TIMEOUT_EXPIRED)return TransportCode::BUSY;if(f==GL_WAIT_FAILED){source.owner.Poll(FenceState::Failed,nullptr,0,GL_INVALID_OPERATION,out);failed=true;return TransportCode::GL_FAILED;}
 if(source.owner.State()==FlightState::Quarantined){source.owner.Poll(FenceState::Signaled,nullptr,0,0,out);glDeleteSync(quantum.fence);quantum.fence=nullptr;source.candidatePinned=false;active=false;failed=true;submitPhase=SubmitPhase::IDLE;return TransportCode::PROTOCOL_REJECTED;}
 glBindBuffer(GL_COPY_READ_BUFFER,results[pending.publicationSlot]);auto p=glMapBufferRange(GL_COPY_READ_BUFFER,0,1024,GL_MAP_READ_BIT);if(!p){failed=true;source.owner.Stop();return TransportCode::GL_FAILED;}std::memcpy(tail.data(),p,1024);stats.mappedBytes+=1024;bool good=glUnmapBuffer(GL_COPY_READ_BUFFER)==GL_TRUE;auto gl=glGetError();if(gl&&!stats.firstGlError)stats.firstGlError=gl;
 auto code=source.owner.Poll(FenceState::Signaled,tail.data(),256,good?(gl?gl:quantum.firstGlError):GL_INVALID_OPERATION,out);glDeleteSync(quantum.fence);quantum.fence=nullptr;source.candidatePinned=false;active=false;used[pending.eventArena]=tail[53];submitPhase=SubmitPhase::IDLE;if(code!=ControlCode::OK){failed=true;return TransportCode::PROTOCOL_REJECTED;}source.tail=tail;source.pending=pending;source.lastFrame=pending.frame;lastPublication=out.publication;return TransportCode::OK;

 }();observed.End(uint32_t(result));return result;
}
ControlCode FullM1Transport::Stop(){failed=true;return source.Stop();}
void FullM1Transport::ContextLost(){source.ContextLost();quantum.ContextLost();eventArenas[0]=eventArenas[1]=results[0]=results[1]=0;active=false;lost=true;submitPhase=SubmitPhase::IDLE;}
TransportCode FullM1Transport::Release(){if(active||quantum.fence||submitPhase!=SubmitPhase::IDLE)return TransportCode::BUSY;if(source.context!=EGL_NO_CONTEXT&&!source.Current())return TransportCode::WRONG_OWNER;source.Retire();if(source.Owner().HasLeases())return TransportCode::BUSY;auto c=source.Release();if(c!=TransportCode::OK)return c;if(eventArenas[0])quantum.events=0;c=quantum.Release();if(c!=TransportCode::OK)return c;glDeleteBuffers(2,eventArenas);glDeleteBuffers(2,results);eventArenas[0]=eventArenas[1]=results[0]=results[1]=0;return TransportCode::OK;}
ControlCode FullM1Transport::Acquire(uint32_t slot,U64 generation,PublicationLease& lease,GLuint& buffer){return source.Acquire(slot,generation,lease,buffer);}
ControlCode FullM1Transport::ReleasePublication(uint32_t slot,U64 generation,GLsync f){return source.ReleasePublication(slot,generation,f);}
TransportCode FullM1Transport::Freeze(bool value){if(active||submitPhase!=SubmitPhase::IDLE)return TransportCode::BUSY;auto c=source.Freeze(value);if(c!=TransportCode::OK)return c;c=quantum.Freeze(value);if(c==TransportCode::OK)frozen=value;return c;}
TransportCode FullM1Transport::Capture(std::vector<uint32_t>& bank,std::vector<uint32_t>& log){if(!frozen||active||submitPhase!=SubmitPhase::IDLE)return TransportCode::BUSY;auto c=source.CaptureAccepted(bank);if(c!=TransportCode::OK)return c;uint32_t a=pending.eventArena,n=quantumFlight?used[a]:tail[53];glBindBuffer(GL_COPY_READ_BUFFER,quantumFlight?eventArenas[a]:source.events[a]);if(n==0){log.clear();return TransportCode::OK;}auto p=glMapBufferRange(GL_COPY_READ_BUFFER,0,GLsizeiptr(n)*256,GL_MAP_READ_BIT);if(!p)return TransportCode::GL_FAILED;log.resize(size_t(n)*64);std::memcpy(log.data(),p,log.size()*4);stats.coldMappedBytes+=log.size()*4;return glUnmapBuffer(GL_COPY_READ_BUFFER)==GL_TRUE?TransportCode::OK:TransportCode::GL_FAILED;}
}}
