#include "pair_executor_v1.h"
#include <EGL/egl.h>
#include <GLES3/gl31.h>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <fstream>
#include <limits>
#include <sstream>
#include <thread>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#undef WAIT_FAILED
#endif

namespace p011gpu {
using Clock=std::chrono::steady_clock;
static double elapsed(Clock::time_point t){return std::chrono::duration<double,std::milli>(Clock::now()-t).count();}
struct ContextSlot::Opaque {
 EGLDisplay display=EGL_NO_DISPLAY;EGLContext context=EGL_NO_CONTEXT;EGLSurface surface=EGL_NO_SURFACE;
 EGLConfig config=nullptr;GLuint programs[3]={};GLuint buffers[2][6]={{0}};
 std::thread::id thread;uint32_t slotState[2]={0,0}; // 0 FREE,1 PREPARED,2 IN_FLIGHT,3 COMPLETED,4 QUARANTINED
 PFNGLMEMORYBARRIERPROC barrier=nullptr;PFNGLBUFFERSUBDATAPROC subData=nullptr;
 PFNGLGETINTEGER64VPROC get64=nullptr;PFNGLUNIFORM1UIPROC uniform1ui=nullptr;
};
static void* getProc(const char* name){
#ifdef _WIN32
 HMODULE egl=GetModuleHandleA("libEGL.dll");if(!egl)egl=LoadLibraryA("libEGL.dll");if(!egl)return nullptr;
 using Proc=void*(__stdcall*)(const char*);auto fn=(Proc)GetProcAddress(egl,"eglGetProcAddress");return fn?fn(name):nullptr;
#else
 return reinterpret_cast<void*>(eglGetProcAddress(name));
#endif
}
static std::string readFile(const std::string& p){std::ifstream f(p,std::ios::binary);return f?std::string((std::istreambuf_iterator<char>(f)),{}):"";}
static bool program(const std::string& path,GLuint& out,RequestDiagnostics& d){std::string code=readFile(path);
 if(code.empty()){d.stage="SHADER_SOURCE";d.detail=path;return false;}
 const char* src=code.c_str();GLuint shader=glCreateShader(GL_COMPUTE_SHADER);
 glShaderSource(shader,1,&src,nullptr);glCompileShader(shader);GLint ok=0;glGetShaderiv(shader,GL_COMPILE_STATUS,&ok);
 if(!ok){char log[2048]={};glGetShaderInfoLog(shader,2048,nullptr,log);d.stage="SHADER_COMPILE";d.detail=log;
  glDeleteShader(shader);return false;}
 out=glCreateProgram();glAttachShader(out,shader);glLinkProgram(out);glGetProgramiv(out,GL_LINK_STATUS,&ok);
 glDeleteShader(shader);if(!ok){char log[2048]={};glGetProgramInfoLog(out,2048,nullptr,log);
  d.stage="PROGRAM_LINK";d.detail=log;return false;}
 d.rawGlError=glGetError();if(d.rawGlError){d.stage="PROGRAM_GL";return false;}return true;
}
static uint32_t pow2(uint32_t x){uint32_t k=1;while(k<x&&k<=0x40000000u)k*=2;return k;}
ContextSlot::ContextSlot():impl_(new Opaque()){}
ContextSlot::~ContextSlot(){reset();delete impl_;}
void ContextSlot::reset(){Opaque& o=*impl_;
 if(o.display!=EGL_NO_DISPLAY){if(o.context!=EGL_NO_CONTEXT&&o.surface!=EGL_NO_SURFACE){
   eglMakeCurrent(o.display,o.surface,o.surface,o.context);
   for(auto& slot:o.buffers)for(GLuint& b:slot)if(b){glDeleteBuffers(1,&b);b=0;}
   for(GLuint& p:o.programs)if(p){glDeleteProgram(p);p=0;}
  }
  eglMakeCurrent(o.display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);
  if(o.surface!=EGL_NO_SURFACE)eglDestroySurface(o.display,o.surface);
  if(o.context!=EGL_NO_CONTEXT)eglDestroyContext(o.display,o.context);
  eglTerminate(o.display);
 }
 o=Opaque();initialized_=false;quarantined_=false;owner_=0;liveBuffers_=0;nmax_=K_=h_=0;
}
void ContextSlot::cancelOwner(uint64_t generation){if(owner_==generation){quarantined_=true;reset();}}
ExecStatus ContextSlot::initialize(uint32_t nmax,const std::string& shaderDirectory,RequestDiagnostics& d){
 reset();d=RequestDiagnostics();if(!littleEndian()||nmax<2||nmax>4096)return ExecStatus::CAPACITY_UNSUPPORTED;
 Opaque& o=*impl_;o.thread=std::this_thread::get_id();o.display=eglGetDisplay(EGL_DEFAULT_DISPLAY);
 if(o.display==EGL_NO_DISPLAY||!eglInitialize(o.display,nullptr,nullptr)){d.stage="EGL_DISPLAY";return ExecStatus::ENV_UNSUPPORTED;}
 EGLint attrs[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_NONE};EGLint count=0;
 if(!eglChooseConfig(o.display,attrs,&o.config,1,&count)||count<1){d.stage="EGL_CONFIG";return ExecStatus::ENV_UNSUPPORTED;}
 eglBindAPI(EGL_OPENGL_ES_API);EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE};
 o.context=eglCreateContext(o.display,o.config,EGL_NO_CONTEXT,ca);
 EGLint sa[]={EGL_WIDTH,16,EGL_HEIGHT,16,EGL_NONE};o.surface=eglCreatePbufferSurface(o.display,o.config,sa);
 if(o.context==EGL_NO_CONTEXT||o.surface==EGL_NO_SURFACE||
  !eglMakeCurrent(o.display,o.surface,o.surface,o.context)){d.stage="EGL_CONTEXT";return ExecStatus::ENV_UNSUPPORTED;}
 GLint major=0,minor=0;glGetIntegerv(GL_MAJOR_VERSION,&major);glGetIntegerv(GL_MINOR_VERSION,&minor);
 if(major<3||(major==3&&minor<1)){d.stage="GLES_31_REQUIRED";return ExecStatus::ENV_UNSUPPORTED;}
 o.barrier=(PFNGLMEMORYBARRIERPROC)getProc("glMemoryBarrier");
 o.subData=(PFNGLBUFFERSUBDATAPROC)getProc("glBufferSubData");
 o.get64=(PFNGLGETINTEGER64VPROC)getProc("glGetInteger64v");
 o.uniform1ui=(PFNGLUNIFORM1UIPROC)getProc("glUniform1ui");
 if(!o.barrier||!o.subData||!o.get64||!o.uniform1ui){d.stage="ENTRYPOINT";return ExecStatus::ENV_UNSUPPORTED;}
 GLint64 maxBlock=0;o.get64(GL_MAX_SHADER_STORAGE_BLOCK_SIZE,&maxBlock);
 GLint bindings=0,blocks=0,invocations=0,groupX=0,shared=0;
 glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS,&bindings);
 glGetIntegerv(GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS,&blocks);
 glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS,&invocations);
 glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE,0,&groupX);
 glGetIntegerv(GL_MAX_COMPUTE_SHARED_MEMORY_SIZE,&shared);d.rawGlError=glGetError();
 K_=pow2(nmax);for(h_=0;(1u<<h_)<K_;h_++);
 uint64_t tableBytes=uint64_t(K_)*(h_+1)*sizeof(Leaf),queryBytes=uint64_t(K_)*sizeof(Query);
 uint64_t candidateBytes=uint64_t(K_)*sizeof(Candidate);
 if(d.rawGlError||bindings<6||blocks<4||invocations<64||groupX<64||shared<2048||
  maxBlock<0||tableBytes>uint64_t(maxBlock)||queryBytes>uint64_t(maxBlock)||
  candidateBytes>uint64_t(maxBlock)||tableBytes>uint64_t(std::numeric_limits<GLsizeiptr>::max())||
  K_>UINT32_MAX/(h_+1)){d.stage="LIMITS";return ExecStatus::CAPACITY_UNSUPPORTED;}
 const char* names[3]={"pair_build_v1.comp","pair_query_v1.comp","pair_reduce_v1.comp"};
 for(int i=0;i<3;i++)if(!program(shaderDirectory+"/"+names[i],o.programs[i],d))return ExecStatus::GL_ERROR;
 const uint64_t sizes[6]={256,tableBytes,queryBytes,candidateBytes,candidateBytes,128};
 for(int slot=0;slot<2;slot++)for(int i=0;i<6;i++){
  glGenBuffers(1,&o.buffers[slot][i]);if(!o.buffers[slot][i]){d.stage="BUFFER_GEN";return ExecStatus::GL_ERROR;}
  liveBuffers_++;peakBuffers_=std::max(peakBuffers_,liveBuffers_);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER,o.buffers[slot][i]);glBufferData(GL_SHADER_STORAGE_BUFFER,GLsizeiptr(sizes[i]),nullptr,GL_DYNAMIC_DRAW);
  d.rawGlError=glGetError();if(d.rawGlError){d.stage="BUFFER_ALLOC";return ExecStatus::GL_ERROR;}
 }
 nmax_=nmax;initialized_=true;epoch_++;d.contextEpoch=uint32_t(epoch_);d.valid=true;return ExecStatus::OK;
}
static void shortcut(const PreparedView& p,Result& r){r=Result();auto& w=r.w;const auto& h=p.header->w;
 w[0]=kResultMagic;w[1]=1;w[2]=kStatusNoPair;w[10]=0;
 for(int i=0;i<4;i++)w[12+i]=h[28+i];for(int i=0;i<2;i++){
  w[16+i]=h[14+i];w[18+i]=h[16+i];w[20+i]=h[18+i];}
 for(int i=0;i<8;i++)w[22+i]=h[48+i];w[30]=kDone;
}
ExecStatus ContextSlot::executeIntegerPair(const PreparedView& p,uint32_t slotIndex,Deadline deadline,
 Result& out,RequestDiagnostics& d){
 d=RequestDiagnostics();out=Result();auto begin=Clock::now();Opaque& o=*impl_;
 if(!initialized_||quarantined_||o.thread!=std::this_thread::get_id()||slotIndex>1||
  p.identity.slotIndex!=slotIndex||p.identity.contextEpoch!=epoch_||p.K!=K_||
  p.header==nullptr||p.leaves==nullptr||p.queries==nullptr||o.slotState[slotIndex]!=0){d.stage="IDENTITY";return ExecStatus::STALE;}
 if(owner_!=0&&owner_!=p.identity.ownerGeneration){d.stage="OWNER";return ExecStatus::CANCELLED;}
 owner_=p.identity.ownerGeneration;
 if(p.n<2){shortcut(p,out);d.stage="CPU_EMPTY_SHORTCUT";d.valid=true;d.requestCpuMs=elapsed(begin);return ExecStatus::OK;}
 GLenum pre=glGetError();if(pre!=GL_NO_ERROR){d.stage="PREEXISTING_GL";d.rawGlError=pre;return ExecStatus::GL_ERROR;}
 o.slotState[slotIndex]=1;GLuint* b=o.buffers[slotIndex];auto fail=[&](ExecStatus s,const char* stage){
  d.stage=stage;GLenum latest=glGetError();if(d.rawGlError==0)d.rawGlError=latest;
  o.slotState[slotIndex]=4;quarantined_=true;return s;};
 auto uploadStart=Clock::now();const uint64_t tableOffset=0,queryBytes=uint64_t(K_)*sizeof(Query);
 const void* payloads[4]={p.header,p.leaves,p.queries,&out};const size_t bytes[4]={256,size_t(K_)*16,size_t(queryBytes),128};
 const int indices[4]={0,1,2,5};for(int i=0;i<4;i++){
  glBindBuffer(GL_SHADER_STORAGE_BUFFER,b[indices[i]]);o.subData(GL_SHADER_STORAGE_BUFFER,GLintptr(tableOffset),GLsizeiptr(bytes[i]),payloads[i]);
  GLenum err=glGetError();if(err){d.rawGlError=err;d.stage="UPLOAD";o.slotState[slotIndex]=4;quarantined_=true;return ExecStatus::GL_ERROR;}}
 d.uploadCpuMs=elapsed(uploadStart);
 for(int i=0;i<6;i++)glBindBufferBase(GL_SHADER_STORAGE_BUFFER,i,b[i]);
 if(glGetError()!=GL_NO_ERROR)return fail(ExecStatus::GL_ERROR,"BIND_BLOCKS");
 o.slotState[slotIndex]=2;auto dispatchStart=Clock::now();uint32_t groups=(K_+63)/64;
 glUseProgram(o.programs[0]);GLint levelLoc=glGetUniformLocation(o.programs[0],"uLevel");
 for(uint32_t level=1;level<=h_;level++){
  o.uniform1ui(levelLoc,level);glDispatchCompute(groups,1,1);d.dispatchCount++;
  GLenum e=glGetError();if(e){d.rawGlError=e;return fail(ExecStatus::GL_ERROR,"BUILD_DISPATCH");}
  o.barrier(GL_SHADER_STORAGE_BARRIER_BIT);e=glGetError();if(e){d.rawGlError=e;return fail(ExecStatus::GL_ERROR,"BUILD_BARRIER");}
 }
 glUseProgram(o.programs[1]);glDispatchCompute(groups,1,1);d.dispatchCount++;
 if(glGetError()!=GL_NO_ERROR)return fail(ExecStatus::GL_ERROR,"QUERY_DISPATCH");
 o.barrier(GL_SHADER_STORAGE_BARRIER_BIT);
 if(glGetError()!=GL_NO_ERROR)return fail(ExecStatus::GL_ERROR,"QUERY_BARRIER");
 uint32_t count=K_;glUseProgram(o.programs[2]);
 GLint countLoc=glGetUniformLocation(o.programs[2],"uCount");
 GLint dispatchLoc=glGetUniformLocation(o.programs[2],"uDispatchCount");
 GLint finalLoc=glGetUniformLocation(o.programs[2],"uFinal");
 GLuint input=b[3],output=b[4];
 do{uint32_t next=(count+63)/64;bool final=next==1;
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER,3,input);glBindBufferBase(GL_SHADER_STORAGE_BUFFER,4,output);
  o.uniform1ui(countLoc,count);o.uniform1ui(finalLoc,final?1u:0u);
  o.uniform1ui(dispatchLoc,d.dispatchCount+1);glDispatchCompute(next,1,1);d.dispatchCount++;
  if(glGetError()!=GL_NO_ERROR)return fail(ExecStatus::GL_ERROR,"REDUCE_DISPATCH");
  if(!final){o.barrier(GL_SHADER_STORAGE_BARRIER_BIT);if(glGetError()!=GL_NO_ERROR)
    return fail(ExecStatus::GL_ERROR,"REDUCE_BARRIER");std::swap(input,output);}
  count=next;
 }while(count>1);
 d.dispatchCpuMs=elapsed(dispatchStart);
 o.barrier(GL_BUFFER_UPDATE_BARRIER_BIT);if(glGetError()!=GL_NO_ERROR)return fail(ExecStatus::GL_ERROR,"READ_BARRIER");
 GLsync fence=glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE,0);
 if(!fence||glGetError()!=GL_NO_ERROR)return fail(ExecStatus::GL_ERROR,"FENCE_CREATE");
 auto waitStart=Clock::now();glFlush();
 while(true){GLenum w=glClientWaitSync(fence,GL_SYNC_FLUSH_COMMANDS_BIT,10000000ull);d.waitCalls++;
  d.waitReturn=w==GL_ALREADY_SIGNALED?"ALREADY_SIGNALED":w==GL_CONDITION_SATISFIED?"CONDITION_SATISFIED":
   w==GL_TIMEOUT_EXPIRED?"TIMEOUT_EXPIRED":"WAIT_FAILED";
  GLenum e=glGetError();if(e||w==GL_WAIT_FAILED){d.rawGlError=e;glDeleteSync(fence);return fail(ExecStatus::WAIT_FAILED,"WAIT");}
  if(w==GL_ALREADY_SIGNALED||w==GL_CONDITION_SATISFIED)break;
  if(Clock::now()>=deadline.at){d.fenceWaitCpuMs=elapsed(waitStart);glDeleteSync(fence);return fail(ExecStatus::TIMEOUT,"WAIT_TIMEOUT");}
 }
 d.fenceWaitCpuMs=elapsed(waitStart);glDeleteSync(fence);
 glBindBuffer(GL_SHADER_STORAGE_BUFFER,b[5]);auto mapStart=Clock::now();
 const void* data=glMapBufferRange(GL_SHADER_STORAGE_BUFFER,0,128,GL_MAP_READ_BIT);
 GLenum mapError=glGetError();if(!data||mapError){d.rawGlError=mapError;return fail(ExecStatus::MAP_FAILED,"MAP");}
 Result candidate;std::memcpy(&candidate,data,128);
 GLboolean unmapped=glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);GLenum unmapError=glGetError();
 d.mapCpuMs=elapsed(mapStart);if(!unmapped||unmapError){d.rawGlError=unmapError;return fail(ExecStatus::UNMAP_FAILED,"UNMAP");}
 o.slotState[slotIndex]=3;d.requestCpuMs=elapsed(begin);d.valid=true;out=candidate;
 o.slotState[slotIndex]=0;return ExecStatus::OK;
}
}
