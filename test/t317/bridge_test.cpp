#define GRS_BRIDGE_HOST_TEST
#include "../../entry/src/main/cpp/gpu_resident/app_bridge.cpp"
#include <cassert>
#include <iostream>
extern "C" {
EGLBoolean EGLAPIENTRY eglMakeCurrent(EGLDisplay,EGLSurface,EGLSurface,EGLContext){return EGL_TRUE;}
EGLBoolean EGLAPIENTRY eglDestroySurface(EGLDisplay,EGLSurface){return EGL_TRUE;}
EGLBoolean EGLAPIENTRY eglDestroyContext(EGLDisplay,EGLContext){return EGL_TRUE;}
EGLBoolean EGLAPIENTRY eglTerminate(EGLDisplay){return EGL_TRUE;}
EGLBoolean EGLAPIENTRY eglQuerySurface(EGLDisplay,EGLSurface,EGLint,EGLint* p){*p=64;return EGL_TRUE;}
EGLBoolean EGLAPIENTRY eglSwapBuffers(EGLDisplay,EGLSurface){return EGL_TRUE;}
EGLint EGLAPIENTRY eglGetError(){return EGL_SUCCESS;}
}
static uint64_t clockNs=1000000;static uint32_t waits=0,pumps=0;static int mode=0;static App* currentApp=nullptr;static uint32_t submitCount=0,pollCount=0;static bool flight=false;
namespace grs {
FoundationInfo GetFoundationInfo(){return {"HOST","HOST","HOST","0000000000000000000000000000000000000000000000000000000000000000","0000000000000000000000000000000000000000000000000000000000000000","unused",0,false,false};}
const char* SourceKernelSha(){return "0000000000000000000000000000000000000000000000000000000000000000";}
TransportCode SourceGpuTransport::Create(uint32_t,const Identity&){
 submitObserver.Emit(SubmitStage::PrepareEnter);if(mode==5)clockNs+=187000000;
 if(mode==6){std::lock_guard<std::mutex> lock(currentApp->mutex);currentApp->InvalidateLocked();}
 submitObserver.Emit(SubmitStage::PrepareExit);return TransportCode::OK;}
TransportCode SourceGpuTransport::Submit(const std::array<uint32_t,32>&,RootRequest r){submitCount++;submitObserver.Emit(SubmitStage::SubmitEnter,r.nonce.lo);
 if(mode==7)clockNs+=78000000;pending=r;flight=true;
 submitObserver.Emit(SubmitStage::SubmitExit,r.nonce.lo,0);return TransportCode::OK;}
TransportCode SourceGpuTransport::SubmitQuantum(RootRequest r,int){submitCount++;submitObserver.Emit(SubmitStage::SubmitEnter,r.nonce.lo);
 if(mode==7)clockNs+=78000000;pending=r;flight=true;
 submitObserver.Emit(SubmitStage::SubmitExit,r.nonce.lo,0);return TransportCode::OK;}
TransportCode SourceGpuTransport::Poll(AcceptedRoot& out){pollCount++;submitObserver.Emit(SubmitStage::PollEnter,pending.nonce.lo);if(!flight)return TransportCode::NOT_READY;if(mode==8)clockNs+=40000000;if(mode==9&&pollCount==1)return TransportCode::BUSY;submitObserver.Emit(SubmitStage::PollExit,pending.nonce.lo);flight=false;out.nonce=pending.nonce;return TransportCode::OK;}
ControlCode SourceGpuTransport::Stop(){input.lo++;return flight?ControlCode::QUARANTINED:ControlCode::OK;}
void SourceGpuTransport::ContextLost(){}
void ResidentScene::Reset(bool){}
ResidentSceneReceipt ResidentScene::Draw(SourceGpuTransport&,uint32_t,U64,const ResidentSceneFrame&,Swap,void*){return {};}
}
void hook(App& a,grs::BridgeStage s){using grs::BridgeStage;if(s==BridgeStage::Wake){waits++;clockNs+=(mode==1&&waits==2)?40000000:8000000;if(waits>5)a.quit=true;}if(s==BridgeStage::PumpBegin){pumps++;if(pumps==1){if(mode==2)clockNs+=40000000;if(mode==3){a.Mark(BridgeStage::DrawBegin,a.epoch);clockNs+=40000000;a.Mark(BridgeStage::DrawEnd,a.epoch);}if(mode==4){std::lock_guard<std::mutex> l(a.mutex);a.queue[0].epoch=0;a.count=1;}}}}

int main(){int passed=0;
 for(mode=0;mode<10;mode++){
  waits=pumps=submitCount=pollCount=0;clockNs=1000000;flight=false;
  App a(false);currentApp=&a;a.window=reinterpret_cast<void*>(1);
  a.hostNow=[](){return clockNs;};a.hostHook=hook;a.loop();
  bool gap=false,anchor=false,prepare=false,rejected=false,ready=false;
  uint64_t preparationEnd=0,firstAnchor=0;
  for(uint32_t i=0;i<a.timeline.count;i++){const auto& row=a.timeline.rows[i];
   if(row.stage==uint32_t(grs::BridgeStage::PrepareExit)){prepare=true;preparationEnd=row.ns;}
   if(row.stage==uint32_t(grs::BridgeStage::Ready))ready=true;
   if(row.stage==uint32_t(grs::BridgeStage::ReadyRejected))rejected=true;
   if(row.stage==uint32_t(grs::BridgeStage::AnchorSet)){anchor=true;if(!firstAnchor)firstAnchor=row.ns;assert(row.last<=row.ns);}
   if(row.stage==uint32_t(grs::BridgeStage::PauseBegin)){gap=true;assert(row.delta>33333333&&row.last>0);}
   if(row.stage>=24){assert(row.epoch==row.surface);assert(row.tid!=0);}
  }
  assert(prepare);
  if(mode==6){assert(rejected&&!ready&&!anchor&&submitCount==0);passed++;continue;}
  assert(anchor&&preparationEnd<=firstAnchor);
  if(mode==0||mode==5||mode==9)assert(!gap);
  if(mode==1||mode==2||mode==3||mode==7||mode==8)assert(gap&&!a.ready&&a.readyEpoch==0);
  if(mode==7){assert(submitCount==1);assert(a.timeline.Export(0).find("T317_STAGE39_V2")!=std::string::npos);}
  if(mode==4)assert(a.staleRejected>0);
  passed++;std::cout<<"mode="<<mode<<" gap="<<gap<<" submits="<<submitCount<<" polls="<<pollCount<<"\n";
 }
 {mode=0;flight=false;grs::SourceGpuTransport transport;grs::ResidentRun run(transport);
  assert(run.Advance(1,{1,0},{1,0},0,192)==grs::ControlCode::OK);
  assert(run.Advance(16666667,{2,0},{2,0},0,192)==grs::ControlCode::OK);
  assert(run.Pump()==grs::TransportCode::OK&&flight);
  assert(run.Pause()==grs::ControlCode::QUARANTINED);
  assert(run.Tick().accepted==0&&run.Tick().fault);
  run.Pump();assert(!flight&&run.Tick().accepted==0);passed++;
 }
 std::cout<<"{\"revision\":\"T317_APP_LOOP_DELAY_DOUBLE_V1\",\"passed\":"<<passed<<",\"failed\":0,\"device\":\"NOT_RUN\"}\n";
}

