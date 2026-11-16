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
int main(){
 using F=EGLDisplay(EGLAPIENTRYP)(EGLenum,void*,const EGLint*);PROC(F,eglGetPlatformDisplayEXT);
 const EGLint displayArgs[]={0x3203,0x3450,0x3209,0x3487,EGL_NONE};auto display=f(0x3202,nullptr,displayArgs);assert(eglInitialize(display,nullptr,nullptr));
 const EGLint attributes[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_NONE};EGLConfig config;EGLint count;assert(eglChooseConfig(display,attributes,&config,1,&count)&&count);
 const EGLint contextArgs[]={EGL_CONTEXT_CLIENT_VERSION,3,0x30FB,1,EGL_NONE},surfaceArgs[]={EGL_WIDTH,8,EGL_HEIGHT,8,EGL_NONE};auto context=eglCreateContext(display,config,EGL_NO_CONTEXT,contextArgs);auto surface=eglCreatePbufferSurface(display,config,surfaceArgs);assert(eglMakeCurrent(display,surface,surface,context));
 std::string shader="#version 310 es\nprecision highp int;layout(local_size_x=64)in;layout(std430,binding=0)readonly buffer B0{uint B[];};layout(std430,binding=1)buffer B1{uint C[];};layout(std430,binding=2)buffer B2{uint W[];};layout(std430,binding=3)buffer B3{uint E[];};uniform uint uK,uKw,uWater,uColumn,uMode;uniform int uTangent;const uint Q=1048576u,CQ=268435456u;uint col(uint f,uint c){return C[uColumn+128u*f+c];}\n";
 shader+=Read("entry/src/main/cpp/gpu_resident/integer_primitives.glsl");shader+=Read("entry/src/main/cpp/gpu_resident/m3a/occupied_solvers.glsl");shader+=Read("entry/src/main/cpp/gpu_resident/m3a/water_event.glsl");shader+=Read("entry/src/main/cpp/gpu_resident/m3a/water_writer.glsl");shader+="\nvoid main(){if(gl_LocalInvocationIndex==0u){if(uMode==1u){E[20000]=m3WaterTarget(uTangent)?1u:0u;if(E[20000]==1u){E[20001]=m3WaterMatch()?1u:0u;E[20002]=m3WaterProjectionAudit()?1u:0u;}E[20003]=W[1];return;}if(!m3CaptureColumns()){E[0]=99u;return;}for(uint c=0u;c<128u;++c)m3Put(M3_REQUESTED+c,B[c]);E[0]=m3TrialEta(B[128]);for(uint c=0u;c<128u;++c){E[16u+c]=m3Get(M3_TARGET_WATER+c);E[144u+c]=m3Get(M3_ADMITTED+c);}E[1]=m3Get(M3_WANT);E[2]=m3Get(M3_ETA_HI);E[3]=m3Get(M3_RESIDUAL);E[4]=m3Get(M3_LAMBDA);}}";
 GLuint sh=glCreateShader(GL_COMPUTE_SHADER);const char* src=shader.c_str();glShaderSource(sh,1,&src,nullptr);glCompileShader(sh);GLint ok;glGetShaderiv(sh,GL_COMPILE_STATUS,&ok);if(!ok){char log[8192];glGetShaderInfoLog(sh,sizeof(log),nullptr,log);std::cerr<<log;return 1;}auto program=glCreateProgram();glAttachShader(program,sh);glLinkProgram(program);glGetProgramiv(program,GL_LINK_STATUS,&ok);assert(ok);glDeleteShader(sh);glUseProgram(program);
 for(auto name:{"uK","uKw","uWater","uColumn","uM3Scratch"}){const uint32_t value=std::string(name)=="uK"?192:std::string(name)=="uKw"?64:std::string(name)=="uWater"?3072:std::string(name)=="uColumn"?4096:4096;glUniform1ui(glGetUniformLocation(program,name),value);}
 GLuint buffers[4];glGenBuffers(4,buffers);
 for(const auto* name:{"uniform","floor","zero","bound","large_u64","truncation_contract"}){
  auto bytes=Read(std::string(".codex/local-test/t522-solver/")+name+".bin");const auto* expected=reinterpret_cast<const uint32_t*>(bytes.data());assert(bytes.size()==(6+128*5)*4);
  std::vector<uint32_t> base(129),state(6400),work(12288),output(512);for(size_t c=0;c<128;++c){const auto material=expected[6+c],water=expected[134+c];state[4096+128*2+c]=state[4096+128*10+c]=material;state[4096+128*6+c]=water;state[4096+128*11+c]=material;state[4096+128*12+c]=state[4096+128*13+c]=material+water;base[c]=expected[262+c];}base[128]=expected[0];
  if(std::string(name)=="truncation_contract"){state[3072]=1;state[3072+64]=0;state[3072+128]=6;state[4096+128*7]=1;state[4096+128*13]=6;}

  const std::vector<uint32_t>* data[]={&base,&state,&work,&output};for(unsigned i=0;i<4;++i){glBindBuffer(GL_SHADER_STORAGE_BUFFER,buffers[i]);glBufferData(GL_SHADER_STORAGE_BUFFER,data[i]->size()*4,data[i]->data(),GL_DYNAMIC_COPY);glBindBufferBase(GL_SHADER_STORAGE_BUFFER,i,buffers[i]);}
  glDispatchCompute(1,1,1);glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT|GL_SHADER_STORAGE_BARRIER_BIT);glBindBuffer(GL_COPY_READ_BUFFER,buffers[3]);auto mapped=static_cast<const uint32_t*>(glMapBufferRange(GL_COPY_READ_BUFFER,0,output.size()*4,GL_MAP_READ_BIT));assert(mapped);
  assert(mapped[0]==expected[1]&&mapped[1]==expected[2]);if(mapped[0]==0){assert(mapped[2]==expected[3]&&mapped[3]==expected[4]);for(size_t c=0;c<128;++c)assert(mapped[16+c]==expected[390+c]);}for(size_t c=0;c<128;++c)assert(mapped[144+c]==expected[518+c]);assert(mapped[4]==expected[5]);assert(glUnmapBuffer(GL_COPY_READ_BUFFER));assert(glGetError()==GL_NO_ERROR);std::cout<<name<<" GPU eta/lambda PASS\n";
 }
 for(const char* name:{"level_odd","level_pair","level_tilt","level_reverse","level_zero_rate"}){
  auto bytes=Read(std::string(".codex/local-test/t522-solver/")+name+".bin");const auto* expected=reinterpret_cast<const uint32_t*>(bytes.data());
  std::vector<uint32_t> base(129),state(6400),work(12288),output(21000);
  for(size_t c=0;c<128;++c){const auto material=expected[3+c],water=expected[131+c];state[4096+128*2+c]=state[4096+128*10+c]=state[4096+128*11+c]=material;state[4096+128*6+c]=water;state[4096+128*12+c]=state[4096+128*13+c]=material+water;}
  const std::vector<uint32_t>* data[]={&base,&state,&work,&output};for(unsigned i=0;i<4;++i){glBindBuffer(GL_SHADER_STORAGE_BUFFER,buffers[i]);glBufferData(GL_SHADER_STORAGE_BUFFER,data[i]->size()*4,data[i]->data(),GL_DYNAMIC_COPY);glBindBufferBase(GL_SHADER_STORAGE_BUFFER,i,buffers[i]);}
  glUniform1ui(glGetUniformLocation(program,"uMode"),1);glUniform1i(glGetUniformLocation(program,"uTangent"),int32_t(expected[0]));glUniform1ui(glGetUniformLocation(program,"uM3WaterRate"),expected[1]);glUniform1ui(glGetUniformLocation(program,"uM3EventCapacity"),255);glUniform1ui(glGetUniformLocation(program,"uM3Role"),1);
  glDispatchCompute(1,1,1);glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT|GL_SHADER_STORAGE_BARRIER_BIT);glBindBuffer(GL_COPY_READ_BUFFER,buffers[3]);auto result=static_cast<const uint32_t*>(glMapBufferRange(GL_COPY_READ_BUFFER,0,output.size()*4,GL_MAP_READ_BIT));assert(result&&result[20000]==1&&result[20001]==1&&result[20002]==1&&result[20003]==expected[2]);
  for(size_t row=0;row<expected[2];++row){assert(result[row*64+20]==expected[387+row*3]&&result[row*64+21]==expected[388+row*3]&&result[row*64+63]==expected[389+row*3]);assert(result[row*64+1]==4&&result[row*64+6]==19);}
  assert(glUnmapBuffer(GL_COPY_READ_BUFFER));glBindBuffer(GL_COPY_READ_BUFFER,buffers[1]);auto actual=static_cast<const uint32_t*>(glMapBufferRange(GL_COPY_READ_BUFFER,0,state.size()*4,GL_MAP_READ_BIT));assert(actual);for(size_t c=0;c<128;++c)assert(actual[4096+128*6+c]==expected[259+c]);assert(glUnmapBuffer(GL_COPY_READ_BUFFER));assert(glGetError()==GL_NO_ERROR);std::cout<<name<<" GPU40-round target/ordered event/stock PASS\n";
 }
 glDeleteBuffers(4,buffers);glDeleteProgram(program);eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);eglDestroyContext(display,context);eglDestroySurface(display,surface);eglTerminate(display);
}
