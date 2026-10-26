// T145 independent color roundtrip probe (tester-owned, not the T144 executable).
// Wider domain than T144: 4-gray-level roundtrip matrix through the real sampler chain.
// default GL_BACK draw/read -> SRGB8_ALPHA8 upload of ACTUAL bytes -> sampler2D -> LINEAR RGBA8 FBO.
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
static const char* samplerSrc=R"(#version 300 es
precision highp float;uniform sampler2D uTex;uniform float uLevel;out vec4 o;
void main(){vec4 t=texture(uTex,vec2(0.5,0.5));o=vec4(vec3(t.r),t.a);})";
static GLuint shader(GLenum kind,const char* src){GLuint s=glCreateShader(kind);glShaderSource(s,1,&src,nullptr);glCompileShader(s);
  GLint ok=0;glGetShaderiv(s,GL_COMPILE_STATUS,&ok);if(!ok){char log[1024]={};glGetShaderInfoLog(s,1024,nullptr,log);
    std::fprintf(stderr,"compile: %s\n",log);return 0;}return s;}
static GLuint program(const char* frag){GLuint v=shader(GL_VERTEX_SHADER,vert),f=shader(GL_FRAGMENT_SHADER,frag);if(!v||!f)return 0;
  GLuint p=glCreateProgram();glAttachShader(p,v);glAttachShader(p,f);glLinkProgram(p);GLint ok=0;glGetProgramiv(p,GL_LINK_STATUS,&ok);
  if(!ok){char log[1024]={};glGetProgramInfoLog(p,1024,nullptr,log);std::fprintf(stderr,"link: %s\n",log);return 0;}
  glDeleteShader(v);glDeleteShader(f);return p;}
static std::string enc(GLenum attachment,std::string& e){GLint x=0;
  glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER,attachment,GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING,&x);
  e=err();return e!="NONE"?"ERROR":x==GL_SRGB?"SRGB":x==GL_LINEAR?"LINEAR":"OTHER";}
