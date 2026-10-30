#define NOMINMAX
#include <windows.h>
#include <psapi.h>
#pragma comment(lib,"psapi.lib")
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
extern "C" void GL_APIENTRY glUniform1f(GLint l,GLfloat v){auto f=reinterpret_cast<PFNGLUNIFORM1FPROC>(eglGetProcAddress("glUniform1f"));assert(f);f(l,v);}
extern "C" void GL_APIENTRY glUniform4fv(GLint l,GLsizei n,const GLfloat* v){auto f=reinterpret_cast<PFNGLUNIFORM4FVPROC>(eglGetProcAddress("glUniform4fv"));assert(f);f(l,n,v);}
static uint64_t testNow=1000000,lastAccepted=0,epochBefore=0,extra=0;static uint32_t loops=0,phase=0,passed=0,pauseTicks=0;static std::string beforeToken;static std::array<uint32_t,32> packet{};
int main(){App a(false);a.window=reinterpret_cast<void*>(1);a.ui.SetMode(false);a.hostNow=[](){return testNow;};a.hostHook=[](App& a,grs::BridgeStage stage){
 if(stage!=grs::BridgeStage::Wake)return;std::this_thread::sleep_for(std::chrono::milliseconds(1));assert(++loops<180000);if(a.status.find("FAULT")!=std::string::npos&&!a.requiresReset){std::cerr<<a.status<<std::endl;assert(false);}
 if(a.accepted>lastAccepted){std::cerr<<"phase="<<phase<<" accepted="<<a.accepted<<" source="<<a.sourceCommit<<" sand="<<a.ui.sand.lo<<" water="<<a.ui.water.lo<<" receipt="<<a.ui.receipt<<" reason="<<(a.completionCount?a.completions[a.completionCount-1][64]:0)<<std::endl;testNow+=8400000;lastAccepted=a.accepted;}else testNow++;
 if(phase==0&&a.ready){std::lock_guard<std::mutex> l(a.mutex);a.SetSourceLocked(1);phase=1;}
 else if(phase==1&&a.ui.SourceReady(testNow)){
  auto id=a.identity;packet={};packet[0]=1129534023;packet[1]=2;packet[2]=1;packet[4]=id.run.lo;packet[5]=id.run.hi;packet[6]=packet[8]=packet[15]=packet[24]=packet[27]=1;packet[10]=uint32_t(a.ui.boundary);packet[11]=uint32_t(a.ui.boundary>>32);packet[12]=id.geometry.lo;packet[13]=id.geometry.hi;packet[14]=2;packet[28]=id.config.lo;packet[29]=id.config.hi;double x=64.5,y=.001,m=4;std::memcpy(packet.data()+16,&x,8);std::memcpy(packet.data()+18,&y,8);std::memcpy(packet.data()+20,&m,8);
  std::lock_guard<std::mutex> l(a.mutex);assert(std::string(a.OfferUiLocked(packet))=="ENQUEUED_NOT_ACK");passed++;phase=2;
 }else if(phase==2&&a.accepted==1){assert(a.ui.flight.lo==4*1048576u&&a.ui.settled.lo==0&&a.sourceCommit==1&&a.status.find("CURRENT")!=std::string::npos);passed++;std::lock_guard<std::mutex> l(a.mutex);a.SetSourceLocked(-1);assert(std::string(a.OfferUiLocked(packet))=="SOURCE_CLOSED_OR_STALE");passed++;phase=3;
 }else if(phase==3&&a.accepted==2){assert(a.ui.flight.lo==0&&a.ui.settled.lo==4*1048576u&&a.sourceCommit==1);passed++;phase=4;
 }else if(phase==4&&a.accepted==3){assert(a.ui.water.lo==4*1048576u&&a.sourceCommit==1&&a.completions[a.completionCount-1][157]>0);passed++;std::lock_guard<std::mutex> l(a.mutex);a.SetSourceLocked(0);phase=20;
 }else if(phase==20&&a.ui.SourceReady(testNow)){packet[2]=0;packet[6]=packet[15]=packet[24]=2;packet[10]=uint32_t(a.ui.boundary);packet[11]=uint32_t(a.ui.boundary>>32);packet[14]=1;double m=.125;std::memcpy(packet.data()+20,&m,8);std::lock_guard<std::mutex> l(a.mutex);assert(std::string(a.OfferUiLocked(packet))=="ENQUEUED_NOT_ACK");passed++;phase=21;
 }else if(phase==21&&a.sourceCommit==2){assert(a.ui.water.lo==4*1048576u&&a.ui.sand.lo==131072&&a.sourceCommit==2);extra=a.accepted-4;passed++;std::lock_guard<std::mutex> l(a.mutex);a.SetSourceLocked(-1);phase=22;
 }else if(phase==22&&a.accepted==5+extra){assert(a.completions[a.completionCount-1][88]==0&&a.ui.sand.lo==131072&&a.ui.water.lo==4*1048576u);passed++;assert(a.ui.bufferBytes>0&&a.ui.frameBytes==8*256*256&&a.ui.knownGpuPeak>=a.ui.bufferBytes+a.ui.frameBytes);passed++;epochBefore=a.epoch;beforeToken=a.status;a.setActive(false);testNow+=60000000000ull;assert(a.epoch==epochBefore);passed++;phase=5;
 }else if(phase==5&&std::string(a.phase)=="BACKGROUND"){assert(!a.ready&&a.accepted==5+extra&&a.ui.water.lo==4*1048576u&&a.status.find("STALE")!=std::string::npos);if(++pauseTicks==3){passed++;a.setActive(true);phase=6;}
 }else if(phase==6&&a.ready){assert(a.epoch==epochBefore&&a.identity.run.lo==1&&a.sourceCommit==2&&a.ui.water.lo==4*1048576u);passed++;testNow+=8400000;phase=7;
 }else if(phase==7&&a.accepted==6+extra){assert(a.sourceCommit==2&&a.ui.water.lo==4*1048576u);passed++;a.ui.SetMode(true);assert(a.ui.Sample(60,.8,1,testNow));testNow+=300000000;phase=8;
 }else if(phase==8&&std::string(a.phase)=="GRAVITY_STALE"){assert(a.accepted==6+extra&&!a.ready&&a.ui.water.lo==4*1048576u&&!a.ui.SourceReady(testNow));passed++;assert(a.ui.Sample(-60,.3,2,testNow));phase=9;
 }else if(phase==9&&a.ready){assert(a.ui.source==-1);passed++;testNow+=8400000;phase=10;
 }else if(phase==10&&a.accepted==7+extra){assert(a.sourceCommit==2&&a.ui.water.lo==4*1048576u&&a.ui.acceptedAngle>=-7680&&a.ui.acceptedAngle<=7680);passed++;a.invalidate(reinterpret_cast<void*>(1),true);assert(a.requiresReset&&!a.ready&&a.status.find("OWNER_STATE_LOST")!=std::string::npos);passed++;phase=14;
 }else if(phase==14&&a.requiresReset){assert(!a.ready);a.ui.SetMode(false);a.optics.values[0]=.05f;a.invalidate(nullptr,false);lastAccepted=0;phase=11;
 }else if(phase==11&&a.ready&&a.identity.run.lo==2){assert(a.epoch>epochBefore&&a.sourceCommit==0&&a.ui.water.lo==0&&a.ui.admitted==0);passed++;testNow+=8400000;phase=12;
 }else if(phase==12&&a.accepted==1){assert(a.ui.water.lo==0&&a.sourceCommit==0&&a.status.find("CURRENT")!=std::string::npos&&a.status!=beforeToken);passed++;a.quit=true;phase=13;}
 };a.loop();assert(phase==13);PROCESS_MEMORY_COUNTERS_EX memory{};assert(GetProcessMemoryInfo(GetCurrentProcess(),reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&memory),sizeof(memory)));passed++;std::cout<<"{\"revision\":\"T228_PRODUCTION_UI_APP_GLES_V1\",\"passed\":"<<passed<<",\"failed\":0,\"gpuBufferAllocatedBytes\":"<<a.ui.bufferBytes<<",\"frameTargetBytes\":"<<a.ui.frameBytes<<",\"knownGpuBytesPeak\":"<<a.ui.knownGpuPeak<<",\"pcWorkingSetBytes\":"<<memory.WorkingSetSize<<",\"pcPeakWorkingSetBytes\":"<<memory.PeakWorkingSetSize<<",\"pcPrivateUsageBytes\":"<<memory.PrivateUsage<<",\"execution\":\"PC_ANGLE_SWIFTSHADER\",\"clock\":\"SCRIPTED_OFFLINE\",\"driver\":\"REAL_FULL_M1_APP_BRIDGE_RUN_SCENE\",\"device\":\"NOT_RUN\"}\n";return 0;}
