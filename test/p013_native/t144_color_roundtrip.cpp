// T144 isolated color D probe: default draw/read -> real SRGB texture upload
// -> sampler2D decode -> explicit LINEAR RGBA8 FBO read. No product renderer.
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
static std::string err(){GLenum e=glGetError();if(e==GL_NO_ERROR)return "NONE";std::ostringstream o;o<<"0x"<<std::hex<<e;return o.str();}
static const char* vert=R"(#version 300 es
void main(){vec2 p=vec2((gl_VertexID==1)?3.0:-1.0,(gl_VertexID==2)?3.0:-1.0);gl_Position=vec4(p,0.0,1.0);})";
static const char* constant=R"(#version 300 es
precision highp float;out vec4 o;void main(){o=vec4(0.5,0.5,0.5,1.0);})";
static const char* sampler=R"(#version 300 es
precision highp float;uniform sampler2D uTex;out vec4 o;
void main(){o=texture(uTex,vec2(0.5,0.5));})";
static GLuint shader(GLenum kind,const char* src){GLuint s=glCreateShader(kind);glShaderSource(s,1,&src,nullptr);glCompileShader(s);
  GLint ok=0;glGetShaderiv(s,GL_COMPILE_STATUS,&ok);if(!ok){char log[1024]={};glGetShaderInfoLog(s,1024,nullptr,log);
    std::fprintf(stderr,"shader compile: %s\n",log);return 0;}return s;}
static GLuint program(const char* frag){GLuint v=shader(GL_VERTEX_SHADER,vert),f=shader(GL_FRAGMENT_SHADER,frag);if(!v||!f)return 0;
  GLuint p=glCreateProgram();glAttachShader(p,v);glAttachShader(p,f);glLinkProgram(p);GLint ok=0;glGetProgramiv(p,GL_LINK_STATUS,&ok);
  if(!ok){char log[1024]={};glGetProgramInfoLog(p,1024,nullptr,log);std::fprintf(stderr,"program link: %s\n",log);return 0;}
  glDeleteShader(v);glDeleteShader(f);return p;}
static std::string encoding(GLenum attachment,std::string& stageError){GLint x=0;
  glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER,attachment,GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING,&x);
  stageError=err();return stageError!="NONE"?"ERROR":x==GL_SRGB?"SRGB":x==GL_LINEAR?"LINEAR":"OTHER";}