int main(int argc,char**argv){if(argc<2)return 2;std::string out=argv[1];
  EGLDisplay d=eglGetDisplay(EGL_DEFAULT_DISPLAY);if(d==EGL_NO_DISPLAY||!eglInitialize(d,nullptr,nullptr))return 2;
  EGLint ca[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_RED_SIZE,8,
    EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_NONE};EGLConfig cfg;EGLint count=0;
  if(!eglChooseConfig(d,ca,&cfg,1,&count)||count<1||!eglBindAPI(EGL_OPENGL_ES_API))return 2;
  EGLint co[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE};EGLContext ctx=eglCreateContext(d,cfg,EGL_NO_CONTEXT,co);
  EGLint sa[]={EGL_WIDTH,64,EGL_HEIGHT,64,EGL_NONE};EGLSurface surf=eglCreatePbufferSurface(d,cfg,sa);
  if(ctx==EGL_NO_CONTEXT||surf==EGL_NO_SURFACE||!eglMakeCurrent(d,surf,surf,ctx))return 2;
  GLuint vao=0;glGenVertexArrays(1,&vao);glBindVertexArray(vao);std::string vaoE=err();
  GLuint prog=program(samplerSrc);if(!prog)return 3;
  // target: LINEAR RGBA8 FBO
  GLuint linTex=0,linFbo=0;glGenTextures(1,&linTex);glBindTexture(GL_TEXTURE_2D,linTex);
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);
  glGenFramebuffers(1,&linFbo);glBindFramebuffer(GL_FRAMEBUFFER,linFbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,linTex,0);
  GLenum complete=glCheckFramebufferStatus(GL_FRAMEBUFFER);
  std::string setupE=err(),tqE,tEnc=enc(GL_COLOR_ATTACHMENT0,tqE);
  GLint fboBind=0;glGetIntegerv(GL_FRAMEBUFFER_BINDING,&fboBind);
  // default BACK encoding query
  glBindFramebuffer(GL_FRAMEBUFFER,0);std::string bindDefE=err(),dqE,dEnc=enc(GL_BACK,dqE);
  glViewport(0,0,64,64);glDisable(GL_BLEND);glDisable(GL_DITHER);glUseProgram(prog);
  // per-gray-level: byte b -> uLevel = b/255 fed through uniform into a texture-free path is NOT
  // the chain; the chain requires the value to live in the SRGB texture. We upload each byte
  // directly as texel data (the "actual bytes" requirement) and sample it.
  int tri[4][4];int smp[4][4];std::string uploadE[4],drawE[4],readE[4];bool srcBound[4];
  const unsigned char levels[4]={0,64,128,200};
  for(int i=0;i<4;i++){
    unsigned char texel[4]={levels[i],levels[i],levels[i],255};
    GLuint t=0;glGenTextures(1,&t);glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,t);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D,0,GL_SRGB8_ALPHA8,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,texel);
    GLint b=0;glGetIntegerv(GL_TEXTURE_BINDING_2D,&b);srcBound[i]=b==GLint(t);
    uploadE[i]=err();
    glViewport(0,0,1,1);glBindFramebuffer(GL_FRAMEBUFFER,linFbo);
    glUseProgram(prog);glUniform1i(glGetUniformLocation(prog,"uTex"),0);
    glDrawArrays(GL_TRIANGLES,0,3);drawE[i]=err();
    unsigned char px[4]={};glReadPixels(0,0,1,1,GL_RGBA,GL_UNSIGNED_BYTE,px);readE[i]=err();
    for(int c=0;c<4;c++)smp[i][c]=px[c];
    glDeleteTextures(1,&t);
    (void)tri;
  }
  // validity: all stage errors NONE, bindings correct, target LINEAR complete,
  // sampled == round(eotf(level/255)*255) within rounding, alpha 255 passthrough
  auto eotf=[](double v){return v<=0.04045?v/12.92:std::pow((v+0.055)/1.055,2.4);};
  bool math=true;double delta[4];
  for(int i=0;i<4;i++){
    double expected=eotf(levels[i]/255.0)*255.0;
    delta[i]=smp[i][0]-expected;
    if(std::abs(delta[i])>1.0)math=false; // 1 LSB: hardware sRGB LUT quantization (same convention as T144)
    if(smp[i][3]!=255)math=false;
  }
  bool valid=vaoE=="NONE"&&bindDefE=="NONE"&&dqE=="NONE"&&tqE=="NONE"&&complete==GL_FRAMEBUFFER_COMPLETE&&
    tEnc=="LINEAR"&&fboBind==GLint(linFbo)&&math;
  for(int i=0;i<4;i++)valid=valid&&srcBound[i]&&uploadE[i]=="NONE"&&drawE[i]=="NONE"&&readE[i]=="NONE";
  std::ostringstream j;j<<"{\"revision\":\"T145_ROUNDTRIP_INDEPENDENT_V1\",\"status\":\""<<(valid?"VALID":"FAULT")
    <<"\",\"probeOwner\":\"tester\",\"uploadedInternalFormat\":\"SRGB8_ALPHA8\",\"samplerUsed\":true"
    <<",\"target\":\"RGBA8_LINEAR_FBO\",\"defaultEncoding\":\""<<dEnc<<"\",\"targetEncoding\":\""<<tEnc
    <<"\",\"targetComplete\":"<<(complete==GL_FRAMEBUFFER_COMPLETE?"true":"false")
    <<",\"fboBound\":"<<(fboBind==GLint(linFbo)?"true":"false")
    <<",\"matrix\":[";
  for(int i=0;i<4;i++){j<<"["<<int(levels[i])<<","<<smp[i][0]<<","<<smp[i][3]<<"]";if(i<3)j<<",";}
  j<<"],\"errors\":{\"vao\":\""<<vaoE<<"\",\"bindDefault\":\""<<bindDefE<<"\",\"defaultQuery\":\""<<dqE
    <<"\",\"targetQuery\":\""<<tqE<<"\"";
  for(int i=0;i<4;i++)j<<",\"upload"<<i<<"\":\""<<uploadE[i]<<"\",\"draw"<<i<<"\":\""<<drawE[i]<<"\",\"read"<<i<<"\":\""<<readE[i]<<"\"";
  j<<"}}\n";
  std::ofstream f(out,std::ios::binary);f<<j.str();f.close();
  glDeleteFramebuffers(1,&linFbo);glDeleteTextures(1,&linTex);
  glDeleteProgram(prog);glDeleteVertexArrays(1,&vao);
  eglMakeCurrent(d,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);eglDestroySurface(d,surf);eglDestroyContext(d,ctx);eglTerminate(d);
  std::puts(valid?"T145 roundtrip VALID":"T145 roundtrip FAULT");return valid?0:4;
}
