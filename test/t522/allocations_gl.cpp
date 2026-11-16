#include <cstdint>
#include <EGL/egl.h>
#include <GLES3/gl31.h>
#include "../../entry/src/main/cpp/gpu_resident/m3a/buffer_plan.h"
#include <cassert>
#include <iostream>
#include "gl_api.inc"
int main(){
 using F=EGLDisplay(EGLAPIENTRYP)(EGLenum,void*,const EGLint*);PROC(F,eglGetPlatformDisplayEXT);
 const EGLint displayArgs[]={0x3203,0x3450,0x3209,0x3487,EGL_NONE};auto display=f(0x3202,nullptr,displayArgs);assert(eglInitialize(display,nullptr,nullptr));
 const EGLint attributes[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_NONE};EGLConfig config;EGLint count;assert(eglChooseConfig(display,attributes,&config,1,&count)&&count);
 const EGLint contextArgs[]={EGL_CONTEXT_CLIENT_VERSION,3,0x30FB,1,EGL_NONE},surfaceArgs[]={EGL_WIDTH,8,EGL_HEIGHT,8,EGL_NONE};auto context=eglCreateContext(display,config,EGL_NO_CONTEXT,contextArgs);auto surface=eglCreatePbufferSurface(display,config,surfaceArgs);assert(eglMakeCurrent(display,surface,surface,context));
 GLint alignment=0;glGetIntegerv(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT,&alignment);assert(alignment>0);
 for(const auto& p:grs::m3a::kProfiles){
  grs::m3a::ResidentBuffersV4 allocation;assert(grs::m3a::ResidentBuffersV4::Make(p,size_t(alignment),allocation));
  std::array<GLuint,28> buffers{};glGenBuffers(GLsizei(buffers.size()),buffers.data());
  for(size_t i=0;i<buffers.size();++i){glBindBuffer(GL_SHADER_STORAGE_BUFFER,buffers[i]);glBufferData(GL_SHADER_STORAGE_BUFFER,GLsizeiptr(allocation.bytes[i]),nullptr,GL_DYNAMIC_COPY);assert(glGetError()==GL_NO_ERROR);GLint64 bytes=0;glGetBufferParameteri64v(GL_SHADER_STORAGE_BUFFER,GL_BUFFER_SIZE,&bytes);assert(bytes==GLint64(allocation.bytes[i]));}
  std::array<GLuint,21> full{};glGenBuffers(21,full.data());
  for(size_t i=0;i<21;++i){const size_t expected=i==20?32768:size_t(p.scopeRows)*256;glBindBuffer(GL_SHADER_STORAGE_BUFFER,full[i]);glBufferData(GL_SHADER_STORAGE_BUFFER,GLsizeiptr(expected),nullptr,GL_DYNAMIC_COPY);assert(glGetError()==GL_NO_ERROR);GLint64 actual=0;glGetBufferParameteri64v(GL_SHADER_STORAGE_BUFFER,GL_BUFFER_SIZE,&actual);assert(actual==expected);}
  glDeleteBuffers(21,full.data());for(auto name:full)assert(glIsBuffer(name)==GL_FALSE);
  glDeleteBuffers(GLsizei(buffers.size()),buffers.data());assert(glGetError()==GL_NO_ERROR);
  for(auto name:buffers)assert(glIsBuffer(name)==GL_FALSE);
  std::cout<<"T522 actual GL allocation/size/deletion "<<p.k<<'/'<<p.kw<<" total="<<allocation.total<<" padding="<<allocation.padding<<" PASS\n";
 }
 eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);eglDestroyContext(display,context);eglDestroySurface(display,surface);eglTerminate(display);
}
