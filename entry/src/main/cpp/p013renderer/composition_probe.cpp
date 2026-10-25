// G0 only. Fragment-generated linear color stripes and binary alpha; no material recipe.
#include "composition_probe.h"
#include <cstring>
namespace p013 {
namespace {
const char* vert = R"GLSL(#version 300 es
void main(){ vec2 p=vec2((gl_VertexID<<1)&2,gl_VertexID&2); gl_Position=vec4(p*2.-1.,0.,1.); }
)GLSL";
const char* frag = R"GLSL(#version 300 es
precision highp float;
uniform vec4 u_box;
out vec4 color;
void main(){
 vec2 uv=(gl_FragCoord.xy-u_box.xy)/u_box.zw;
 if(uv.x<0. || uv.x>=1. || uv.y<0. || uv.y>=0.5){color=vec4(0.);return;}
 float c=uv.x<0.2?0.:uv.x<0.4?0.0031308:uv.x<0.6?0.18:uv.x<0.8?0.5:1.;
 color=vec4(vec3(c),1.);
}
)GLSL";
GLuint Compile(GLenum type, const char* text) {
 GLuint s=glCreateShader(type); glShaderSource(s,1,&text,nullptr); glCompileShader(s);
 GLint ok=0; glGetShaderiv(s,GL_COMPILE_STATUS,&ok);
 if(!ok){glDeleteShader(s);return 0;}return s;
}
}
void CompositionProbe::Reset(bool deleteGpu){
 if(program){if(deleteGpu)glDeleteProgram(program);else programAbandonCount++;}
 if(vao){if(deleteGpu)glDeleteVertexArrays(1,&vao);else vaoAbandonCount++;}
 program=vao=0;epoch=0;captured=false;
}
Status CompositionProbe::Draw(const FrameInput& f, uint64_t generation) {
 captured=false;error=GL_NO_ERROR;
 if(epoch!=generation){
  Reset();epoch=generation;
  GLuint v=Compile(GL_VERTEX_SHADER,vert),p=Compile(GL_FRAGMENT_SHADER,frag);
  if(!v||!p){if(v)glDeleteShader(v);if(p)glDeleteShader(p);return Status::SHADER_COMPILE_FAILED;}
  program=glCreateProgram();glAttachShader(program,v);glAttachShader(program,p);glLinkProgram(program);
  glDeleteShader(v);glDeleteShader(p);GLint ok=0;glGetProgramiv(program,GL_LINK_STATUS,&ok);
  if(!ok)return Status::SHADER_COMPILE_FAILED;
  glGenVertexArrays(1,&vao);
  sourceHash=14695981039346656037ULL;
  for(const char* source : {vert,frag})for(size_t i=0;i<std::strlen(source);i++){sourceHash^=static_cast<unsigned char>(source[i]);sourceHash*=1099511628211ULL;}
 }
 glBindFramebuffer(GL_FRAMEBUFFER,0);glViewport(0,0,f.surfaceWidthPx,f.surfaceHeightPx);
 glDisable(GL_BLEND);glDisable(GL_DEPTH_TEST);glDisable(GL_STENCIL_TEST);glDisable(GL_SCISSOR_TEST);glDisable(GL_CULL_FACE);
 glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);glDisable(GL_SAMPLE_COVERAGE);
 glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);glClearColor(0,0,0,0);glClear(GL_COLOR_BUFFER_BIT);
 glUseProgram(program);glBindVertexArray(vao);
 const float y=static_cast<float>(f.surfaceHeightPx-f.boxYPx-f.boxHeightPx);
 glUniform4f(glGetUniformLocation(program,"u_box"),f.boxXPx,y,f.boxWidthPx,f.boxHeightPx);
 if(f.diagnosticEnabled)glDrawArrays(GL_TRIANGLES,0,3);
 GLint previousPack=4;glGetIntegerv(GL_PACK_ALIGNMENT,&previousPack);
 glPixelStorei(GL_PACK_ALIGNMENT,1);glReadBuffer(GL_BACK);
 for(size_t i=0;i<6;i++){
  float x=i<5?static_cast<float>(f.boxXPx+(i+0.5)*f.boxWidthPx/5):0;
  float py=i<5?y+static_cast<float>(f.boxHeightPx*.25):static_cast<float>(f.surfaceHeightPx-1);
  int ix=static_cast<int>(x),iy=static_cast<int>(py);
  points[i*2]=static_cast<float>(ix);points[i*2+1]=static_cast<float>(iy);
  glReadPixels(ix,iy,1,1,GL_RGBA,GL_UNSIGNED_BYTE,rgba.data()+i*4);
 }
 // Separate offscreen controls cannot substitute for the default attachment evidence.
 for(int arm=0;arm<2;arm++){
  GLuint texture=0,fb=0;glGenTextures(1,&texture);glBindTexture(GL_TEXTURE_2D,texture);
  glTexImage2D(GL_TEXTURE_2D,0,arm==0?GL_RGBA8:GL_SRGB8_ALPHA8,64,32,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
  glGenFramebuffers(1,&fb);glBindFramebuffer(GL_FRAMEBUFFER,fb);
  glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,texture,0);
  if(glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE){
   glBindFramebuffer(GL_FRAMEBUFFER,0);glDeleteFramebuffers(1,&fb);glDeleteTextures(1,&texture);return Status::GL_RESOURCE_FAILED;
  }
  glViewport(0,0,64,32);glClearColor(0,0,0,0);glClear(GL_COLOR_BUFFER_BIT);
  glUniform4f(glGetUniformLocation(program,"u_box"),0,0,64,32);
  if(f.diagnosticEnabled)glDrawArrays(GL_TRIANGLES,0,3);
  auto& target=arm==0?linearRgba:srgbRgba;
  for(size_t i=0;i<6;i++)glReadPixels(i<5?static_cast<int>((i+.5)*64/5):0,i<5?8:31,1,1,GL_RGBA,GL_UNSIGNED_BYTE,target.data()+i*4);
  glBindFramebuffer(GL_FRAMEBUFFER,0);glDeleteFramebuffers(1,&fb);glDeleteTextures(1,&texture);
 }
 glViewport(0,0,f.surfaceWidthPx,f.surfaceHeightPx);
 glPixelStorei(GL_PACK_ALIGNMENT,previousPack);glBindVertexArray(0);glUseProgram(0);
 error=glGetError();if(error!=GL_NO_ERROR)return Status::GL_DRAW_FAILED;
 frame=f;captured=true;return Status::OK;
}
}
