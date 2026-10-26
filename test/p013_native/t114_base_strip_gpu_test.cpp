#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <cstdio>
#include <vector>
#include "base_strip_uploader.h"
using namespace p013;
#define CHECK(x) do{if(!(x)){std::printf("FAIL line %d egl=%x gl=%x\n",__LINE__,eglGetError(),glGetError());return 1;}}while(0)
int main(){
 auto display=eglGetDisplay(EGL_DEFAULT_DISPLAY);CHECK(eglInitialize(display,nullptr,nullptr));
 CHECK(eglBindAPI(EGL_OPENGL_ES_API));EGLint a[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,
  EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_NONE};
 EGLConfig cfg;EGLint n=0;CHECK(eglChooseConfig(display,a,&cfg,1,&n)&&n);
 EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE};auto ctx=eglCreateContext(display,cfg,EGL_NO_CONTEXT,ca);CHECK(ctx!=EGL_NO_CONTEXT);
 EGLint sa[]={EGL_WIDTH,128,EGL_HEIGHT,1,EGL_NONE};auto surf=eglCreatePbufferSurface(display,cfg,sa);CHECK(surf!=EGL_NO_SURFACE);
 CHECK(eglMakeCurrent(display,surf,surf,ctx));
 std::vector<uint8_t> bytes(128*12);for(int k=0;k<3;k++)for(int j=0;j<128;j++){
  auto i=(k*128+j)*4;bytes[i]=uint8_t(145+k);bytes[i+1]=uint8_t(j);bytes[i+2]=172;bytes[i+3]=1;}
 BaseStripGlGpu gl;BaseStripUploader uploader(gl);BaseStripDesc d;d.session=d.serial=d.stateEpoch=d.commit=d.bindingId=
  d.geometry=d.context=d.surface=1;d.width=128;d.height=1;d.boxWidth=128;d.token="gpu";d.run="gpu";
 d.digest=std::string(64,'a');d.profileHash=std::string(64,'b');d.paletteHash=std::string(64,'c');
 d.payloadHash=MaterialSha256(bytes.data(),bytes.size());CHECK(uploader.Stage(d,bytes.data(),bytes.size()).code==0);
 CHECK(uploader.Pump(7,1,1).uploadedBytes==4);
 while(uploader.Inspect().pendingBytes)CHECK(uploader.Pump(509,1,1).code==0);
 BaseStripLease lease;CHECK(uploader.Acquire(d,lease)&&lease.texture);
 GLuint fbo=0;glGenFramebuffers(1,&fbo);glBindFramebuffer(GL_FRAMEBUFFER,fbo);
 std::vector<uint8_t> actual(128*4);
 for(int k=0;k<3;k++){
  glFramebufferTextureLayer(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,lease.texture,0,k);
  CHECK(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE);
  glReadPixels(0,0,128,1,GL_RGBA_INTEGER,GL_UNSIGNED_BYTE,actual.data());CHECK(glGetError()==GL_NO_ERROR);
  for(int j=0;j<128*4;j++)CHECK(actual[j]==bytes[k*128*4+j]);
 }
 uploader.CommitAfterSwap(lease);CHECK(uploader.Inspect().textureLiveCount==1);
 glDeleteFramebuffers(1,&fbo);uploader.ClearSession(true);CHECK(uploader.Inspect().textureLiveCount==0);
 CHECK(eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT));eglDestroySurface(display,surf);
 eglDestroyContext(display,ctx);eglTerminate(display);std::puts("T114 RGBA8UI three-band GPU readback PASS");
}
