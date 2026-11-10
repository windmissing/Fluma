#include <array>
#include <cstring>
#include <fstream>
#include <sstream>

static std::string ReadShader(const char* path){
 std::ifstream file(path);assert(file);std::ostringstream content;content<<file.rdbuf();return content.str();
}
static GLuint CompileProbeShader(const std::string& shader){
 const auto s=glCreateShader(GL_COMPUTE_SHADER);const char* text=shader.c_str();glShaderSource(s,1,&text,nullptr);glCompileShader(s);
 GLint ok;glGetShaderiv(s,GL_COMPILE_STATUS,&ok);
 if(!ok){char log[4096]{};auto info=reinterpret_cast<PFNGLGETSHADERINFOLOGPROC>(eglGetProcAddress("glGetShaderInfoLog"));if(info)info(s,sizeof(log),nullptr,log);std::cerr<<log<<std::endl;}
 assert(ok);
 auto p=glCreateProgram();glAttachShader(p,s);glLinkProgram(p);glDeleteShader(s);glGetProgramiv(p,GL_LINK_STATUS,&ok);assert(ok);
 return p;
}
static unsigned MergeProbe(){
 std::string shader="#version 310 es\nprecision highp int;\n";
 shader+=ReadShader("entry/src/main/cpp/gpu_resident/integer_primitives.glsl");
 shader+=ReadShader("entry/src/main/cpp/gpu_resident/m2/layout_generated.glsl");
 shader+=ReadShader("entry/src/main/cpp/gpu_resident/m2/merge_primitives.glsl");
 shader+=ReadShader("test/t452/merge_probe.glsl");
 const auto p=CompileProbeShader(shader);
 std::array<uint32_t,6*36> rows{};
 for(unsigned i=0;i<6;++i){
  auto a=rows.data()+i*36;auto b=a+16;
  a[0]=b[0]=1;a[1]=20;b[1]=21;a[2]=2;b[2]=3;
  a[3]=uint32_t(-3);b[3]=uint32_t(-2);a[4]=uint32_t(-1);b[4]=1;
  a[5]=2;b[5]=0xffffffffu;a[6]=b[6]=1;a[7]=b[7]=1;a[8]=b[8]=2;a[10]=3;b[10]=4;
  a[32]=1;a[34]=2;a[35]=1;
  if(i==1)b[8]=3;
  if(i==2)b[0]=122334;
  if(i==3)a[32]=0;
  if(i==4){a[0]=b[0]=122333;a[3]=b[3]=1073741824;a[4]=b[4]=uint32_t(-1073741824);}
  if(i==5){a[5]=0xffffffffu;b[5]=2;}
 }
 GLuint buffers[2];glGenBuffers(2,buffers);
 glBindBuffer(GL_SHADER_STORAGE_BUFFER,buffers[0]);glBufferData(GL_SHADER_STORAGE_BUFFER,sizeof(rows),rows.data(),GL_STATIC_DRAW);
 glBindBuffer(GL_SHADER_STORAGE_BUFFER,buffers[1]);glBufferData(GL_SHADER_STORAGE_BUFFER,6*33*4,nullptr,GL_DYNAMIC_COPY);
 glBindBufferBase(GL_SHADER_STORAGE_BUFFER,0,buffers[0]);glBindBufferBase(GL_SHADER_STORAGE_BUFFER,1,buffers[1]);
 glUseProgram(p);glDispatchCompute(1,1,1);glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
 const auto* words=static_cast<const uint32_t*>(glMapBufferRange(GL_SHADER_STORAGE_BUFFER,0,6*33*4,GL_MAP_READ_BIT));assert(words);
 for(unsigned i=0;i<6;++i)assert(words[i*33]==uint32_t(i==0||i==4));
 const auto* m=words+1;const auto* b=words+17;
 assert(m[0]==2&&m[1]==20&&m[2]==2&&m[3]==uint32_t(-2)&&m[4]==0&&m[5]==2&&m[10]==3&&m[14]==1);
 for(unsigned f=0;f<16;++f)assert(b[f]==(f==10?4u:0u));
 m=words+4*33+1;assert(m[0]==244666&&m[3]==1073741824&&m[4]==uint32_t(-1073741824));
 assert(glUnmapBuffer(GL_SHADER_STORAGE_BUFFER)==GL_TRUE);assert(glGetError()==GL_NO_ERROR);
 glDeleteBuffers(2,buffers);glDeleteProgram(p);
 return 6;
}

