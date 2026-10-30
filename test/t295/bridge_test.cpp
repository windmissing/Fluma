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
static uint64_t clockNs=1000000;static uint32_t waits=0,pumps=0;static int mode=0;static bool flight=false;
namespace grs {
FoundationInfo GetFoundationInfo(){return {"HOST","HOST","HOST","0000000000000000000000000000000000000000000000000000000000000000","0000000000000000000000000000000000000000000000000000000000000000","unused",0,false,false};}
const char* SourceKernelSha(){return "0000000000000000000000000000000000000000000000000000000000000000";}
TransportCode SourceGpuTransport::Create(uint32_t,const Identity&){return TransportCode::OK;}
TransportCode SourceGpuTransport::Submit(const std::array<uint32_t,32>&,RootRequest r){pending=r;flight=true;return TransportCode::OK;}
TransportCode SourceGpuTransport::SubmitQuantum(RootRequest r,int){pending=r;flight=true;return TransportCode::OK;}
TransportCode SourceGpuTransport::Poll(AcceptedRoot& out){if(!flight)return TransportCode::NOT_READY;flight=false;out.nonce=pending.nonce;return TransportCode::OK;}
ControlCode SourceGpuTransport::Stop(){input.lo++;return flight?ControlCode::QUARANTINED:ControlCode::OK;}
void SourceGpuTransport::ContextLost(){}
void ResidentScene::Reset(bool){}
ResidentSceneReceipt ResidentScene::Draw(SourceGpuTransport&,uint32_t,U64,const ResidentSceneFrame&,Swap,void*){return {};}
}
void hook(App& a,grs::BridgeStage s){using grs::BridgeStage;if(s==BridgeStage::Wake){waits++;clockNs+=(mode==1&&waits==2)?40000000:8000000;if(waits>5)a.quit=true;}if(s==BridgeStage::PumpBegin){pumps++;if(pumps==1){if(mode==2)clockNs+=40000000;if(mode==3){a.Mark(BridgeStage::DrawBegin,a.epoch);clockNs+=40000000;a.Mark(BridgeStage::DrawEnd,a.epoch);}if(mode==4){std::lock_guard<std::mutex> l(a.mutex);a.queue[0].epoch=0;a.count=1;}}}}
int main(){int passed=0;for(mode=0;mode<5;mode++){waits=pumps=0;clockNs=1000000;flight=false;App a(false);a.window=reinterpret_cast<void*>(1);a.hostNow=[](){return clockNs;};a.hostHook=hook;a.loop();bool gap=false,anchor=false,work=false;for(uint32_t i=0;i<a.timeline.count;i++){auto& row=a.timeline.rows[i];if(row.stage==uint32_t(grs::BridgeStage::AnchorSet)){anchor=true;assert(row.last<=row.ns);}if(row.stage==uint32_t(grs::BridgeStage::PauseBegin)){gap=true;assert(row.delta>33333333&&row.last>0);}if(i&&row.ns-a.timeline.rows[i-1].ns>=40000000){auto previous=a.timeline.rows[i-1].stage;if(mode==1)assert(previous==uint32_t(grs::BridgeStage::WaitEnter));if(mode==2)assert(previous==uint32_t(grs::BridgeStage::PumpBegin));if(mode==3)assert(previous==uint32_t(grs::BridgeStage::DrawBegin));work=true;}}assert(anchor);if(mode>=1&&mode<=3)assert(gap&&work&&!a.ready&&a.readyEpoch==0);if(mode==0)assert(!gap);if(mode==4)assert(a.staleRejected>0);passed+=3;std::cout<<"mode="<<mode<<" rows="<<a.timeline.count<<" gap="<<gap<<"\n";}
 grs::BridgeTimeline trace;for(uint32_t i=0;i<1100;i++){grs::BridgeTimeRow row;row.ns=i;trace.Add(row);}assert(trace.count==1024&&trace.dropped==76&&trace.Export(1020).find("\"next\":1024")!=std::string::npos);passed++;std::cout<<"{\"revision\":\"T295_PRODUCTION_BRIDGE_LOOP_HOST\",\"passed\":"<<passed<<",\"failed\":0,\"platform\":\"WINDOW_EGL_TRANSPORT_DOUBLE_REAL_RUN_CLOCK\",\"device\":\"NOT_RUN\"}\n";
}
