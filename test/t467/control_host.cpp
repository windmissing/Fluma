#include <EGL/egl.h>
#include <GLES3/gl31.h>
#include <cassert>
#include <iostream>
#define GRS_APP_M1
#define GRS_BRIDGE_HOST_TEST
#include "app_bridge.cpp"
extern "C" void GL_APIENTRY glFinish(){auto f=reinterpret_cast<PFNGLFINISHPROC>(eglGetProcAddress("glFinish"));assert(f);f();}
extern "C" void GL_APIENTRY glCopyBufferSubData(GLenum a,GLenum b,GLintptr c,GLintptr d,GLsizeiptr n){auto f=reinterpret_cast<PFNGLCOPYBUFFERSUBDATAPROC>(eglGetProcAddress("glCopyBufferSubData"));assert(f);f(a,b,c,d,n);}
extern "C" void GL_APIENTRY glUniform1f(GLint a,GLfloat v){auto f=reinterpret_cast<PFNGLUNIFORM1FPROC>(eglGetProcAddress("glUniform1f"));assert(f);f(a,v);}
extern "C" void GL_APIENTRY glUniform4fv(GLint a,GLsizei n,const GLfloat* v){auto f=reinterpret_cast<PFNGLUNIFORM4FVPROC>(eglGetProcAddress("glUniform4fv"));assert(f);f(a,n,v);}
extern "C" void GL_APIENTRY glGetShaderInfoLog(GLuint a,GLsizei n,GLsizei* len,GLchar* out){auto f=reinterpret_cast<PFNGLGETSHADERINFOLOGPROC>(eglGetProcAddress("glGetShaderInfoLog"));assert(f);f(a,n,len,out);}
extern "C" void GL_APIENTRY glGetProgramInfoLog(GLuint a,GLsizei n,GLsizei* len,GLchar* out){auto f=reinterpret_cast<PFNGLGETPROGRAMINFOLOGPROC>(eglGetProcAddress("glGetProgramInfoLog"));assert(f);f(a,n,len,out);}
static uint64_t nowNs=1000000000;
static Identity MakeId(uint32_t run){Identity id{};id.run={run,0};id.owner=id.context=id.surface=id.geometry=id.config={1,0};return id;}
static void Request(App& a,bool v2){a.nextProgressV2=v2;a.invalidate(nullptr,false);a.progressV2=v2;}
static void Resources(App& a,uint32_t run){
 assert(!a.AllowedLocked());assert(a.recovery.Retiring(a.epoch));assert(a.recovery.Retired(a.epoch));
 a.progress.ResetRun();a.progress.open=false;a.progress.hasPulse=false;
 a.transportIdentity=MakeId(run);a.identity=a.transportIdentity;a.input={1,0};
 a.v2ResourcesReady=a.recovery.Resources(a.epoch,a.window,a.transportIdentity);
}
static uint64_t Handoff(App& a){auto token=a.RecoverySessionLocked(a.recovery.serial,a.epoch,a.transportIdentity.run,1);assert(token);return token;}
int main(){
 auto a=std::make_unique<App>(false);a->hostNow=[](){return nowNs;};a->window=reinterpret_cast<void*>(1);a->ui.SetMode(false);
 unsigned cases=0;
 Request(*a,true);const auto serial=a->recovery.serial,epoch=a->epoch.load();
 assert(!a->progress.open&&!a->AllowedLocked());assert(a->BeginPageSessionLocked()==0);++cases;
 nowNs+=39000000000ull;Resources(*a,1);a->TryReadyV2Locked();assert(!a->AllowedLocked()&&a->progress.pending==0);++cases;
 assert(a->RecoverySessionLocked(serial,epoch,{999,0},1)==0);assert(a->RecoverySessionLocked(serial-1,epoch,{1,0},1)==0);++cases;
 auto token=Handoff(*a);assert(!a->AllowedLocked());assert(std::string(a->PagePulseLocked(token,1,nowNs))=="PAGE_PULSE_ACCEPTED");assert(a->AllowedLocked()&&a->readyEpoch==epoch&&a->progress.generated==0);++cases;
 nowNs+=8400000;a->PagePulseLocked(token,2,nowNs);nowNs+=8400000;a->PagePulseLocked(token,3,nowNs);assert(a->progress.generated==1&&a->progress.Balanced());++cases;
 // Owner delay never substitutes the V1 anchor for the actual V2 pulse domain.
 a->controlAnchor=nowNs-50000000;for(unsigned i=0;i<7;++i){nowNs+=8000000;a->PagePulseLocked(token,4+i,nowNs);a->ObserveControlLocked(nowNs);}assert(a->AllowedLocked());++cases;
 nowNs+=33333334;a->ObserveControlLocked(nowNs);assert(!a->AllowedLocked()&&!a->progress.open&&a->recovery.phase==recovery467::Phase::FAULTED_REVOKED);++cases;
 for(unsigned i=0;i<3;++i){++nowNs;assert(std::string(a->PagePulseLocked(token,20+i,nowNs))=="STALE_PAGE_SESSION");}assert(a->BeginPageSessionLocked()==0&&!a->AllowedLocked());++cases;
 Request(*a,true);auto second=a->recovery.serial;assert(second>serial);assert(a->progress.Balanced());Resources(*a,2);auto token2=Handoff(*a);assert(token2>token);++cases;
 assert(std::string(a->PagePulseLocked(token,99,nowNs))=="STALE_PAGE_SESSION");assert(!a->AllowedLocked());++cases;
 a->PagePulseLocked(token2,1,nowNs);assert(a->AllowedLocked());nowNs+=33333333;assert(std::string(a->PagePulseLocked(token2,2,nowNs))=="PAGE_PULSE_ACCEPTED");++cases;
 auto oldEpoch=a->epoch.load();Request(*a,true);auto staleSerial=a->recovery.serial;Request(*a,true);
 assert(!a->recovery.Resources(oldEpoch,a->window,MakeId(3)));assert(!a->recovery.CanSession(staleSerial,oldEpoch,{2,0},a->window));++cases;
 Resources(*a,4);a->progress.session=UINT64_MAX;assert(a->RecoverySessionLocked(a->recovery.serial,a->epoch,{4,0},1)==0&&a->requiresReset);++cases;
 a->progress.session=100;Request(*a,false);Resources(*a,5);auto v1=Handoff(*a);a->PagePulseLocked(v1,1,++nowNs);assert(a->AllowedLocked()&&!a->progressV2);++cases;
 Request(*a,true);Resources(*a,6);auto v2=Handoff(*a);assert(v2>v1);a->PagePulseLocked(v2,1,++nowNs);assert(a->AllowedLocked()&&a->progressV2);++cases;
 a->setActive(false);assert(!a->AllowedLocked());assert(a->RecoverySessionLocked(a->recovery.serial,a->epoch,{6,0},2)==0);a->setActive(true);assert(!a->AllowedLocked());++cases;
 // All RootCut gates use the same immutable permit; renewal is not a continuation.
 Request(*a,true);Resources(*a,7);auto t=Handoff(*a);a->PagePulseLocked(t,1,++nowNs);
 a->permit={a->identity,a->input,a->epoch,a->permissionRevision,nowNs+250000000,0,a->window};
 a->permitSession=t;a->permitPlan=1;a->progress.active.id=1;a->permit.nonce=7;a->permitKind=2;a->permitStarted=nowNs;a->jobLive=true;
 RootRequest r{};r.kind=2;r.nonce={7,0};
 assert(a->PermitLocked(r,nowNs));const auto deadline=a->permit.deadline;
 m1::RootIsolation gate; a->FreezePermitLocked(a->epoch,a->window,gate,7,2);assert(a->permit.deadline==deadline);
 uint64_t seq=1;while(nowNs+8000000<deadline){nowNs+=8000000;a->PagePulseLocked(t,++seq,nowNs);}
 nowNs=deadline;a->PagePulseLocked(t,++seq,nowNs);assert(a->PermitLocked(r,nowNs));assert(!a->PermitLocked(r,nowNs+1));++cases;
 Request(*a,true);assert(!a->PermitLocked(r,nowNs));++cases;
 a->captureRequest.state=App::CaptureRequest::RESERVED;a->captureRequest.armByNs=UINT64_MAX;
 Request(*a,true);assert(a->captureRequest.state==App::CaptureRequest::CANCELLED&&std::string(a->captureRequest.reason)=="RUN_RESET");++cases;
 a->evidence=App::NewCaptureStorage();a->evidence->armed=true;
 Request(*a,true);assert(a->evidence->frozen&&!a->evidence->armed&&std::string(a->evidence->reason.data())=="RUN_RESET");++cases;
 auto originalReason=a->evidence->reason;Request(*a,true);assert(a->evidence->reason==originalReason);++cases;
 // Whole-context retirement is the explicit path for pending roots/leases.
 recovery467::Transaction tx;assert(tx.Request(1,(void*)1));assert(tx.Retiring(1));assert(!tx.Resources(1,(void*)1,MakeId(1)));assert(tx.phase==recovery467::Phase::RETIRING_OLD);++cases;
 for(unsigned i=0;i<600;++i)tx.Note({"PULSE"});assert(tx.dropped==88);tx.Fault("FIRST");tx.Fault("SECOND");assert(std::string(tx.firstReason)=="FIRST");++cases;
 std::cout<<"{\"schema\":\"T467_GENERATED_BRIDGE_CONTROLS_V1\",\"passed\":"<<cases<<",\"failed\":0,\"device\":\"NOT_RUN\"}"<<std::endl;
}