static unsigned IndexProbe(){
 std::string shader="#version 310 es\nprecision highp int;\n";
 shader+=ReadShader("entry/src/main/cpp/gpu_resident/integer_primitives.glsl");
 shader+=ReadShader("entry/src/main/cpp/gpu_resident/m2/layout_generated.glsl");
 shader+=ReadShader("entry/src/main/cpp/gpu_resident/m2/merge_primitives.glsl");
 shader+=ReadShader("test/t452/index_prefix.glsl");
 shader+=ReadShader("entry/src/main/cpp/gpu_resident/m2/pair_index.glsl");
 shader+=ReadShader("entry/src/main/cpp/gpu_resident/m2/source_row.glsl");
 shader+=ReadShader("test/t452/index_probe.glsl");
 auto p=CompileProbeShader(shader);
 std::array<uint32_t,192*16> rows{};
 const uint32_t ids[]={1,99,5,2,0xffffffffu};
 const uint32_t ys[]={100,101,100,125,120};
 for(unsigned i=1;i<=5;++i){
  auto* slot=rows.data()+i*16;
  slot[0]=1;slot[1]=32;slot[2]=ys[i-1];slot[5]=ids[i-1];slot[6]=1;
  slot[7]=1;slot[8]=i==3?2:1;slot[10]=1;
 }
 GLuint buffers[4];glGenBuffers(4,buffers);
 glBindBuffer(GL_SHADER_STORAGE_BUFFER,buffers[0]);glBufferData(GL_SHADER_STORAGE_BUFFER,sizeof(rows),rows.data(),GL_DYNAMIC_COPY);
 glBindBuffer(GL_SHADER_STORAGE_BUFFER,buffers[1]);glBufferData(GL_SHADER_STORAGE_BUFFER,262*4,nullptr,GL_DYNAMIC_COPY);
 glBindBuffer(GL_SHADER_STORAGE_BUFFER,buffers[2]);glBufferData(GL_SHADER_STORAGE_BUFFER,512*4,nullptr,GL_DYNAMIC_COPY);
 glBindBuffer(GL_SHADER_STORAGE_BUFFER,buffers[3]);glBufferData(GL_SHADER_STORAGE_BUFFER,256*4,nullptr,GL_DYNAMIC_COPY);
 for(GLuint i=0;i<4;++i)glBindBufferBase(GL_SHADER_STORAGE_BUFFER,i,buffers[i]);
 glUseProgram(p);glDispatchCompute(1,1,1);glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
 glBindBuffer(GL_SHADER_STORAGE_BUFFER,buffers[1]);
 const auto* words=static_cast<const uint32_t*>(glMapBufferRange(GL_SHADER_STORAGE_BUFFER,0,262*4,GL_MAP_READ_BIT));assert(words);
 assert(words[0]==1&&words[1]==4); // least unsigned idB is deliberately not nearest y
 assert(words[2]==1&&words[3]==2); // retired b no longer participates
 const uint32_t expected[]={1,2,5,3,0,4};
 for(unsigned i=0;i<256;++i)assert(words[4+i]==(i<6?expected[i]:i));
 assert(words[260]==1&&words[261]==1);
 assert(glUnmapBuffer(GL_SHADER_STORAGE_BUFFER)==GL_TRUE);assert(glGetError()==GL_NO_ERROR);
 glDeleteBuffers(4,buffers);glDeleteProgram(p);
 return 5;
}
