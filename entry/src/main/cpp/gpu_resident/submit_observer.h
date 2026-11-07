#pragma once
#include <GLES3/gl31.h>
#include <cstdint>
#include <chrono>
namespace grs {
// T317: fixed callback to the App's existing bounded timeline. No GL calls,
// strings or allocation in Emit. Codes are original TransportCode / raw GL.
enum class SubmitStage:uint32_t {SubmitEnter=24,SubmitExit,ShaderEnter,ShaderExit,
 ProgramEnter,ProgramExit,ResourceEnter,ResourceExit,PrepareEnter,PrepareExit,
 DispatchEnter,DispatchExit,FenceEnter,FenceExit,PollEnter,PollExit,
 // T324 §2.6: SubmitRootBegin fires once at the first batch of a partial
 // submit; SubmitCancel fires when CancelSubmit revokes an unsealed root.
 SubmitRootBegin=40,SubmitCancel,SubmitWarmup};
enum class TailStage:uint32_t {BatchCall=43,Flush,Continuation,Inflight,RealPoll,Accepted,Gap,Presentation,
 StagingFlush,ColdFixture,ColdStagingFlush,ColdBatch,ColdFence,ColdCleanup,ColdBatchFlush,IsolationCall,IsolationRevoke,
 PagePulse,PlanGrant,QuotaBalance,JobIntent,ProgressTimeout,SourceBoundary};
struct TailSample {
 TailStage stage; uint64_t root=0,start=0,end=0,a=0,b=0,c=0;
 uint32_t code=0,ordinal=0;
};
struct TailObserver {
 void* context=nullptr;
 void (*write)(void*,const TailSample&)=nullptr;
 static uint64_t Now(){
  return uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(
   std::chrono::steady_clock::now().time_since_epoch()).count());
 }
 uint64_t Stamp() const{return write?Now():0;}
 void Emit(const TailSample& s)const{if(write)write(context,s);}
};
struct SubmitObserver {
 void* context=nullptr;
 void (*write)(void*,SubmitStage,uint64_t,uint32_t,uint32_t,uint64_t)=nullptr;
 void Emit(SubmitStage s,uint64_t root=0,uint32_t code=0,uint32_t ordinal=0,uint64_t wallNs=0)const{
  if(write)write(context,s,root,code,ordinal,wallNs);
 }
};
struct SubmitScope {
 SubmitObserver observer;SubmitStage exit;uint64_t root;bool ended=false;
 SubmitScope(SubmitObserver o,SubmitStage enter,SubmitStage leave,uint64_t r=0):observer(o),exit(leave),root(r){observer.Emit(enter,root);}
 void End(uint32_t code){observer.Emit(exit,root,code);ended=true;}
 ~SubmitScope(){if(!ended)observer.Emit(exit,root,UINT32_MAX);}
};
// Zero groups are legal GLES 3.1: no invocation, no state/ledger/event writes,
// nonce, publication or stats. Force deferred program/resource driver work
// before READY/first anchor, on the same GL owner; no runtime clock is reset.
inline GLenum PrepareProgram(GLuint program,GLuint a,GLuint b,GLuint w,GLuint out,SubmitObserver observer){
 SubmitScope scope(observer,SubmitStage::PrepareEnter,SubmitStage::PrepareExit);
 glUseProgram(program);glBindBufferBase(GL_SHADER_STORAGE_BUFFER,0,a);
 glBindBufferBase(GL_SHADER_STORAGE_BUFFER,1,b);glBindBufferBase(GL_SHADER_STORAGE_BUFFER,2,w);
 glBindBufferBase(GL_SHADER_STORAGE_BUFFER,3,out);glDispatchCompute(0,1,1);glFinish();
 auto error=glGetError();scope.End(error);return error;
}
}
