// Experimental APP_ROUTE. All EGL/GL and ResidentRun calls belong to one thread.
#include "run.h"
#include "scene.h"
#ifdef GRS_APP_M1
#include "m1/run.h"
#include "m1/scene.h"
#include "m1/ui_state.h"
#endif
#include "foundation.h"
#include "bridge_packet.h"
#include "bridge_state.h"
#include "bridge_timeline.h"
#ifndef GRS_BRIDGE_HOST_TEST
#include "bridge_napi.h"
#include <napi/native_api.h>
#include <ace/xcomponent/native_interface_xcomponent.h>
#include <native_window/external_window.h>
#else
#include "bridge_platform_double.h"
#endif
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <sstream>
#include <cstring>
#include <atomic>
#include <cmath>
namespace {
using namespace grs;
#ifdef GRS_APP_M1
class AppTransport:public m1::FullM1Transport {public:U64 InputGeneration()const{const auto& h=Owner().Header();return {h[84],h[85]};}};
using AppRun=m1::Run;using AppScene=m1::Scene;
const char* AppShaderSha(){return m1::FullM1ShaderSha();}
const char* AppLayoutSha(){return m1::manifestSha;}
#else
using AppTransport=SourceGpuTransport;using AppRun=ResidentRun;using AppScene=ResidentScene;
const char* AppShaderSha(){return SourceKernelSha();}
const char* AppLayoutSha(){return GetFoundationInfo().layoutSha;}
#endif
U64 u64(uint64_t n){return {uint32_t(n),uint32_t(n>>32)};}
uint64_t now(){return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();}
void hash(const char* s,std::array<uint32_t,8>& a){for(size_t i=0;i<32;i++){unsigned v=0;for(int j=0;j<2;j++){char c=s[2*i+j];v=v*16+(c<='9'?c-'0':c-'a'+10);}a[i/4]|=v<<((i%4)*8);}}
struct App : BridgeState {
 std::condition_variable wake;std::thread thread;
 bool quit=false;uint32_t Kw=128;
#ifdef GRS_APP_M1
 m1::UiState ui;m1::SceneOptics optics;bool requiresReset=false;
 void UiJsonLocked(std::ostringstream& o){o<<",\"sourceUiRevision\":\"GRS_M1_DEMO_UI_V4\",\"sourceOpen\":"<<(AllowedLocked()&&ui.SourceReady(Stamp())?"true":"false")<<",\"sourceKind\":"<<ui.source<<",\"sourceControlLo\":"<<uint32_t(ui.boundary)<<",\"sourceControlHi\":"<<uint32_t(ui.boundary>>32)<<",\"gravityMode\":\""<<(ui.sensorMode?"SENSOR":"EXPLICIT_MANUAL")<<"\",\"gravityRaw\":"<<ui.rawAngle<<",\"gravityTargetQ\":"<<ui.target<<",\"gravityAcceptedQ\":"<<ui.acceptedAngle<<",\"shakeDiagnostic\":"<<ui.shake<<",\"sourceReceiptStatus\":"<<ui.receipt<<",\"sourceAdmittedQ\":"<<ui.admitted<<",\"sourceRejectedQ\":"<<ui.rejected<<",\"sourceRequestSeq\":"<<ui.request<<",\"settledLo\":"<<ui.settled.lo<<",\"settledHi\":"<<ui.settled.hi<<",\"flightLo\":"<<ui.flight.lo<<",\"flightHi\":"<<ui.flight.hi<<",\"waterTotalLo\":"<<ui.water.lo<<",\"waterTotalHi\":"<<ui.water.hi<<",\"sandTotalLo\":"<<ui.sand.lo<<",\"sandTotalHi\":"<<ui.sand.hi<<",\"gpuBufferAllocatedBytes\":"<<ui.bufferBytes<<",\"frameTargetBytes\":"<<ui.frameBytes<<",\"knownGpuBytesPeak\":"<<ui.knownGpuPeak<<",\"cpuFixedControlBytes\":"<<(sizeof(App)+sizeof(AppRun)+sizeof(AppTransport))<<",\"driverMemory\":\"NOT_MEASURED\",\"calibration\":\"PENDING_CALIBRATION\"";}
 const char* OfferUiLocked(const std::array<uint32_t,32>& w){const char* code=ui.Offer(w,Stamp());return std::strcmp(code,"OK")==0?OfferLocked(w):code;}
 bool SetSourceLocked(int kind){if(!ui.SetSource(kind)){RevokeLocked("SOURCE_CONTROL_EXHAUSTED");wake.notify_one();return false;}cancelled+=count;head=count=0;wake.notify_one();return true;}
 TransportCode PumpControlled(AppRun& run,bool& suspended,uint64_t& last){std::lock_guard<std::mutex> l(mutex);if(ui.applied!=ui.boundary){cancelled+=run.DiscardSources();ui.applied=ui.boundary;}if(!active||!ui.Fresh(Stamp())){if(!suspended){run.Suspend();last=0;suspended=true;SetSourceLocked(-1);RevokeLocked(active?"GRAVITY_STALE":"BACKGROUND");}if(!run.HasFlight())return TransportCode::NOT_READY;}transportLockHeld=true;auto result=run.Pump();transportLockHeld=false;return result;}
#endif
 uint64_t accepted=0,sourceCommit=0,physicsCommit=0,deltaNs=0;uint32_t carryBefore=0,carryAfter=0,quanta=0;int advanceCode=0,lastRunOffer=0;std::string firstFailure="NONE";const char* lastOffer="NONE";
 std::array<Words256,32> completions{};size_t completionCount=0,completionDropped=0;U64 rootNonce{};
#ifndef GRS_BRIDGE_HOST_TEST
 BridgeDecode decoded{};std::array<BridgeDecode,64> offers{};size_t offerCount=0,offerDropped=0;
#endif
 BridgeTimeline timeline;bool timelinePaused=false;bool transportLockHeld=false;uint64_t transportEpoch=0;Identity transportIdentity{};
 uint64_t Stamp()const {
#ifdef GRS_BRIDGE_HOST_TEST
  if(hostNow)return hostNow();
#endif
  return now();
 }
#ifdef GRS_BRIDGE_HOST_TEST
 uint64_t (*hostNow)()=nullptr;void (*hostHook)(App&,BridgeStage)=nullptr;
#endif
 void MarkLocked(BridgeStage stage,uint64_t seen=0,uint64_t last=0,uint64_t delta=0,int code=0,uint32_t carry=0,uint32_t q=0){if(stage==BridgeStage::InitBegin)timelinePaused=false;if(timelinePaused&&stage!=BridgeStage::Invalidate&&stage!=BridgeStage::InitBegin)return;BridgeTimeRow r;r.ns=Stamp();r.tid=BridgeTid();r.epoch=seen?seen:epoch.load();r.run=(uint64_t(identity.run.hi)<<32)|identity.run.lo;r.surface=(uint64_t(identity.surface.hi)<<32)|identity.surface.lo;r.input=(uint64_t(input.hi)<<32)|input.lo;r.stage=uint32_t(stage);r.now=r.ns;r.last=last;r.delta=delta;r.code=uint32_t(code);r.carry=carry;r.quanta=q;r.appQueue=uint32_t(count);r.runQueue=runQueue;r.root=(uint64_t(rootNonce.hi)<<32)|rootNonce.lo;r.flags=(active?1:0)|(window?2:0)|(AllowedLocked()?4:0);timeline.Add(r);if(stage==BridgeStage::PauseEnd)timelinePaused=true;}
 void Mark(BridgeStage stage,uint64_t seen,uint64_t last=0,uint64_t delta=0,int code=0,uint32_t carry=0,uint32_t q=0){std::lock_guard<std::mutex> l(mutex);MarkLocked(stage,seen,last,delta,code,carry,q);}
 void Hook(BridgeStage stage){
#ifdef GRS_BRIDGE_HOST_TEST
  if(hostHook)hostHook(*this,stage);
#else
  (void)stage;
#endif
 }
 App(bool launch=true){
#ifdef GRS_APP_M1
commandRevision=2;waterAllowed=true;
#endif
if(launch)thread=std::thread([this]{loop();});}
 ~App(){{std::lock_guard<std::mutex> l(mutex);quit=true;wake.notify_one();}if(thread.joinable())thread.join();if(window)OH_NativeWindow_NativeObjectUnreference(window);}
 void invalidate(void* w,bool replace){std::lock_guard<std::mutex> l(mutex);
#ifdef GRS_APP_M1
 if(ui.enabled){if(replace&&accepted>0)requiresReset=true;else if(!replace)requiresReset=false;SetSourceLocked(-1);}
#endif
 if(replace){if(w&&OH_NativeWindow_NativeObjectReference(w)!=0){if(window)OH_NativeWindow_NativeObjectUnreference(window);window=nullptr;InvalidateLocked();permission=phase="FAULT";status="{\"state\":\"FAULT\",\"stage\":\"WINDOW_REFERENCE\"}";wake.notify_one();return;}if(window)OH_NativeWindow_NativeObjectUnreference(window);window=w;}InvalidateLocked();
#ifdef GRS_APP_M1
 if(requiresReset){permission=phase="OWNER_STATE_LOST";status="{\"state\":\"FAULT\",\"stage\":\"OWNER_STATE_LOST\",\"newRunRequired\":true}";}
#endif
 MarkLocked(BridgeStage::Invalidate);wake.notify_one();}
 bool setActive(bool value){std::lock_guard<std::mutex> l(mutex);if(active==value)return false;active=value;
#ifdef GRS_APP_M1
 if(ui.enabled){SetSourceLocked(-1);RevokeLocked(value?"RESUMING":"BACKGROUND");wake.notify_one();return true;}
#endif
 InvalidateLocked();MarkLocked(BridgeStage::Invalidate);wake.notify_one();return true;}
 void loop(){
  EGLDisplay display=EGL_NO_DISPLAY;EGLContext context=EGL_NO_CONTEXT;EGLSurface surface=EGL_NO_SURFACE;
  void* held=nullptr;uint64_t seen=0,runId=0,last=0,frame=0,render=0,lastAccepted=0;bool failed=false,gap=false;
  std::unique_ptr<AppTransport> gpu;std::unique_ptr<AppRun> run;AppScene scene;
#ifdef GRS_APP_M1
 bool suspended=false;m1::SceneOptics activeOptics;
#endif
  auto retire=[&]{run.reset();if(gpu)gpu->ContextLost();gpu.reset();scene.Reset(false);if(display!=EGL_NO_DISPLAY){eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);if(surface!=EGL_NO_SURFACE)eglDestroySurface(display,surface);if(context!=EGL_NO_CONTEXT)eglDestroyContext(display,context);eglTerminate(display);}display=EGL_NO_DISPLAY;surface=EGL_NO_SURFACE;context=EGL_NO_CONTEXT;if(held)OH_NativeWindow_NativeObjectUnreference(held);held=nullptr;last=frame=render=lastAccepted=0;};
  auto publish=[&](const std::string& s){std::lock_guard<std::mutex> l(mutex);if(CurrentLocked(seen,held)){status=s;if(s.find("FAULT")!=std::string::npos){RevokeLocked("FAULT");phase="FAULT";if(firstFailure=="NONE"){auto at=s.find("\"stage\":\"");if(at!=std::string::npos){at+=9;auto end=s.find('"',at);firstFailure=s.substr(at,end-at);}else firstFailure=phase;}}}};
  while(true){
   uint64_t currentEpoch;bool enabled;uint32_t capacity,waterCapacity;int requestedAngle;
#ifdef GRS_APP_M1
   bool resetBlocked;
#endif
   {std::unique_lock<std::mutex> l(mutex);MarkLocked(BridgeStage::WaitEnter);
#ifdef GRS_BRIDGE_HOST_TEST
if(hostHook){l.unlock();Hook(BridgeStage::Wake);l.lock();}else
#endif
wake.wait_for(l,std::chrono::milliseconds(8));MarkLocked(BridgeStage::Wake);if(quit)break;currentEpoch=epoch;enabled=active;capacity=K;waterCapacity=Kw;requestedAngle=angle;
#ifdef GRS_APP_M1
 resetBlocked=requiresReset;
#endif
 }
   if(currentEpoch!=seen
#ifdef GRS_APP_M1
      ||(!gpu&&enabled&&!failed&&!resetBlocked)
#endif
   ){retire();seen=currentEpoch;failed=gap=false;{std::lock_guard<std::mutex> l(mutex);if(seen!=epoch)continue;
#ifdef GRS_APP_M1
    suspended=false;activeOptics=optics;ui.NewRun();
#endif
    if(active&&window
#ifdef GRS_APP_M1
       &&!requiresReset
#endif
       &&OH_NativeWindow_NativeObjectReference(window)==0)held=window;}if(!held)continue;
    Mark(BridgeStage::InitBegin,seen);
#if !defined(GRS_BRIDGE_HOST_TEST) || defined(GRS_BRIDGE_HOST_GL)
    display=eglGetDisplay(EGL_DEFAULT_DISPLAY);EGLint major,minor;EGLConfig config;EGLint n=0;
    const EGLint attrs[]={EGL_SURFACE_TYPE,EGL_WINDOW_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_NONE};const EGLint ctx[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE};
    bool good=display!=EGL_NO_DISPLAY&&eglInitialize(display,&major,&minor)&&eglBindAPI(EGL_OPENGL_ES_API)&&eglChooseConfig(display,attrs,&config,1,&n)&&n==1;
    if(good){context=eglCreateContext(display,config,EGL_NO_CONTEXT,ctx);surface=eglCreateWindowSurface(display,config,reinterpret_cast<EGLNativeWindowType>(held),nullptr);good=context!=EGL_NO_CONTEXT&&surface!=EGL_NO_SURFACE&&eglMakeCurrent(display,surface,surface,context);}
#else
    display=reinterpret_cast<EGLDisplay>(1);context=reinterpret_cast<EGLContext>(1);surface=reinterpret_cast<EGLSurface>(1);bool good=true;
#endif
    if(!good){publish("{\"state\":\"FAULT\",\"stage\":\"EGL_CREATE\",\"rawEGL\":"+std::to_string(eglGetError())+"}");failed=true;continue;}
    Identity id;id.run=u64(++runId);id.owner={1,0};id.context=id.surface=id.geometry=u64(seen);id.config={1,0};auto f=GetFoundationInfo();hash(AppLayoutSha(),id.layoutSha);hash(f.angleSha,id.angleSha);hash(AppShaderSha(),id.shaderSha);
    transportEpoch=seen;transportIdentity=id;gpu=std::make_unique<AppTransport>();
    gpu->submitObserver={this,[](void* ptr,SubmitStage stage,uint64_t root,uint32_t code,uint32_t ordinal){auto& app=*static_cast<App*>(ptr);std::unique_lock<std::mutex> lock(app.mutex,std::defer_lock);if(!app.transportLockHeld)lock.lock();BridgeTimeRow row;row.ns=row.now=app.Stamp();row.tid=BridgeTid();row.epoch=app.transportEpoch;row.run=(uint64_t(app.transportIdentity.run.hi)<<32)|app.transportIdentity.run.lo;row.surface=(uint64_t(app.transportIdentity.surface.hi)<<32)|app.transportIdentity.surface.lo;row.input=(uint64_t(app.input.hi)<<32)|app.input.lo;row.root=root;row.stage=uint32_t(stage);row.code=code;row.carry=ordinal;if(!app.timelinePaused)app.timeline.Add(row);}};
#ifdef GRS_APP_M1
    auto code=gpu->Create(capacity,waterCapacity,id);
#else
    auto code=gpu->Create(capacity,id);
#endif
    if(code!=TransportCode::OK){publish("{\"state\":\"FAULT\",\"stage\":\"TRANSPORT_CREATE\",\"code\":"+std::to_string(int(code))+"}");failed=true;continue;}run=std::make_unique<AppRun>(*gpu);bool committed=false;{std::lock_guard<std::mutex> l(mutex);committed=CommitReadyLocked(seen,held,id,gpu->InputGeneration());MarkLocked(committed?BridgeStage::Ready:BridgeStage::ReadyRejected,seen);if(committed){accepted=sourceCommit=physicsCommit=0;completionCount=completionDropped=0;rootNonce={};firstFailure="NONE";}}if(!committed){retire();continue;}
   }
#ifdef GRS_APP_M1
   if(run&&!failed&&!gap){
    bool pause;const char* why;
    {std::lock_guard<std::mutex> l(mutex);if(seen!=epoch||held!=window)continue;if(ui.applied!=ui.boundary){cancelled+=run->DiscardSources();ui.applied=ui.boundary;}pause=!active||!ui.Fresh(Stamp());why=!active?"BACKGROUND":"GRAVITY_STALE";if(ui.enabled&&ui.sensorMode)requestedAngle=ui.target;ui.Memory(gpu->AllocatedBytes(),ui.frameBytes);}
    if(pause||suspended){
     if(!suspended){run->Suspend();last=0;suspended=true;std::lock_guard<std::mutex> l(mutex);SetSourceLocked(-1);RevokeLocked(why);}
     if(run->HasFlight()){auto c=run->Pump();if(c==TransportCode::BUSY)continue;if(c!=TransportCode::OK){failed=true;std::lock_guard<std::mutex> l(mutex);RevokeLocked("FAULT");status="{\"state\":\"FAULT\",\"stage\":\"PAUSE_DRAIN\"}";continue;}}
     {std::lock_guard<std::mutex> l(mutex);if(seen!=epoch||held!=window)continue;const auto& h=gpu->Owner().Header();accepted=gpu->Owner().AcceptedCount();sourceCommit=(uint64_t(h[15])<<32)|h[14];physicsCommit=(uint64_t(h[17])<<32)|h[16];ui.Completion(gpu->LastCompletion());runQueue=run->Queued();phase=why;permission=why;status="{\"state\":\"STALE\",\"stage\":\""+std::string(why)+"\",\"renderedToken\":\""+scene.LastGoodToken()+"\",\"current\":false}";}
     lastAccepted=accepted;if(pause)continue;
     run->Resume();last=0;suspended=false;{std::lock_guard<std::mutex> l(mutex);CommitReadyLocked(seen,held,identity,gpu->InputGeneration());}
    }
   }
#endif
   if(!enabled||!run)continue;{std::lock_guard<std::mutex> l(mutex);if(!CurrentLocked(seen,held))continue;}
   if(gap){Mark(BridgeStage::PumpBegin,seen,last);Hook(BridgeStage::PumpBegin);auto submitted=run->Tick().submitted,wasAccepted=run->Tick().accepted;auto code=run->Pump();Mark(BridgeStage::PumpEnd,seen,last,0,int(code));if(run->Tick().submitted>submitted){std::lock_guard<std::mutex> l(mutex);if(gpu->Trace().Size())rootNonce=gpu->Trace().At(gpu->Trace().Size()-1).root.nonce;MarkLocked(BridgeStage::RootSubmit,seen,last);}else if(run->Tick().active||run->Tick().accepted>wasAccepted)Mark(BridgeStage::RootPoll,seen,last,0,int(code));if(code!=TransportCode::BUSY){std::lock_guard<std::mutex> l(mutex);if(CurrentLocked(seen,held))phase="NEW_RUN_REQUIRED";}continue;}
   if(failed)continue;
   const auto time=Stamp();const uint64_t delta=last?time-last:1;if(!last)Mark(BridgeStage::FirstTick,seen,last,delta);
   if(delta>33333333u){
    {std::lock_guard<std::mutex> l(mutex);if(!CurrentLocked(seen,held))continue;MarkLocked(BridgeStage::PauseBegin,seen,last,delta);RevokeLocked("PAUSED_CLOCK");deltaNs=delta;carryBefore=run->Tick().time.carryAfter;cancelled+=run->Queued();phase="CLOCK_GAP";firstFailure="CLOCK_GAP";status="{\"state\":\"PAUSED_CLOCK\",\"stage\":\"CLOCK_GAP\",\"newRunRequired\":true}";}
    auto pause=run->Pause();Mark(BridgeStage::PauseEnd,seen,last,delta,int(pause));last=0;gap=true;{std::lock_guard<std::mutex> l(mutex);if(!CurrentLocked(seen,held))continue;input=gpu->InputGeneration();runQueue=run->Queued();carryAfter=0;advanceCode=int(pause);status="{\"state\":\"PAUSED_CLOCK\",\"stage\":\"CLOCK_GAP\",\"newRunRequired\":true}";}continue;
   }
   {std::lock_guard<std::mutex> l(mutex);if(!CurrentLocked(seen,held))continue;std::array<uint32_t,32> command{};if(FrontLocked(seen,gpu->InputGeneration(),command)){MarkLocked(BridgeStage::Consume,seen);auto c=run->Offer(command);lastRunOffer=int(c);if(c==ControlCode::OK)PopLocked();}}
   if(!run->Tick().active){++frame;Mark(BridgeStage::AdvanceBegin,seen,last,delta,0,run->Tick().time.carryAfter);auto code=run->Advance(delta,u64(frame),u64(frame),requestedAngle,capacity);
    {std::lock_guard<std::mutex> l(mutex);if(!CurrentLocked(seen,held))continue;MarkLocked(BridgeStage::AdvanceEnd,seen,last,delta,int(code),run->Tick().time.carryAfter,run->Tick().time.quanta);advanceCode=int(code);deltaNs=delta;carryBefore=run->Tick().time.carryBefore;carryAfter=run->Tick().time.carryAfter;quanta=run->Tick().time.quanta;}
    if(code==ControlCode::BUSY||code==ControlCode::BACKPRESSURE)continue;
    if(code!=ControlCode::OK){failed=true;{std::lock_guard<std::mutex> l(mutex);if(CurrentLocked(seen,held))phase="ADVANCE_INVALID";}publish("{\"state\":\"FAULT\",\"stage\":\"ADVANCE_INVALID\",\"code\":"+std::to_string(int(code))+"}");continue;}last=time;Mark(BridgeStage::AnchorSet,seen,last,delta);
   }
   Mark(BridgeStage::PumpBegin,seen,last);Hook(BridgeStage::PumpBegin);auto submitted=run->Tick().submitted,wasAccepted=run->Tick().accepted;auto code=
#ifdef GRS_APP_M1
PumpControlled(*run,suspended,last);
#else
run->Pump();
#endif
Mark(BridgeStage::PumpEnd,seen,last,0,int(code));if(run->Tick().submitted>submitted){std::lock_guard<std::mutex> l(mutex);if(gpu->Trace().Size())rootNonce=gpu->Trace().At(gpu->Trace().Size()-1).root.nonce;MarkLocked(BridgeStage::RootSubmit,seen,last);}else if(run->Tick().active||run->Tick().accepted>wasAccepted)Mark(BridgeStage::RootPoll,seen,last,0,int(code));if(last&&Stamp()-last>33333333u){
    const auto elapsed=Stamp()-last;{std::lock_guard<std::mutex> lock(mutex);if(!CurrentLocked(seen,held))continue;MarkLocked(BridgeStage::PauseBegin,seen,last,elapsed);RevokeLocked("PAUSED_CLOCK");deltaNs=elapsed;firstFailure="CLOCK_GAP";phase="CLOCK_GAP";status="{\"state\":\"PAUSED_CLOCK\",\"stage\":\"CLOCK_GAP\",\"newRunRequired\":true}";}
    auto paused=run->Pause();Mark(BridgeStage::PauseEnd,seen,last,elapsed,int(paused));last=0;gap=true;continue;
   }if(run->Tick().fault){failed=true;publish("{\"state\":\"FAULT\",\"stage\":\"PRODUCT_COMPLETION\",\"code\":"+std::to_string(int(code))+"}");continue;}
   uint64_t acceptedNow=gpu->Owner().AcceptedCount();{std::lock_guard<std::mutex> l(mutex);if(!CurrentLocked(seen,held))continue;accepted=acceptedNow;runQueue=run->Queued();input=gpu->InputGeneration();const auto& h=gpu->Owner().Header();sourceCommit=(uint64_t(h[15])<<32)|h[14];physicsCommit=(uint64_t(h[17])<<32)|h[16];}if(acceptedNow>lastAccepted){lastAccepted=acceptedNow;const auto& a=run->Tick().latest;{std::lock_guard<std::mutex> l(mutex);if(!CurrentLocked(seen,held))continue;rootNonce=a.nonce;if(completionCount<completions.size())completions[completionCount++]=a.completion;else completionDropped++;}EGLint width=0,height=0;eglQuerySurface(display,surface,EGL_WIDTH,&width);eglQuerySurface(display,surface,EGL_HEIGHT,&height);
    ResidentSceneFrame f;f.run=u64(runId);f.context=f.surface=f.geometry=u64(seen);f.renderFrame=++render;f.width=width;f.height=height;f.boxWidth=float(width);f.boxHeight=float(height);
    struct Swap {App* app;uint64_t epoch;EGLDisplay d;EGLSurface s;};Swap args{this,seen,display,surface};
    auto swapped=[](void* ptr){auto& v=*static_cast<Swap*>(ptr);return [&]{v.app->Mark(BridgeStage::SwapBegin,v.epoch);bool ok=v.epoch==v.app->epoch.load()&&eglSwapBuffers(v.d,v.s)==EGL_TRUE;v.app->Mark(BridgeStage::SwapEnd,v.epoch,0,0,ok?0:1);return ok;}();};
    Mark(BridgeStage::DrawBegin,seen,last);Hook(BridgeStage::DrawBegin);auto receipt=scene.Draw(*gpu,a.publication,a.publicationGeneration,f,swapped,&args
#ifdef GRS_APP_M1
     ,activeOptics
#endif
    );
#ifdef GRS_APP_M1
    {std::lock_guard<std::mutex> l(mutex);ui.Completion(a.completion);ui.Memory(gpu->AllocatedBytes(),receipt.frameTargetBytes);}
#endif
    Mark(BridgeStage::DrawEnd,seen,last,0,receipt.current?0:1);
    std::ostringstream out;out<<"{\"state\":\""<<(receipt.current?"CURRENT":"FAULT")<<"\",\"stage\":\""<<receipt.stage<<"\",\"run\":"<<runId<<",\"epoch\":"<<seen<<",\"sourceCommit\":"<<a.source.lo<<",\"physicsCommit\":"<<a.physics.lo<<",\"completionBytes\":1024,\"displayPresentVerified\":false,\"requestedToken\":\""<<receipt.requestedToken<<"\",\"renderedToken\":\""<<receipt.renderedToken<<"\",\"rawGL\":"<<receipt.rawGl<<",\"rawEGL\":"<<receipt.rawEgl<<"}";publish(out.str());if(!receipt.current)failed=true;
   }
  }retire();
 }
};
#ifndef GRS_BRIDGE_HOST_TEST
App& app(){static App a;return a;}
void created(OH_NativeXComponent*,void* w){app().invalidate(w,true);}
void changed(OH_NativeXComponent*,void* w){app().invalidate(w,true);}
void destroyed(OH_NativeXComponent*,void*){app().invalidate(nullptr,true);}
OH_NativeXComponent_Callback callbacks{created,changed,destroyed,nullptr};
napi_value text(napi_env e,const std::string& s){napi_value v;napi_create_string_utf8(e,s.c_str(),s.size(),&v);return v;}
napi_value status(napi_env e,napi_callback_info){auto& a=app();std::lock_guard<std::mutex> l(a.mutex);std::ostringstream o;std::string base=a.status;if(!base.empty())base.pop_back();o<<base<<",\"timelineCount\":"<<a.timeline.count<<",\"timelineDropped\":"<<a.timeline.dropped<<",\"timelineSerial\":"<<a.timeline.serial<<",\"timelineApi\":\"timeline(cursor),4rows,page\",\"ready\":"<<(a.AllowedLocked()?"true":"false")<<",\"epoch\":"<<a.epoch.load()<<",\"readyEpoch\":"<<a.readyEpoch<<",\"permissionState\":\""<<a.permission<<"\",\"staleQueuedRejected\":"<<a.staleRejected<<",\"runQueueSample\":\"POST_PUMP_OR_POST_PAUSE\",\"runLo\":"<<a.identity.run.lo<<",\"runHi\":"<<a.identity.run.hi<<",\"geometryLo\":"<<a.identity.geometry.lo<<",\"geometryHi\":"<<a.identity.geometry.hi<<",\"configLo\":"<<a.identity.config.lo<<",\"configHi\":"<<a.identity.config.hi<<",\"inputLo\":"<<a.input.lo<<",\"inputHi\":"<<a.input.hi<<",\"ownerLo\":"<<a.identity.owner.lo<<",\"ownerHi\":"<<a.identity.owner.hi<<",\"contextLo\":"<<a.identity.context.lo<<",\"contextHi\":"<<a.identity.context.hi<<",\"surfaceLo\":"<<a.identity.surface.lo<<",\"surfaceHi\":"<<a.identity.surface.hi<<",\"appQueueDepth\":"<<a.count<<",\"runQueueDepth\":"<<a.runQueue<<",\"lastOfferCode\":\""<<a.lastOffer<<"\",\"lastRunOfferCode\":"<<a.lastRunOffer<<",\"napiStage\":\""<<a.decoded.stage<<"\",\"napiStatus\":"<<int(a.decoded.status)<<",\"typedType\":"<<a.decoded.type<<",\"typedLength\":"<<a.decoded.length<<",\"byteOffset\":"<<a.decoded.offset<<",\"backingBytes\":"<<a.decoded.backingBytes<<",\"advanceCode\":"<<a.advanceCode<<",\"clockDeltaNs\":"<<a.deltaNs<<",\"carryBefore\":"<<a.carryBefore<<",\"carryAfter\":"<<a.carryAfter<<",\"plannedQuanta\":"<<a.quanta<<",\"clockPhase\":\""<<a.phase<<"\",\"firstFailureStage\":\""<<a.firstFailure<<"\",\"acceptedCount\":"<<a.accepted<<",\"cancelledQueued\":"<<a.cancelled<<",\"offerDropped\":"<<a.offerDropped<<",\"rootNonceLo\":"<<a.rootNonce.lo<<",\"rootNonceHi\":"<<a.rootNonce.hi<<",\"sourceCommitLo\":"<<uint32_t(a.sourceCommit)<<",\"sourceCommitHi\":"<<uint32_t(a.sourceCommit>>32)<<",\"physicsCommitLo\":"<<uint32_t(a.physicsCommit)<<",\"physicsCommitHi\":"<<uint32_t(a.physicsCommit>>32)<<"}";
#ifdef GRS_APP_M1
std::string baseUi=o.str();if(!baseUi.empty())baseUi.pop_back();o.str("");o.clear();o<<baseUi;a.UiJsonLocked(o);o<<"}";
#endif
return text(e,o.str());}
napi_value active(napi_env e,napi_callback_info i){size_t n=1;napi_value v;bool value=false;napi_get_cb_info(e,i,&n,&v,nullptr,nullptr);if(n!=1||napi_get_value_bool(e,v,&value)!=napi_ok)return text(e,"INVALID_ACTIVE");bool changed=app().setActive(value);
#ifdef GRS_APP_M1
return text(e,changed?"ACTIVITY_BOUNDARY_REQUESTED":"ACTIVE_UNCHANGED");
#else
return text(e,changed?"NEW_RUN_REQUESTED":"ACTIVE_UNCHANGED");
#endif
}
napi_value angle(napi_env e,napi_callback_info i){size_t n=1;napi_value v;double value=0;napi_get_cb_info(e,i,&n,&v,nullptr,nullptr);if(n!=1||napi_get_value_double(e,v,&value)!=napi_ok||!std::isfinite(value)||value<-7680||value>7680||std::floor(value)!=value)return text(e,"INVALID_ANGLE");auto& a=app();std::lock_guard<std::mutex> l(a.mutex);
#ifdef GRS_APP_M1
if(a.ui.enabled&&a.ui.sensorMode)return text(e,"SENSOR_MODE_ACTIVE");a.ui.rawAngle=value/256;a.ui.target=int(value);
#endif
a.angle=int(value);return text(e,"ANGLE_ENQUEUED");}
napi_value newRun(napi_env e,napi_callback_info){app().invalidate(nullptr,false);return text(e,"NEW_RUN_REQUESTED");}
napi_value offer(napi_env e,napi_callback_info i){auto d=DecodeBridgeCommand(e,i);auto& a=app();std::lock_guard<std::mutex> l(a.mutex);a.decoded=d;if(a.offerCount<a.offers.size())a.offers[a.offerCount++]=d;else a.offerDropped++;a.lastOffer=d.stage;if(std::strcmp(d.stage,"OK")!=0){a.MarkLocked(BridgeStage::Offer,0,0,0,2);return text(e,d.stage);}
#ifdef GRS_APP_M1
a.lastOffer=a.OfferUiLocked(d.words);
#else
a.lastOffer=a.OfferLocked(d.words);
#endif
a.MarkLocked(BridgeStage::Offer,0,0,0,std::strcmp(a.lastOffer,"ENQUEUED_NOT_ACK")==0?0:1);if(std::strcmp(a.lastOffer,"ENQUEUED_NOT_ACK")==0)a.wake.notify_one();return text(e,a.lastOffer);}

