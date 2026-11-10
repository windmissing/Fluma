#include "m2/capability.h"
#include "m2/buffer_plan.h"
#include <cassert>
#include <iostream>
using namespace grs::m2;
#include "merge_probe.inc"
static GLenum AllocateProfile(const Profile& p){
 const auto bytes=ResidentBufferBytes(p);
 std::array<GLuint,bytes.size()+1> buffers{};
 glGenBuffers(GLsizei(buffers.size()),buffers.data());
 GLenum error=glGetError();
 for(size_t i=0;i<buffers.size()&&error==GL_NO_ERROR;++i){
  glBindBuffer(GL_SHADER_STORAGE_BUFFER,buffers[i]);
  glBufferData(GL_SHADER_STORAGE_BUFFER,i<bytes.size()?bytes[i]:p.coldTransientBytes,nullptr,GL_DYNAMIC_COPY);
  error=glGetError();
 }
 glBindBuffer(GL_SHADER_STORAGE_BUFFER,0);
 glDeleteBuffers(GLsizei(buffers.size()),buffers.data());
 auto releaseError=glGetError();
 return error?error:releaseError;
}
int main(){
 assert(!SelectProfile(192,64,2,3,1));
 assert(!SelectProfile(192,64,3,7,2));
 Limits limits;
 assert(ProbeCurrent(&kProfiles[0],limits)==ProbeCode::NO_CONTEXT);
 using Platform=EGLDisplay(EGLAPIENTRYP)(EGLenum,void*,const EGLint*);
 auto platform=reinterpret_cast<Platform>(eglGetProcAddress("eglGetPlatformDisplayEXT"));assert(platform);
 EGLint args[]={0x3203,0x3450,0x3209,0x3487,EGL_NONE};
 EGLDisplay display=platform(0x3202,nullptr,args);assert(eglInitialize(display,nullptr,nullptr));
 const EGLint attrs[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_NONE};
 EGLConfig cfg;EGLint count;assert(eglChooseConfig(display,attrs,&cfg,1,&count)&&count);
 assert(eglBindAPI(EGL_OPENGL_ES_API));
 const EGLint ctxAttr[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_CONTEXT_MINOR_VERSION,1,EGL_NONE};
 const EGLint surfAttr[]={EGL_WIDTH,1,EGL_HEIGHT,1,EGL_NONE};
 EGLContext context=eglCreateContext(display,cfg,EGL_NO_CONTEXT,ctxAttr);
 EGLSurface surface=eglCreatePbufferSurface(display,cfg,surfAttr);
 assert(eglMakeCurrent(display,surface,surface,context));
 std::cout<<"{\"schema\":\"T452_M2_LIMITS_PROBE_V1\",\"device\":\"NOT_RUN\",\"profiles\":[";
 unsigned passed=0;
 for(const auto& p:kProfiles){
  auto code=ProbeCurrent(SelectProfile(p.k,p.kw,3,7,1),limits);
  const GLenum allocation=code==ProbeCode::OK?AllocateProfile(p):GL_INVALID_OPERATION;
  if(code==ProbeCode::OK&&allocation==GL_NO_ERROR)++passed;
  if(&p!=&kProfiles[0])std::cout<<',';
  std::cout<<"{\"K\":"<<p.k<<",\"Kw\":"<<p.kw<<",\"code\":"<<int(code)<<",\"allocationGlError\":"<<allocation<<",\"residentBytes\":"<<p.payloadBytes<<",\"requiredMaxObject\":"<<p.coldTransientBytes
   <<",\"maxSsboBytes\":"<<limits.blockBytes<<",\"sharedBytes\":"<<limits.sharedBytes
   <<",\"alignment\":"<<limits.alignment<<",\"bindings\":"<<limits.bindings<<'}';
 }
 const auto merge=MergeProbe();const auto index=IndexProbe();
 std::cout<<"],\"mergeCases\":"<<merge<<",\"indexCases\":"<<index<<",\"passed\":"<<passed<<",\"failed\":"<<4-passed<<"}\n";
 eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);
 eglDestroySurface(display,surface);eglDestroyContext(display,context);eglTerminate(display);
 return passed==4?0:1;
}
