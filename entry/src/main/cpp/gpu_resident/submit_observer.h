#pragma once
#include <GLES3/gl31.h>
#include <cstdint>
namespace grs {
// T317: fixed callback to the App's existing bounded timeline. No GL calls,
// strings or allocation in Emit. Codes are original TransportCode / raw GL.
enum class SubmitStage:uint32_t {SubmitEnter=24,SubmitExit,ShaderEnter,ShaderExit,
 ProgramEnter,ProgramExit,ResourceEnter,ResourceExit,PrepareEnter,PrepareExit,
 DispatchEnter,DispatchExit,FenceEnter,FenceExit,PollEnter,PollExit,
 // T324 §2.6: SubmitRootBegin fires once at the first batch of a partial
 // submit; SubmitCancel fires when CancelSubmit revokes an unsealed root.
 SubmitRootBegin=40,SubmitCancel};
struct SubmitObserver {
 void* context=nullptr;
 void (*write)(void*,SubmitStage,uint64_t,uint32_t,uint32_t)=nullptr;
 void Emit(SubmitStage s,uint64_t root=0,uint32_t code=0,uint32_t ordinal=0)const{
  if(write)write(context,s,root,code,ordinal);
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