napi_value diagnostics(napi_env e,napi_callback_info){auto& a=app();std::lock_guard<std::mutex> l(a.mutex);std::ostringstream o;o<<"{\"revision\":\"T272_BRIDGE_V1\",\"dropped\":"<<a.offerDropped<<",\"offers\":[";for(size_t j=0;j<a.offerCount;j++){if(j)o<<',';const auto& d=a.offers[j];o<<"{\"stage\":\""<<d.stage<<"\",\"napiStatus\":"<<int(d.status)<<",\"words\":[";for(size_t k=0;k<32;k++){if(k)o<<',';o<<d.words[k];}o<<"]}";}o<<"],\"completionDropped\":"<<a.completionDropped<<",\"completions\":[";for(size_t j=0;j<a.completionCount;j++){if(j)o<<',';o<<'[';for(size_t k=0;k<256;k++){if(k)o<<',';o<<a.completions[j][k];}o<<']';}o<<"]}";return text(e,o.str());}
napi_value timeline(napi_env e,napi_callback_info i){size_t n=1;napi_value v;double cursor=0;napi_get_cb_info(e,i,&n,&v,nullptr,nullptr);if(n!=1||napi_get_value_double(e,v,&cursor)!=napi_ok||!std::isfinite(cursor)||cursor<0||cursor>BridgeTimeline::capacity||std::floor(cursor)!=cursor)return text(e,"INVALID_CURSOR");auto& a=app();std::lock_guard<std::mutex> l(a.mutex);return text(e,a.timeline.Export(uint32_t(cursor)));}
#ifdef GRS_APP_M1
napi_value sourceMode(napi_env e,napi_callback_info i){size_t n=1;napi_value v;double kind=0;if(napi_get_cb_info(e,i,&n,&v,nullptr,nullptr)!=napi_ok||n!=1||napi_get_value_double(e,v,&kind)!=napi_ok||(kind!=-1&&kind!=0&&kind!=1))return text(e,"INVALID_SOURCE");auto& a=app();std::lock_guard<std::mutex> l(a.mutex);return text(e,a.SetSourceLocked(int(kind))?"SOURCE_BOUNDARY_REQUESTED":"SOURCE_CONTROL_EXHAUSTED");}
napi_value gravityMode(napi_env e,napi_callback_info i){size_t n=1;napi_value v;bool sensor=true;if(napi_get_cb_info(e,i,&n,&v,nullptr,nullptr)!=napi_ok||n!=1||napi_get_value_bool(e,v,&sensor)!=napi_ok)return text(e,"INVALID_GRAVITY_MODE");auto& a=app();std::lock_guard<std::mutex> l(a.mutex);if(!a.ui.SetMode(sensor))return text(e,"SOURCE_CONTROL_EXHAUSTED");a.cancelled+=a.count;a.head=a.count=0;a.wake.notify_one();return text(e,sensor?"SENSOR_MODE":"EXPLICIT_MANUAL_MODE");}
napi_value gravitySample(napi_env e,napi_callback_info i){size_t n=3;napi_value v[3];double raw=0,shake=0,seq=0;if(napi_get_cb_info(e,i,&n,v,nullptr,nullptr)!=napi_ok||n!=3||napi_get_value_double(e,v[0],&raw)!=napi_ok||napi_get_value_double(e,v[1],&shake)!=napi_ok||napi_get_value_double(e,v[2],&seq)!=napi_ok||!std::isfinite(seq)||seq<1||seq>UINT32_MAX||std::floor(seq)!=seq)return text(e,"INVALID_GRAVITY_SAMPLE");auto& a=app();std::lock_guard<std::mutex> l(a.mutex);if(!a.ui.enabled||!a.ui.sensorMode||!a.ui.Sample(raw,shake,uint32_t(seq),a.Stamp()))return text(e,"INVALID_GRAVITY_SAMPLE");a.wake.notify_one();return text(e,"SENSOR_TARGET_ENQUEUED_NOT_ACCEPTED");}
napi_value opticsConfig(napi_env e,napi_callback_info i){size_t n=1;napi_value v;bool typed=false;napi_typedarray_type type;napi_value backing;size_t len=0,offset=0,bytes=0;void* ptr=nullptr;void* base=nullptr;if(napi_get_cb_info(e,i,&n,&v,nullptr,nullptr)!=napi_ok||n!=1||napi_is_typedarray(e,v,&typed)!=napi_ok||!typed||napi_get_typedarray_info(e,v,&type,&len,&ptr,&backing,&offset)!=napi_ok||type!=napi_float32_array||len!=16||!ptr||offset%4||napi_get_arraybuffer_info(e,backing,&base,&bytes)!=napi_ok||!base||offset>bytes||bytes-offset<64)return text(e,"INVALID_OPTICS16");m1::SceneOptics cold;std::memcpy(cold.values.data(),ptr,64);if(!m1::ValidOptics(cold))return text(e,"INVALID_OPTICS16");auto& a=app();std::lock_guard<std::mutex> l(a.mutex);a.SetSourceLocked(-1);a.optics=cold;a.requiresReset=false;a.InvalidateLocked();a.MarkLocked(BridgeStage::Invalidate);a.wake.notify_one();return text(e,"NEW_EMPTY_M1_OPTICS_RUN_REQUESTED");}
napi_value configure(napi_env e,napi_callback_info i){size_t n=4;napi_value v[4];double k=0,kw=0,feature=0,revision=0;if(napi_get_cb_info(e,i,&n,v,nullptr,nullptr)!=napi_ok||n!=4||napi_get_value_double(e,v[0],&k)!=napi_ok||napi_get_value_double(e,v[1],&kw)!=napi_ok||napi_get_value_double(e,v[2],&feature)!=napi_ok||napi_get_value_double(e,v[3],&revision)!=napi_ok||(k!=192&&k!=512)||(kw!=64&&kw!=128)||feature!=3||revision!=2)return text(e,"UNSUPPORTED_M1_CONFIG");auto& a=app();std::lock_guard<std::mutex> l(a.mutex);a.K=uint32_t(k);a.Kw=uint32_t(kw);a.requiresReset=false;a.InvalidateLocked();a.MarkLocked(BridgeStage::Invalidate);a.wake.notify_one();return text(e,"NEW_EMPTY_M1_RUN_REQUESTED");}
#endif
napi_value init(napi_env e,napi_value out){napi_property_descriptor d[]={
#ifdef GRS_APP_M1
{"setSource",nullptr,sourceMode,nullptr,nullptr,nullptr,napi_default,nullptr},{"setGravityMode",nullptr,gravityMode,nullptr,nullptr,nullptr,napi_default,nullptr},{"setGravitySample",nullptr,gravitySample,nullptr,nullptr,nullptr,napi_default,nullptr},{"configureOptics",nullptr,opticsConfig,nullptr,nullptr,nullptr,napi_default,nullptr},
{"configureM1",nullptr,configure,nullptr,nullptr,nullptr,napi_default,nullptr},
#endif
{"timeline",nullptr,timeline,nullptr,nullptr,nullptr,napi_default,nullptr},{"diagnostics",nullptr,diagnostics,nullptr,nullptr,nullptr,napi_default,nullptr},{"newRun",nullptr,newRun,nullptr,nullptr,nullptr,napi_default,nullptr},{"status",nullptr,status,nullptr,nullptr,nullptr,napi_default,nullptr},{"setActive",nullptr,active,nullptr,nullptr,nullptr,napi_default,nullptr},{"setAngle",nullptr,angle,nullptr,nullptr,nullptr,napi_default,nullptr},{"offer",nullptr,offer,nullptr,nullptr,nullptr,napi_default,nullptr}};napi_define_properties(e,out,sizeof(d)/sizeof(d[0]),d);bool has=false;napi_has_named_property(e,out,OH_NATIVE_XCOMPONENT_OBJ,&has);if(has){napi_value v;OH_NativeXComponent* c=nullptr;napi_get_named_property(e,out,OH_NATIVE_XCOMPONENT_OBJ,&v);napi_unwrap(e,v,reinterpret_cast<void**>(&c));if(c)OH_NativeXComponent_RegisterCallback(c,&callbacks);}return out;}
#ifdef GRS_APP_M1
napi_module module={1,0,nullptr,init,"grsm1",nullptr,{0}};
#else
napi_module module={1,0,nullptr,init,"grsm0",nullptr,{0}};
#endif
#endif
}
#ifndef GRS_BRIDGE_HOST_TEST
extern "C" __attribute__((constructor)) void RegisterGrsM0(){napi_module_register(&module);}
#endif
