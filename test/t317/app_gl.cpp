#include <EGL/egl.h>
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
#include "../../entry/src/main/cpp/gpu_resident/app_bridge.cpp"
#include <iostream>
extern "C" void GL_APIENTRY glFinish(){auto f=reinterpret_cast<PFNGLFINISHPROC>(eglGetProcAddress("glFinish"));assert(f);f();}
extern "C" void GL_APIENTRY glCopyBufferSubData(GLenum a,GLenum b,GLintptr c,GLintptr d,GLsizeiptr n){auto f=reinterpret_cast<PFNGLCOPYBUFFERSUBDATAPROC>(eglGetProcAddress("glCopyBufferSubData"));assert(f);f(a,b,c,d,n);}
extern "C" void GL_APIENTRY glUniform1f(GLint location,GLfloat v){auto f=reinterpret_cast<PFNGLUNIFORM1FPROC>(eglGetProcAddress("glUniform1f"));assert(f);f(location,v);}
extern "C" void GL_APIENTRY glUniform4fv(GLint location,GLsizei count,const GLfloat* v){auto f=reinterpret_cast<PFNGLUNIFORM4FVPROC>(eglGetProcAddress("glUniform4fv"));assert(f);f(location,count,v);}
static uint64_t testNow=1000000,lastAccepted=0;static bool offered=false;static uint32_t loops=0;
int main(){App a(false);a.window=reinterpret_cast<void*>(1);a.hostNow=[](){return testNow;};a.hostHook=[](App& a,grs::BridgeStage stage){if(stage!=grs::BridgeStage::Wake)return;std::this_thread::sleep_for(std::chrono::milliseconds(1));if(++loops>180000){a.quit=true;return;}if(a.accepted>lastAccepted){testNow+=8400000;lastAccepted=a.accepted;}else testNow++;if(a.ready&&!offered){auto id=a.identity;std::array<uint32_t,32> w{};w[0]=1129534023;w[1]=2;w[2]=1;w[4]=id.run.lo;w[5]=id.run.hi;w[6]=w[8]=w[10]=w[14]=w[15]=w[24]=w[27]=1;w[12]=id.geometry.lo;w[13]=id.geometry.hi;w[28]=id.config.lo;w[29]=id.config.hi;double x=64.5,y=0,m=64;std::memcpy(w.data()+16,&x,8);std::memcpy(w.data()+18,&y,8);std::memcpy(w.data()+20,&m,8);std::lock_guard<std::mutex> l(a.mutex);assert(std::string(a.OfferLocked(w))=="ENQUEUED_NOT_ACK");offered=true;}if(a.accepted>=2)a.quit=true;};a.loop();std::cout<<"accepted="<<a.accepted<<" source="<<a.sourceCommit<<" physics="<<a.physicsCommit<<" status="<<a.status<<std::endl;bool stages[40]{};for(uint32_t i=0;i<a.timeline.count;i++){auto stage=a.timeline.rows[i].stage;if(stage<40)stages[stage]=true;}for(int stage:{24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39})assert(stages[stage]);assert(offered&&a.accepted>=2&&a.sourceCommit==1&&a.physicsCommit>=2&&a.status.find("CURRENT")!=std::string::npos);std::cout<<"{\"revision\":\"T317_PRODUCTION_APP_GLES_V1\",\"passed\":6,\"failed\":0,\"window\":\"PC_PBUFFER\",\"clock\":\"SCRIPTED_OFFLINE\",\"device\":\"NOT_RUN\"}\n";}
