// Experimental APP_ROUTE. All EGL/GL and ResidentRun calls belong to one thread.
#include "run.h"
#include "scene.h"
#include "foundation.h"
#include <napi/native_api.h>
#include <ace/xcomponent/native_interface_xcomponent.h>
#include <native_window/external_window.h>
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
U64 u64(uint64_t n){return {uint32_t(n),uint32_t(n>>32)};}
uint64_t now(){return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();}
void hash(const char* s,std::array<uint32_t,8>& a){for(size_t i=0;i<32;i++){unsigned v=0;for(int j=0;j<2;j++){char c=s[2*i+j];v=v*16+(c<='9'?c-'0':c-'a'+10);}a[i/4]|=v<<((i%4)*8);}}
struct App {
 std::mutex mutex;std::condition_variable wake;std::thread thread;
 void* window=nullptr;std::atomic<uint64_t> epoch{1};bool active=true,quit=false;uint32_t K=192;int angle=0;
 std::array<std::array<uint32_t,32>,16> queue{};size_t head=0,count=0;
 std::string status="{\"state\":\"WAIT_WINDOW\"}";
 App(){thread=std::thread([this]{loop();});}
 ~App(){{std::lock_guard<std::mutex> l(mutex);quit=true;wake.notify_one();}thread.join();if(window)OH_NativeWindow_NativeObjectUnreference(window);}
 void invalidate(void* w,bool replace){std::lock_guard<std::mutex> l(mutex);if(replace){if(w&&OH_NativeWindow_NativeObjectReference(w)!=0){status="{\"state\":\"FAULT\",\"stage\":\"WINDOW_REFERENCE\"}";return;}if(window)OH_NativeWindow_NativeObjectUnreference(window);window=w;}++epoch;head=count=0;status="{\"state\":\"STATE_LOST\",\"newRunRequired\":true}";wake.notify_one();}
 void loop(){
  EGLDisplay display=EGL_NO_DISPLAY;EGLContext context=EGL_NO_CONTEXT;EGLSurface surface=EGL_NO_SURFACE;
  void* held=nullptr;uint64_t seen=0,runId=0,last=0,frame=0,render=0,lastAccepted=0;bool failed=false;
  std::unique_ptr<SourceGpuTransport> gpu;std::unique_ptr<ResidentRun> run;ResidentScene scene;
  auto retire=[&]{run.reset();if(gpu)gpu->ContextLost();gpu.reset();scene.Reset(false);if(display!=EGL_NO_DISPLAY){eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);if(surface!=EGL_NO_SURFACE)eglDestroySurface(display,surface);if(context!=EGL_NO_CONTEXT)eglDestroyContext(display,context);eglTerminate(display);}display=EGL_NO_DISPLAY;surface=EGL_NO_SURFACE;context=EGL_NO_CONTEXT;if(held)OH_NativeWindow_NativeObjectUnreference(held);held=nullptr;last=frame=render=lastAccepted=0;};
  auto publish=[&](const std::string& s){std::lock_guard<std::mutex> l(mutex);if(seen==epoch)status=s;};
  while(true){
   uint64_t currentEpoch;bool enabled;uint32_t capacity;int requestedAngle;
   {std::unique_lock<std::mutex> l(mutex);wake.wait_for(l,std::chrono::milliseconds(8));if(quit)break;currentEpoch=epoch;enabled=active;capacity=K;requestedAngle=angle;}
   if(currentEpoch!=seen){retire();seen=currentEpoch;failed=false;{std::lock_guard<std::mutex> l(mutex);if(seen!=epoch)continue;if(active&&window&&OH_NativeWindow_NativeObjectReference(window)==0)held=window;}if(!held)continue;
    display=eglGetDisplay(EGL_DEFAULT_DISPLAY);EGLint major,minor;EGLConfig config;EGLint n=0;
    const EGLint attrs[]={EGL_SURFACE_TYPE,EGL_WINDOW_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_NONE};const EGLint ctx[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE};
    bool good=display!=EGL_NO_DISPLAY&&eglInitialize(display,&major,&minor)&&eglBindAPI(EGL_OPENGL_ES_API)&&eglChooseConfig(display,attrs,&config,1,&n)&&n==1;
    if(good){context=eglCreateContext(display,config,EGL_NO_CONTEXT,ctx);surface=eglCreateWindowSurface(display,config,reinterpret_cast<EGLNativeWindowType>(held),nullptr);good=context!=EGL_NO_CONTEXT&&surface!=EGL_NO_SURFACE&&eglMakeCurrent(display,surface,surface,context);}
    if(!good){publish("{\"state\":\"FAULT\",\"stage\":\"EGL_CREATE\",\"rawEGL\":"+std::to_string(eglGetError())+"}");failed=true;continue;}
    Identity id;id.run=u64(++runId);id.owner={1,0};id.context=id.surface=id.geometry=u64(seen);id.config={1,0};auto f=GetFoundationInfo();hash(f.layoutSha,id.layoutSha);hash(f.angleSha,id.angleSha);hash(SourceKernelSha(),id.shaderSha);
    gpu=std::make_unique<SourceGpuTransport>();auto code=gpu->Create(capacity,id);if(code!=TransportCode::OK){publish("{\"state\":\"FAULT\",\"stage\":\"TRANSPORT_CREATE\",\"code\":"+std::to_string(int(code))+"}");failed=true;continue;}run=std::make_unique<ResidentRun>(*gpu);publish("{\"state\":\"READY\",\"run\":"+std::to_string(runId)+",\"epoch\":"+std::to_string(seen)+"}");
   }
   if(!enabled||failed||!run)continue;
   {std::lock_guard<std::mutex> l(mutex);if(seen!=epoch)continue;if(count){auto c=run->Offer(queue[head]);if(c==ControlCode::OK){head=(head+1)%queue.size();--count;}}}
   const auto time=now();if(!run->Tick().active){uint64_t delta=last?time-last:1;last=time;++frame;auto code=run->Advance(delta,u64(frame),u64(frame),requestedAngle,capacity);if(code!=ControlCode::OK){failed=true;publish("{\"state\":\"FAULT\",\"stage\":\"ADVANCE\"}");continue;}}
   auto code=run->Pump();if(run->Tick().fault){failed=true;publish("{\"state\":\"FAULT\",\"stage\":\"PRODUCT_COMPLETION\",\"code\":"+std::to_string(int(code))+"}");continue;}
   uint64_t accepted=gpu->Owner().AcceptedCount();if(accepted>lastAccepted){lastAccepted=accepted;const auto& a=run->Tick().latest;EGLint width=0,height=0;eglQuerySurface(display,surface,EGL_WIDTH,&width);eglQuerySurface(display,surface,EGL_HEIGHT,&height);
    ResidentSceneFrame f;f.run=u64(runId);f.context=f.surface=f.geometry=u64(seen);f.renderFrame=++render;f.width=width;f.height=height;f.boxWidth=float(width);f.boxHeight=float(height);
    struct Swap {App* app;uint64_t epoch;EGLDisplay d;EGLSurface s;};Swap args{this,seen,display,surface};
    auto swapped=[](void* ptr){auto& v=*static_cast<Swap*>(ptr);return v.epoch==v.app->epoch.load()&&eglSwapBuffers(v.d,v.s)==EGL_TRUE;};
    auto receipt=scene.Draw(*gpu,a.publication,a.publicationGeneration,f,swapped,&args);
    std::ostringstream out;out<<"{\"state\":\""<<(receipt.current?"CURRENT":"FAULT")<<"\",\"stage\":\""<<receipt.stage<<"\",\"run\":"<<runId<<",\"epoch\":"<<seen<<",\"sourceCommit\":"<<a.source.lo<<",\"physicsCommit\":"<<a.physics.lo<<",\"completionBytes\":1024,\"displayPresentVerified\":false,\"requestedToken\":\""<<receipt.requestedToken<<"\",\"renderedToken\":\""<<receipt.renderedToken<<"\",\"rawGL\":"<<receipt.rawGl<<",\"rawEGL\":"<<receipt.rawEgl<<"}";publish(out.str());if(!receipt.current)failed=true;
   }
  }retire();
 }
};
App& app(){static App a;return a;}
void created(OH_NativeXComponent*,void* w){app().invalidate(w,true);}
void changed(OH_NativeXComponent*,void* w){app().invalidate(w,true);}
void destroyed(OH_NativeXComponent*,void*){app().invalidate(nullptr,true);}
OH_NativeXComponent_Callback callbacks{created,changed,destroyed,nullptr};
napi_value text(napi_env e,const std::string& s){napi_value v;napi_create_string_utf8(e,s.c_str(),s.size(),&v);return v;}
napi_value status(napi_env e,napi_callback_info){auto& a=app();std::lock_guard<std::mutex> l(a.mutex);return text(e,a.status);}
napi_value active(napi_env e,napi_callback_info i){size_t n=1;napi_value v;bool value=false;napi_get_cb_info(e,i,&n,&v,nullptr,nullptr);if(n!=1||napi_get_value_bool(e,v,&value)!=napi_ok)return text(e,"INVALID_ACTIVE");auto& a=app();{std::lock_guard<std::mutex> l(a.mutex);a.active=value;}a.invalidate(nullptr,false);return text(e,"NEW_RUN_REQUESTED");}
napi_value angle(napi_env e,napi_callback_info i){size_t n=1;napi_value v;double value=0;napi_get_cb_info(e,i,&n,&v,nullptr,nullptr);if(n!=1||napi_get_value_double(e,v,&value)!=napi_ok||!std::isfinite(value)||value<-7680||value>7680||std::floor(value)!=value)return text(e,"INVALID_ANGLE");auto& a=app();std::lock_guard<std::mutex> l(a.mutex);a.angle=int(value);return text(e,"ANGLE_ENQUEUED");}
napi_value offer(napi_env e,napi_callback_info i){size_t n=1;napi_value v;napi_get_cb_info(e,i,&n,&v,nullptr,nullptr);napi_typedarray_type type;size_t length=0,offset;void* data=nullptr;napi_value buffer;if(n!=1||napi_get_typedarray_info(e,v,&type,&length,&data,&buffer,&offset)!=napi_ok||type!=napi_uint32_array||length!=32)return text(e,"INVALID_COMMAND");auto& a=app();std::lock_guard<std::mutex> l(a.mutex);if(!a.active||!a.window)return text(e,"NOT_READY");if(a.count==a.queue.size())return text(e,"BACKPRESSURE");std::memcpy(a.queue[(a.head+a.count)%a.queue.size()].data(),data,128);++a.count;a.wake.notify_one();return text(e,"ENQUEUED_NOT_ACK");}
napi_value init(napi_env e,napi_value out){napi_property_descriptor d[]={{"status",nullptr,status,nullptr,nullptr,nullptr,napi_default,nullptr},{"setActive",nullptr,active,nullptr,nullptr,nullptr,napi_default,nullptr},{"setAngle",nullptr,angle,nullptr,nullptr,nullptr,napi_default,nullptr},{"offer",nullptr,offer,nullptr,nullptr,nullptr,napi_default,nullptr}};napi_define_properties(e,out,4,d);bool has=false;napi_has_named_property(e,out,OH_NATIVE_XCOMPONENT_OBJ,&has);if(has){napi_value v;OH_NativeXComponent* c=nullptr;napi_get_named_property(e,out,OH_NATIVE_XCOMPONENT_OBJ,&v);napi_unwrap(e,v,reinterpret_cast<void**>(&c));if(c)OH_NativeXComponent_RegisterCallback(c,&callbacks);}return out;}
napi_module module={1,0,nullptr,init,"grsm0",nullptr,{0}};
}
extern "C" __attribute__((constructor)) void RegisterGrsM0(){napi_module_register(&module);}