int main(int argc,char**argv){if(argc<2)return 2;std::string out=argv[1];
  EGLDisplay d=eglGetDisplay(EGL_DEFAULT_DISPLAY);if(d==EGL_NO_DISPLAY||!eglInitialize(d,nullptr,nullptr))return 2;
  EGLint ca[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_RED_SIZE,8,
    EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_NONE};EGLConfig cfg;EGLint count=0;
  if(!eglChooseConfig(d,ca,&cfg,1,&count)||count<1||!eglBindAPI(EGL_OPENGL_ES_API))return 2;
  EGLint co[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE};EGLContext ctx=eglCreateContext(d,cfg,EGL_NO_CONTEXT,co);
  EGLint sa[]={EGL_WIDTH,160,EGL_HEIGHT,288,EGL_NONE};EGLSurface surf=eglCreatePbufferSurface(d,cfg,sa);
  if(ctx==EGL_NO_CONTEXT||surf==EGL_NO_SURFACE||!eglMakeCurrent(d,surf,surf,ctx))return 2;
  GLuint vao=0;glGenVertexArrays(1,&vao);glBindVertexArray(vao);std::string vaoError=err();
  GLuint drawProgram=program(constant),sampleProgram=program(sampler);
  if(!drawProgram||!sampleProgram)return 3;
  glBindFramebuffer(GL_FRAMEBUFFER,0);std::string bindDefaultError=err();
  std::string defaultQueryError,defaultEncoding=encoding(GL_BACK,defaultQueryError);
  glViewport(0,0,160,288);glDisable(GL_BLEND);glDisable(GL_DITHER);glUseProgram(drawProgram);
  glClearColor(0,0,0,0);glClear(GL_COLOR_BUFFER_BIT);glDrawArrays(GL_TRIANGLES,0,3);
  std::string defaultDrawError=err();unsigned char triRead[4]={};
  glReadPixels(80,144,1,1,GL_RGBA,GL_UNSIGNED_BYTE,triRead);std::string defaultReadError=err();
  // Upload the ACTUAL default readback bytes, not a constant or null texture.
  GLuint triTex=0;glGenTextures(1,&triTex);glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,triTex);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
  glTexImage2D(GL_TEXTURE_2D,0,GL_SRGB8_ALPHA8,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,triRead);
  GLint sourceBinding=0;glGetIntegerv(GL_TEXTURE_BINDING_2D,&sourceBinding);
  std::string uploadError=err();
  GLuint linearTex=0,linearFbo=0;glGenTextures(1,&linearTex);glBindTexture(GL_TEXTURE_2D,linearTex);
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);
  glGenFramebuffers(1,&linearFbo);glBindFramebuffer(GL_FRAMEBUFFER,linearFbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,linearTex,0);
  GLenum complete=glCheckFramebufferStatus(GL_FRAMEBUFFER);GLint targetBinding=0;
  glGetIntegerv(GL_FRAMEBUFFER_BINDING,&targetBinding);std::string targetSetupError=err();
  std::string targetQueryError,targetEncoding=encoding(GL_COLOR_ATTACHMENT0,targetQueryError);
  glViewport(0,0,1,1);glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,triTex);
  glUseProgram(sampleProgram);glUniform1i(glGetUniformLocation(sampleProgram,"uTex"),0);
  glDrawArrays(GL_TRIANGLES,0,3);std::string sampleDrawError=err();
  unsigned char sampled[4]={};glReadPixels(0,0,1,1,GL_RGBA,GL_UNSIGNED_BYTE,sampled);
  std::string sampleReadError=err();
  bool valid=vaoError=="NONE"&&bindDefaultError=="NONE"&&defaultQueryError=="NONE"&&
    defaultDrawError=="NONE"&&defaultReadError=="NONE"&&uploadError=="NONE"&&
    targetSetupError=="NONE"&&targetQueryError=="NONE"&&complete==GL_FRAMEBUFFER_COMPLETE&&
    targetEncoding=="LINEAR"&&sourceBinding==GLint(triTex)&&targetBinding==GLint(linearFbo)&&
    sampleDrawError=="NONE"&&sampleReadError=="NONE";
  std::ostringstream j;j<<"{\"revision\":\"T144_COLOR_ROUNDTRIP_V1\",\"status\":\""<<(valid?"VALID":"FAULT")
    <<"\",\"source\":\"DEFAULT_GL_BACK_READBACK\",\"uploadedInternalFormat\":\"SRGB8_ALPHA8\""
    <<",\"samplerUsed\":true,\"target\":\"RGBA8_LINEAR_FBO\",\"defaultEncoding\":\""<<defaultEncoding
    <<"\",\"targetEncoding\":\""<<targetEncoding<<"\",\"targetComplete\":"<<(complete==GL_FRAMEBUFFER_COMPLETE?"true":"false")
    <<",\"sourceTextureBound\":"<<(sourceBinding==GLint(triTex)?"true":"false")
    <<",\"linearFboBound\":"<<(targetBinding==GLint(linearFbo)?"true":"false")
    <<",\"triRead\":["<<int(triRead[0])<<","<<int(triRead[1])<<","<<int(triRead[2])<<","<<int(triRead[3])
    <<"],\"sampled\":["<<int(sampled[0])<<","<<int(sampled[1])<<","<<int(sampled[2])<<","<<int(sampled[3])<<"],\"errors\":{"
    <<"\"vao\":\""<<vaoError<<"\",\"bindDefault\":\""<<bindDefaultError<<"\",\"defaultQuery\":\""<<defaultQueryError
    <<"\",\"defaultDraw\":\""<<defaultDrawError<<"\",\"defaultRead\":\""<<defaultReadError
    <<"\",\"textureUpload\":\""<<uploadError<<"\",\"targetSetup\":\""<<targetSetupError
    <<"\",\"targetQuery\":\""<<targetQueryError<<"\",\"sampleDraw\":\""<<sampleDrawError
    <<"\",\"sampleRead\":\""<<sampleReadError<<"\"}}\n";
  std::ofstream f(out,std::ios::binary);f<<j.str();f.close();
  glDeleteFramebuffers(1,&linearFbo);glDeleteTextures(1,&linearTex);glDeleteTextures(1,&triTex);
  glDeleteProgram(drawProgram);glDeleteProgram(sampleProgram);glDeleteVertexArrays(1,&vao);
  eglMakeCurrent(d,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);eglDestroySurface(d,surf);eglDestroyContext(d,ctx);eglTerminate(d);
  std::puts(valid?"T144 roundtrip VALID":"T144 roundtrip FAULT");return valid?0:4;
}
