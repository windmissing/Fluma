#include <cstdint>
#include <EGL/egl.h>
#include <GLES3/gl31.h>
#include "../../entry/src/main/cpp/gpu_resident/m3a/controller.h"
#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>
#ifdef _WIN32
#include <Windows.h>
#include <mmsystem.h>
#pragma comment(lib,"winmm.lib")
struct HostTimerResolution {HostTimerResolution(){timeBeginPeriod(1);}~HostTimerResolution(){timeEndPeriod(1);}};
#endif
#include "gl_api.inc"
static uint64_t Now(){return uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());}
int main(){
#ifdef _WIN32
 HostTimerResolution timerResolution;
#endif
 std::cout.setf(std::ios::unitbuf);
 using F=EGLDisplay(EGLAPIENTRYP)(EGLenum,void*,const EGLint*);PROC(F,eglGetPlatformDisplayEXT);
 const EGLint displayArgs[]={0x3203,0x3450,0x3209,0x3487,EGL_NONE};auto display=f(0x3202,nullptr,displayArgs);assert(eglInitialize(display,nullptr,nullptr));
 const EGLint attributes[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_NONE};EGLConfig config;EGLint count;assert(eglChooseConfig(display,attributes,&config,1,&count)&&count);
 const EGLint contextArgs[]={EGL_CONTEXT_CLIENT_VERSION,3,0x30FB,1,EGL_NONE},surfaceArgs[]={EGL_WIDTH,8,EGL_HEIGHT,8,EGL_NONE};auto context=eglCreateContext(display,config,EGL_NO_CONTEXT,contextArgs);auto surface=eglCreatePbufferSurface(display,config,surfaceArgs);assert(eglMakeCurrent(display,surface,surface,context));
 using namespace grs::m3a;ControllerV4 controller;ControlIdentityV4 identity{1,2,3,4,5,6,7,8,9};PhysicalConfigV4 physical;physical.omegaProvided=true;
 std::array<uint32_t,8> sha{};const auto configured=controller.Configure(kProfiles[0],identity,71,physical,sha,sha,sha);std::cout<<"native configure="<<uint32_t(configured)<<"\n";assert(configured==ControlResult::OK);
 std::atomic<bool> stop{false};std::thread pulse([&]{uint64_t sequence=0;while(!stop){const auto now=Now();auto code=controller.Pulse({identity,++sequence,now},now);if(code!=ControlResult::OK)std::cout<<"pulse="<<uint32_t(code)<<"\n";std::this_thread::sleep_for(std::chrono::milliseconds(4));}});
 while(controller.Quota(Now())[5]==3)std::this_thread::yield();
 SourcePacketV4 packet;auto& w=packet.words;w[0]=1129534023;w[1]=4;w[2]=1;w[4]=1;w[6]=1;w[8]=7;w[10]=2;w[12]=71;w[14]=w[15]=1;w[24]=1;w[28]=8;
 auto binary=[&](size_t at,double value){std::memcpy(w.data()+at,&value,8);};binary(16,64.5);binary(18,0);binary(20,0.0625);
 assert(controller.Offer(identity,{1,2,3,4,5,1,1},packet)==ControlResult::OK);
 const auto begin=controller.Advance(Now(),1,1);std::cout<<"native source begin="<<uint32_t(begin)<<"\n";assert(begin==ControlResult::OK);
 TransportResultV4 result=TransportResultV4::IN_PROGRESS;
 const auto start=Now();while(result==TransportResultV4::IN_PROGRESS){result=controller.Pump(Now());if(result==TransportResultV4::IN_PROGRESS)std::this_thread::yield();}
 std::cout<<"native source result="<<uint32_t(result)<<" elapsedNs="<<Now()-start<<"\n";
 {auto d=controller.DiagnosticOwnerThread();std::cout<<"native source fault="<<uint32_t(d.fault)<<" outcome="<<d.completion[28]<<" reason="<<d.completion[29]<<" completed="<<d.completion[97]<<" uploaded="<<d.completion[112]<<" planned="<<d.permit.uploadedBytes<<" observed="<<d.evidence.uploadedBytes<<"\n";}
 // This host uses a software backend only as an offline fixture. A deadline
 // rejection is retained verbatim and never presented as product acceptance.
 if(result!=TransportResultV4::OK){stop=true;pulse.join();controller.Stop();while(!controller.DrainRevoked())std::this_thread::yield();controller.Release(true);return 2;}
 auto view=controller.PublicationView();assert(view.valid&&view.generation==1);std::cout<<"native V4 SOURCE gate/current-bank/published lease metadata PASS\n";
 const auto request=controller.ColdBegin(0,Now());std::cout<<"native cold begin="<<request<<"\n";assert(request.find("REQUESTED")!=std::string::npos);
 controller.BuildCold(Now());const auto state=controller.ColdState(1,Now());std::cout<<"native cold state="<<state<<"\n";assert(state.find("TRACE_READY")!=std::string::npos);
 const auto page=controller.ColdPage(1,1,Now());assert(page.find("GRS_M3_BOUNDED_COLD_V1")!=std::string::npos&&page.find("GRS_M3_TRACE_V4_QUOTA3_CONTROL128")!=std::string::npos&&page.find("INCOMPLETE")==std::string::npos);
 controller.ColdEnd(1,Now());

 const auto quantumBegin=controller.Advance(Now(),2,2);std::cout<<"native quantum begin="<<uint32_t(quantumBegin)<<"\n";assert(quantumBegin==ControlResult::OK);
 result=TransportResultV4::IN_PROGRESS;const auto qstart=Now();
 while(result==TransportResultV4::IN_PROGRESS){result=controller.Pump(Now());if(result==TransportResultV4::IN_PROGRESS)std::this_thread::yield();}
 std::cout<<"native quantum result="<<uint32_t(result)<<" elapsedNs="<<Now()-qstart<<"\n";
 {auto d=controller.DiagnosticOwnerThread();std::cout<<"native quantum fault="<<uint32_t(d.fault)<<" outcome="<<d.completion[28]<<" reason="<<d.completion[29]<<" completed="<<d.completion[97]<<" uploaded="<<d.completion[112]<<" planned="<<d.permit.uploadedBytes<<" observed="<<d.evidence.uploadedBytes<<" barriers="<<d.evidence.barriers<<" shaderBarrier="<<d.completion[111]<<"\n";}
 if(result!=TransportResultV4::OK){
  auto diagnostic=controller.DiagnosticOwnerThread();stop=true;pulse.join();
  if((result==TransportResultV4::DEADLINE&&diagnostic.fault==ControlResult::DEADLINE)||(result==TransportResultV4::STALE&&diagnostic.fault==ControlResult::CLOCK_GAP)){
   auto retained=controller.SceneHeaderV4();auto quota=controller.Quota(Now());assert(retained[72]==1&&quota[32]==0);
   std::cout<<"native host QUANTUM deadline/GAP guard: rejected, SOURCE retained, no Q accepted PASS (not a performance PASS)\n";
   controller.Stop();while(!controller.DrainRevoked())std::this_thread::yield();controller.Release(true);return 0;
  }
  controller.Stop();while(!controller.DrainRevoked())std::this_thread::yield();controller.Release(true);return 3;
 }
 view=controller.PublicationView();assert(view.valid&&view.generation==2);stop=true;pulse.join();
 std::cout<<"native full QUANTUM acceptance and bounded GPU cold capture/page PASS\n";
 controller.Release(true);eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);eglDestroyContext(display,context);eglDestroySurface(display,surface);eglTerminate(display);
}
