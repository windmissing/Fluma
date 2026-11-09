#include <EGL/egl.h>
#include <iostream>
#include <GLES3/gl31.h>
#include <cassert>
EGLDisplay hostDisplay(EGLNativeDisplayType){using Fn=EGLDisplay(EGLAPIENTRYP)(EGLenum,void*,const EGLint*);auto fn=reinterpret_cast<Fn>(eglGetProcAddress("eglGetPlatformDisplayEXT"));EGLint args[]={0x3203,0x3450,0x3209,0x3487,EGL_NONE};return fn(0x3202,nullptr,args);}
EGLSurface hostWindow(EGLDisplay d,EGLConfig cfg,EGLNativeWindowType,const EGLint*){EGLint attrs[]={EGL_WIDTH,256,EGL_HEIGHT,256,EGL_NONE};return eglCreatePbufferSurface(d,cfg,attrs);}
#define eglGetDisplay hostDisplay
#define eglCreateWindowSurface hostWindow
#undef EGL_WINDOW_BIT
#define EGL_WINDOW_BIT EGL_PBUFFER_BIT
#define GRS_APP_M1
#define GRS_BRIDGE_HOST_TEST
#define GRS_BRIDGE_HOST_GL
#include "app_bridge.cpp"
#include <iostream>
extern "C" void GL_APIENTRY glFinish(){auto f=reinterpret_cast<PFNGLFINISHPROC>(eglGetProcAddress("glFinish"));assert(f);f();}
extern "C" void GL_APIENTRY glCopyBufferSubData(GLenum a,GLenum b,GLintptr c,GLintptr d,GLsizeiptr n){auto f=reinterpret_cast<PFNGLCOPYBUFFERSUBDATAPROC>(eglGetProcAddress("glCopyBufferSubData"));assert(f);f(a,b,c,d,n);}
extern "C" void GL_APIENTRY glUniform1f(GLint location,GLfloat v){auto f=reinterpret_cast<PFNGLUNIFORM1FPROC>(eglGetProcAddress("glUniform1f"));assert(f);f(location,v);}
extern "C" void GL_APIENTRY glUniform4fv(GLint location,GLsizei count,const GLfloat* v){auto f=reinterpret_cast<PFNGLUNIFORM4FVPROC>(eglGetProcAddress("glUniform4fv"));assert(f);f(location,count,v);}

namespace {
uint64_t stamp=1000000,lastAccepted=0;
unsigned phase=0,sequence=0;
uint32_t admittedWater=0;
U64 originalRun{};
std::chrono::steady_clock::time_point begin;

void Offer(App& a,int kind,double x,double y){
 std::array<uint32_t,32> w{};
 w[0]=1129534023;w[1]=2;w[2]=kind;
 w[4]=a.identity.run.lo;w[5]=a.identity.run.hi;w[6]=++sequence;
 w[8]=a.input.lo;w[9]=a.input.hi;w[10]=uint32_t(a.ui.boundary);w[11]=uint32_t(a.ui.boundary>>32);
 w[12]=a.identity.geometry.lo;w[13]=a.identity.geometry.hi;
 w[14]=kind+1;w[15]=w[24]=sequence;w[26]=w[27]=1;
 w[28]=a.identity.config.lo;w[29]=a.identity.config.hi;
 double mass=.125;
 std::memcpy(w.data()+16,&x,8);std::memcpy(w.data()+18,&y,8);std::memcpy(w.data()+20,&mass,8);
 assert(std::string(a.OfferUiLocked(w))=="ENQUEUED_NOT_ACK");
 stamp+=8400000;
}

void Wake(App& a,BridgeStage stage){
 if(stage!=BridgeStage::Wake)return;
 std::this_thread::sleep_for(std::chrono::milliseconds(1));
 bool reset=false;
 {
  std::lock_guard<std::mutex> lock(a.mutex);
  if(std::chrono::steady_clock::now()-begin>std::chrono::seconds(150)){
   std::cerr<<"timeout phase="<<phase<<" src="<<a.sourceCommit<<" physics="<<a.physicsCommit<<" status="<<a.status<<std::endl;
   a.quit=true;return;
  }
  if(a.accepted>lastAccepted)stamp+=8400000;else ++stamp;
  lastAccepted=a.accepted;
  if(!a.AllowedLocked())return;
  switch(phase){
   case 0: originalRun=a.identity.run;assert(a.SetSourceLocked(0));++phase;break;
   case 1: if(a.ui.SourceReady(stamp)){Offer(a,0,32.5,0);++phase;}break;
   case 2: if(a.sourceCommit==1){assert(a.ui.sand.lo==131072);assert(a.SetSourceLocked(-1));assert(a.SetSourceLocked(1));++phase;}break;
   case 3: if(a.ui.SourceReady(stamp)){Offer(a,1,64.5,.125);++phase;}break;
   case 4: if(a.sourceCommit==2){
    admittedWater=a.ui.flight.lo;assert(admittedWater==131072&&a.ui.settled.lo==0);
    assert(a.SetSourceLocked(-1));++phase;
   }break;
   case 5: if(a.ui.settled.lo==admittedWater&&a.ui.flight.lo==0){
    assert(a.sourceCommit==2&&a.ui.source==-1&&a.ui.water.lo==admittedWater);
    assert(Compare64(a.identity.run,originalRun)==0);++phase;reset=true;
   }break;
   case 6:
    assert(Compare64(a.identity.run,originalRun)!=0&&a.sourceCommit==0);
    assert(a.ui.sand.lo==0&&a.ui.water.lo==0&&a.ui.flight.lo==0);++phase;a.quit=true;break;
  }
 }
 if(reset)a.invalidate(nullptr,false);
}
}
int main(){
 auto app=std::make_unique<App>(false);
 app->window=reinterpret_cast<void*>(1);
 app->hostNow=[](){return stamp;};app->hostHook=Wake;
 assert(app->ui.SetMode(false));
 begin=std::chrono::steady_clock::now();app->loop();assert(phase==7);
 std::cout<<"{\"schema\":\"T438_DEVELOPER_NATIVE_GL_V1\",\"passed\":1,\"failed\":0,\"admittedJQ\":"<<admittedWater
  <<",\"settledAfterUpQ\":"<<admittedWater<<",\"device\":\"NOT_RUN\",\"capabilities\":"
  <<grs::m1::ExperienceCapabilities(app->K,app->Kw)<<"}\n";
}
