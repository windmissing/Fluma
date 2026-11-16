#include <cstdint>
#include <EGL/egl.h>
#include <GLES3/gl31.h>
#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>
#include <string>
#include "../t522/gl_api.inc"
int main(){
 using F=EGLDisplay(EGLAPIENTRYP)(EGLenum,void*,const EGLint*);PROC(F,eglGetPlatformDisplayEXT);
 const EGLint displayArgs[]={0x3203,0x3450,0x3209,0x3487,EGL_NONE};auto display=f(0x3202,nullptr,displayArgs);assert(eglInitialize(display,nullptr,nullptr));
 const EGLint attributes[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_NONE};EGLConfig config;GLint count;assert(eglChooseConfig(display,attributes,&config,1,&count)&&count);
 const EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,3,0x30FB,1,EGL_NONE},sa[]={EGL_WIDTH,8,EGL_HEIGHT,8,EGL_NONE};auto context=eglCreateContext(display,config,EGL_NO_CONTEXT,ca);auto surface=eglCreatePbufferSurface(display,config,sa);assert(eglMakeCurrent(display,surface,surface,context));
 std::ifstream in(".codex/local-test/t517-lineage.glsl",std::ios::binary);assert(in);std::string shader{std::istreambuf_iterator<char>(in),std::istreambuf_iterator<char>()};
 auto sh=glCreateShader(GL_COMPUTE_SHADER);auto src=shader.c_str();glShaderSource(sh,1,&src,nullptr);glCompileShader(sh);GLint ok;glGetShaderiv(sh,GL_COMPILE_STATUS,&ok);if(!ok){char log[8192];glGetShaderInfoLog(sh,sizeof(log),nullptr,log);std::cerr<<log;return 1;}
 auto program=glCreateProgram();glAttachShader(program,sh);glLinkProgram(program);glGetProgramiv(program,GL_LINK_STATUS,&ok);assert(ok);glUseProgram(program);
 GLuint buffers[4];glGenBuffers(4,buffers);std::vector<uint32_t> data(43136);
 for(unsigned i=0;i<4;++i){glBindBuffer(GL_SHADER_STORAGE_BUFFER,buffers[i]);glBufferData(GL_SHADER_STORAGE_BUFFER,data.size()*4,data.data(),GL_DYNAMIC_COPY);glBindBufferBase(GL_SHADER_STORAGE_BUFFER,i,buffers[i]);}
 const std::pair<const char*,uint32_t> uniforms[]={{"uK",192},{"uColumn",4096},{"uV5Cohort",6400},{"uV5Depth",39168},{"uV5Certificate",39296},{"uV5Aux",42368}};
 for(auto u:uniforms)glUniform1ui(glGetUniformLocation(program,u.first),u.second);
 for(uint32_t pass=0;pass<3;++pass){glUniform1ui(glGetUniformLocation(program,"uPass"),pass);glDispatchCompute(1,1,1);glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT|GL_BUFFER_UPDATE_BARRIER_BIT);assert(glGetError()==GL_NO_ERROR);}
 glBindBuffer(GL_COPY_READ_BUFFER,buffers[2]);auto w=static_cast<const uint32_t*>(glMapBufferRange(GL_COPY_READ_BUFFER,0,128,GL_MAP_READ_BIT));assert(w);
 const uint32_t expected[]={1,1,1,1,1,80,1,1,1,1,1,1,26,74,1,74,1,64994,1,0,1,8388608,1,0,1,uint32_t(-33554432),58117952,1};for(size_t i=0;i<28;++i){if(w[i]!=expected[i])std::cerr<<"word "<<i<<" got "<<w[i]<<" expected "<<expected[i]<<"\n";assert(w[i]==expected[i]);}assert(glUnmapBuffer(GL_COPY_READ_BUFFER));
 glBindBuffer(GL_COPY_READ_BUFFER,buffers[1]);auto c=static_cast<const uint32_t*>(glMapBufferRange(GL_COPY_READ_BUFFER,0,data.size()*4,GL_MAP_READ_BIT));assert(c);for(size_t slot=0;slot<2;++slot){for(size_t j=0;j<16;++j)assert(c[39296+16*slot+j]==0);for(size_t j=0;j<4;++j)assert(c[42368+4*slot+j]==0);}assert(glUnmapBuffer(GL_COPY_READ_BUFFER));
 glDeleteBuffers(4,buffers);glDeleteShader(sh);glDeleteProgram(program);eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);eglDestroyContext(display,context);eglDestroySurface(display,surface);eglTerminate(display);
 std::cout<<"T517 actual ES3.1 lineage: coalesce/top-first/depth64 wait/prefix/merge/partial+full return/certificate retirement PASS\n";
}
