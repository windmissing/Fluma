#pragma once
#include <array>
#include <chrono>
#include <cstdio>
#include <GLES3/gl3.h>
#include "single_sand_packet.h"

namespace p013 {
struct ProbeRoi {
 int x=0,y=0,w=0,h=0;
 bool Fits(int width,int height) const {
  return x>=0&&y>=0&&w>0&&h>0&&w<=64&&h<=64&&x<=width-w&&y<=height-h;
 }
};
struct ProbeTarget {
 GLint framebuffer=0,buffer=0,encoding=0,samples=0;
 GLenum setupError=GL_NO_ERROR,readError=GL_NO_ERROR,restoreError=GL_NO_ERROR;
 bool read=false;
};
struct SingleSandWindowProbe {
 static constexpr int MaxRois=3, TargetStride=MaxRois*64*64*4;
 SingleSandPacket packet{};
 std::array<double,19> config{};
 std::array<ProbeRoi,MaxRois> roi{};
 std::array<ProbeTarget,2> target{};
 std::array<uint8_t,2*TargetStride> bytes{};
 std::array<GLint,4> savedPack{};
 GLint savedReadFramebuffer=0,savedReadBuffer=0,savedPackBuffer=0;
 char version[128]{},renderer[128]{};
 const char* stage="PRE_PRESENT";
 int count=0,used=0,unbindError=0,drawEglError=0;
 bool available=false,unbound=false,dither=false,blend=false;
 double ms=0;

 bool Configure(const int32_t* coords,int length) {
  if(length<4||length>MaxRois*4||length%4!=0)return false;
  count=length/4;used=0;
  for(int i=0;i<count;i++){
   const auto* c=coords+4*i;
   roi[i]={c[0],c[1],c[2],c[3]};
   if(!roi[i].Fits(packet.width,packet.height))return false;
   used+=4*roi[i].w*roi[i].h;
  }
  return true;
 }
 void Capture(GLuint candidate) {
  const auto begin=std::chrono::steady_clock::now();
  stage="PRE_SWAP";
  const GLenum packNames[]={GL_PACK_ALIGNMENT,GL_PACK_ROW_LENGTH,GL_PACK_SKIP_PIXELS,GL_PACK_SKIP_ROWS};
  glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING,&savedReadFramebuffer);
  glGetIntegerv(GL_READ_BUFFER,&savedReadBuffer);
  glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING,&savedPackBuffer);
  for(int i=0;i<4;i++)glGetIntegerv(packNames[i],&savedPack[i]);
  dither=glIsEnabled(GL_DITHER);blend=glIsEnabled(GL_BLEND);
  const auto* v=glGetString(GL_VERSION);
  const auto* r=glGetString(GL_RENDERER);
  if(v)std::snprintf(version,sizeof(version),"%s",reinterpret_cast<const char*>(v));
  if(r)std::snprintf(renderer,sizeof(renderer),"%s",reinterpret_cast<const char*>(r));
  glBindBuffer(GL_PIXEL_PACK_BUFFER,0);
  glPixelStorei(GL_PACK_ALIGNMENT,1);
  for(int i=1;i<4;i++)glPixelStorei(packNames[i],0);
  ReadTarget(0,candidate,GL_COLOR_ATTACHMENT0);
  ReadTarget(1,0,GL_BACK);
  glBindFramebuffer(GL_READ_FRAMEBUFFER,savedReadFramebuffer);
  glReadBuffer(savedReadBuffer);
  glBindBuffer(GL_PIXEL_PACK_BUFFER,savedPackBuffer);
  for(int i=0;i<4;i++)glPixelStorei(packNames[i],savedPack[i]);
  const GLenum restore=DrainErrors();
  if(restore!=GL_NO_ERROR)target[1].restoreError=restore;
  ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count();
 }
 private:
 static GLenum DrainErrors(){
  const GLenum first=glGetError();
  if(first!=GL_NO_ERROR)for(int i=0;i<15&&glGetError()!=GL_NO_ERROR;i++){}
  return first;
 }
 void ReadTarget(int index,GLuint fbo,GLenum selector){
  auto& t=target[index];
  glBindFramebuffer(GL_READ_FRAMEBUFFER,fbo);
  GLint previous=0;glGetIntegerv(GL_READ_BUFFER,&previous);
  glReadBuffer(selector);
  glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING,&t.framebuffer);
  glGetIntegerv(GL_READ_BUFFER,&t.buffer);
  // GL_SAMPLES uses the draw binding. Read attachment samples are fixed by
  // FullFrameTargets (single sample) or by the still-bound default draw FBO.
  if(index==1)glGetIntegerv(GL_SAMPLES,&t.samples);
  glGetFramebufferAttachmentParameteriv(GL_READ_FRAMEBUFFER,selector,GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING,&t.encoding);
  t.setupError=DrainErrors();
  if(t.setupError==GL_NO_ERROR&&t.framebuffer==GLint(fbo)&&t.buffer==GLint(selector)){
   int offset=index*TargetStride;
   for(int i=0;i<count;i++){
    const auto& p=roi[i];
    glReadPixels(p.x,p.y,p.w,p.h,GL_RGBA,GL_UNSIGNED_BYTE,bytes.data()+offset);
    t.readError=DrainErrors();
    if(t.readError!=GL_NO_ERROR)break;
    offset+=4*p.w*p.h;
   }
   t.read=t.readError==GL_NO_ERROR;
  }
  glReadBuffer(previous);
  t.restoreError=DrainErrors();
 }
};
}
