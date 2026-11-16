#include <cstdint>
#include <EGL/egl.h>
#include <GLES3/gl31.h>
#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>
#include <string>
#include "gl_api.inc"
static std::string Read(const std::string& path){std::ifstream in(path,std::ios::binary);assert(in);return {std::istreambuf_iterator<char>(in),std::istreambuf_iterator<char>()};}
int main(int argc,char** argv){
 using F=EGLDisplay(EGLAPIENTRYP)(EGLenum,void*,const EGLint*);PROC(F,eglGetPlatformDisplayEXT);
 const EGLint displayArgs[]={0x3203,0x3450,0x3209,0x3487,EGL_NONE};auto display=f(0x3202,nullptr,displayArgs);assert(eglInitialize(display,nullptr,nullptr));
 const EGLint attributes[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_NONE};EGLConfig config;EGLint count;assert(eglChooseConfig(display,attributes,&config,1,&count)&&count);
 const EGLint contextArgs[]={EGL_CONTEXT_CLIENT_VERSION,3,0x30FB,1,EGL_NONE},surfaceArgs[]={EGL_WIDTH,8,EGL_HEIGHT,8,EGL_NONE};auto context=eglCreateContext(display,config,EGL_NO_CONTEXT,contextArgs);auto surface=eglCreatePbufferSurface(display,config,surfaceArgs);assert(eglMakeCurrent(display,surface,surface,context));
 std::string shader=Read(argc>1?argv[1]:".codex/local-test/t522-solver/seed.glsl");
 GLuint sh=glCreateShader(GL_COMPUTE_SHADER);const char* src=shader.c_str();glShaderSource(sh,1,&src,nullptr);glCompileShader(sh);GLint ok;glGetShaderiv(sh,GL_COMPILE_STATUS,&ok);if(!ok){char log[8192];glGetShaderInfoLog(sh,sizeof(log),nullptr,log);std::cerr<<log;return 1;}auto program=glCreateProgram();glAttachShader(program,sh);glLinkProgram(program);glGetProgramiv(program,GL_LINK_STATUS,&ok);assert(ok);glDeleteShader(sh);glUseProgram(program);
 if(argc>1){glDeleteProgram(program);eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);eglDestroyContext(display,context);eglDestroySurface(display,surface);eglTerminate(display);std::cout<<"T522 ES3.1 shader compilation/link PASS: "<<argv[1]<<"\n";return 0;}
 GLuint buffers[4];glGenBuffers(4,buffers);
 std::vector<uint32_t> base(6400),state(6400),work(8192+61444),events(5153*128);
 for(size_t c=0;c<128;++c){state[4096+128*6+c]=1048576;state[4096+128*12+c]=state[4096+128*13+c]=1048576;}
 state[6144+39]=1;state[6144+43]=192;base[0]=122333;base[1]=122334;
 work[2000+2]=2;work[2000+14]=1;work[3000+7680*4+2]=1048576;
 const std::vector<uint32_t>* data[]={&base,&state,&work,&events};for(unsigned i=0;i<4;++i){glBindBuffer(GL_SHADER_STORAGE_BUFFER,buffers[i]);glBufferData(GL_SHADER_STORAGE_BUFFER,data[i]->size()*4,data[i]->data(),GL_DYNAMIC_COPY);glBindBufferBase(GL_SHADER_STORAGE_BUFFER,i,buffers[i]);}
 const std::pair<const char*,uint32_t> uniforms[]={{"uK",192},{"uKw",64},{"uWater",3072},{"uColumn",4096},{"uHeader",6144},{"uM3Scratch",4096},{"uM3SourceRows",5153},{"uM3Input",2000},{"uM3Index0",2048},{"uM3Index1",2304},{"uM3YRequest",500000},{"uM3Settle",65536}};
 for(auto u:uniforms)glUniform1ui(glGetUniformLocation(program,u.first),u.second);
 glUniform1i(glGetUniformLocation(program,"uM3Cos"),1048576);
 for(unsigned pass=0;pass<2;++pass){glUniform1ui(glGetUniformLocation(program,"uPass"),pass);glDispatchCompute(1,1,1);glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT|GL_BUFFER_UPDATE_BARRIER_BIT);assert(glGetError()==GL_NO_ERROR);}
 glBindBuffer(GL_COPY_READ_BUFFER,buffers[2]);auto workspace=static_cast<const uint32_t*>(glMapBufferRange(GL_COPY_READ_BUFFER,0,work.size()*4,GL_MAP_READ_BIT));assert(workspace);std::cout<<"seed error="<<workspace[0]<<" reason="<<workspace[4097]<<" births="<<workspace[4116]<<" born="<<workspace[4119]<<"\n";assert(workspace[0]==0&&workspace[4116]==3&&workspace[4119]==244667);assert(glUnmapBuffer(GL_COPY_READ_BUFFER));
 glBindBuffer(GL_COPY_READ_BUFFER,buffers[1]);auto actual=static_cast<const uint32_t*>(glMapBufferRange(GL_COPY_READ_BUFFER,0,state.size()*4,GL_MAP_READ_BIT));assert(actual);assert(actual[0]==122333&&actual[1]==122333&&actual[2]==1);assert(actual[4096+8*128]==122333&&actual[4096+8*128+1]==122334);assert(actual[6144+39]==4);assert(glUnmapBuffer(GL_COPY_READ_BUFFER));std::cout<<"T522 real GPU shared origin/column seed order/package tail PASS\n";
 glDeleteBuffers(4,buffers);glDeleteProgram(program);eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);eglDestroyContext(display,context);eglDestroySurface(display,surface);eglTerminate(display);
}
