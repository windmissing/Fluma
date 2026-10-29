#include <cstdint>
#include <EGL/egl.h>
#include <GLES3/gl31.h>
#include <vector>
#include <array>
#include <string>
#include <fstream>
#include <iostream>
#include <cstring>
#include <cassert>
using Words=std::vector<uint32_t>;
int main(){
 using Platform=EGLDisplay(EGLAPIENTRYP)(EGLenum,void*,const EGLint*);auto platform=reinterpret_cast<Platform>(eglGetProcAddress("eglGetPlatformDisplayEXT"));assert(platform);
 EGLint args[]={0x3203,0x3450,0x3209,0x3487,EGL_NONE};EGLDisplay d=platform(0x3202,nullptr,args);EGLint major,minor;assert(eglInitialize(d,&major,&minor));EGLConfig cfg;EGLint count;EGLint attr[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_NONE};assert(eglChooseConfig(d,attr,&cfg,1,&count)&&count);assert(eglBindAPI(EGL_OPENGL_ES_API));EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_CONTEXT_MINOR_VERSION,1,EGL_NONE},pa[]={EGL_WIDTH,1,EGL_HEIGHT,1,EGL_NONE};auto ctx=eglCreateContext(d,cfg,EGL_NO_CONTEXT,ca);auto surf=eglCreatePbufferSurface(d,cfg,pa);assert(eglMakeCurrent(d,surf,surf,ctx));
 std::ifstream file(".codex/local-test/t240-source/source_core.comp");std::string text((std::istreambuf_iterator<char>(file)),{});assert(!text.empty());const char* src=text.c_str();GLuint shader=glCreateShader(GL_COMPUTE_SHADER);glShaderSource(shader,1,&src,nullptr);glCompileShader(shader);GLint good;glGetShaderiv(shader,GL_COMPILE_STATUS,&good);if(!good){char log[8192];auto fn=reinterpret_cast<PFNGLGETSHADERINFOLOGPROC>(eglGetProcAddress("glGetShaderInfoLog"));fn(shader,sizeof(log),nullptr,log);std::cerr<<log;return 1;}GLuint program=glCreateProgram();glAttachShader(program,shader);glLinkProgram(program);glGetProgramiv(program,GL_LINK_STATUS,&good);assert(good);glUseProgram(program);
 constexpr uint32_t K=192,Kw=128,water=16*K,col=water+16*Kw,header=col+16*128,words=header+256,spare=96+4096*64;
 GLuint banks[2],workspace,events;glGenBuffers(2,banks);glGenBuffers(1,&workspace);glGenBuffers(1,&events);GLuint publication,resultBuffer;glGenBuffers(1,&publication);glGenBuffers(1,&resultBuffer);
 auto put=[&](GLuint b,const Words& w){glBindBuffer(GL_SHADER_STORAGE_BUFFER,b);glBufferData(GL_SHADER_STORAGE_BUFFER,w.size()*4,w.data(),GL_DYNAMIC_COPY);};
 Words initial(words);initial[header]=1213420103;initial[header+1]=2;initial[header+2]=256;initial[header+4]=1;initial[header+20]=initial[header+22]=initial[header+84]=1;initial[header+31]=3;initial[header+39]=initial[header+40]=1;initial[header+43]=K;initial[header+44]=Kw;
 put(banks[0],initial);put(banks[1],initial);put(workspace,Words(spare+words+4096*16));put(events,Words(64*(K+1)));put(publication,Words(16*K+20*Kw+16*128+256));put(resultBuffer,Words(256));
 auto uniform=[&](const char* n,uint32_t v){glUniform1ui(glGetUniformLocation(program,n),v);};uniform("uK",K);uniform("uKw",Kw);uniform("uWater",water);uniform("uColumn",col);uniform("uHeader",header);uniform("uWords",words);uniform("uEmitQ",1048576);glUniform2ui(glGetUniformLocation(program,"uNonce"),1,0);glUniform2ui(glGetUniformLocation(program,"uFrame"),1,0);
 auto read=[&](GLuint b,uint32_t start,uint32_t n){glBindBuffer(GL_COPY_READ_BUFFER,b);void* p=glMapBufferRange(GL_COPY_READ_BUFFER,start*4,n*4,GL_MAP_READ_BIT);assert(p);Words w(n);std::memcpy(w.data(),p,n*4);assert(glUnmapBuffer(GL_COPY_READ_BUFFER));return w;};
 uint32_t seq=0;auto command=[&](uint32_t kind,double x,double y,double q){std::array<uint32_t,32> a{};a[0]=1129534023;a[1]=2;a[2]=kind;a[4]=1;a[6]=++seq;a[8]=a[10]=a[12]=a[14]=a[15]=a[24]=a[27]=a[28]=1;std::memcpy(a.data()+16,&x,8);std::memcpy(a.data()+18,&y,8);std::memcpy(a.data()+20,&q,8);return a;};
 auto run=[&](const std::array<uint32_t,32>& a,bool corrupt=false,bool sealed=false,int fault=0){glBindBuffer(GL_SHADER_STORAGE_BUFFER,workspace);glBufferSubData(GL_SHADER_STORAGE_BUFFER,0,128,a.data());for(int pass=0;pass<(sealed?8:4);pass++){glBindBufferBase(GL_SHADER_STORAGE_BUFFER,0,banks[0]);glBindBufferBase(GL_SHADER_STORAGE_BUFFER,1,banks[1]);glBindBufferBase(GL_SHADER_STORAGE_BUFFER,2,workspace);glBindBufferBase(GL_SHADER_STORAGE_BUFFER,3,pass==5||pass==6?publication:pass==7?resultBuffer:events);uniform("uPass",pass);if(pass==4&&fault>0&&fault<7){GLuint b=fault==5?workspace:banks[1];uint32_t at=fault==1?water+5*Kw:fault==2?water+12*Kw:fault==3?header+68:fault==4?col+14*128:fault==5?96+32+3:header+132;uint32_t bad=fault==1?0:77;glBindBuffer(GL_SHADER_STORAGE_BUFFER,b);glBufferSubData(GL_SHADER_STORAGE_BUFFER,at*4,4,&bad);}if(pass==6&&fault==7){uint32_t bad=77;glBindBuffer(GL_SHADER_STORAGE_BUFFER,publication);glBufferSubData(GL_SHADER_STORAGE_BUFFER,(16*K+20*Kw)*4,4,&bad);}if(pass==3&&corrupt){uint32_t forged=0;glBindBuffer(GL_SHADER_STORAGE_BUFFER,events);glBufferSubData(GL_SHADER_STORAGE_BUFFER,23*4,4,&forged);}glDispatchCompute((words+63)/64,1,1);glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT|GL_BUFFER_UPDATE_BARRIER_BIT);}auto result=read(workspace,32,64);assert(glGetError()==GL_NO_ERROR);std::cout<<"ROOT seq="<<a[6]<<" outcome="<<result[32]<<" reason="<<result[33]<<" replayAudit="<<result[44]<<std::endl;if(result[32]!=4){assert(result[44]==1);std::swap(banks[0],banks[1]);}return result;};
 glUniform2ui(glGetUniformLocation(program,"uControl"),1,0);uniform("uBaseBank",0);uniform("uPublicationSlot",0);uniform("uUploaded",128);
 int passed=0;auto flight=command(1,64.5,100,.25);auto r=run(flight);assert(r[32]==1&&r[3]==262144&&r[30]==2);auto state=read(banks[0],0,words);assert(state[water]==262144&&state[col+7*128+64]==262144&&state[col+6*128+64]==0&&state[header+58]==262144&&state[header+114]==262144);passed++;
 auto base=state;r=run(flight);assert(r[32]==3&&r[2]==1&&read(banks[0],0,words)==base);passed++;
 auto conflict=flight;conflict[26]=7;r=run(conflict);assert(r[32]==2&&r[1]==8);passed++;
 r=run(command(1,20.5,0,.5));state=read(banks[0],0,words);assert(r[32]==1&&r[30]==1&&state[col+6*128+20]==524288&&state[col+12*128+20]==524288&&state[header+112]==524288&&state[header+42]==1);passed++;
 r=run(command(0,70.5,100,.25));state=read(banks[0],0,words);assert(r[32]==1&&r[6]==2&&state[header+41]==2&&state[col+8*128+70]==262144);passed++;
 base=state;r=run(command(1,64.5,100,.125),true);assert(r[32]==4&&r[33]==16&&read(banks[0],0,words)==base);passed++;
 auto zero=command(1,64.5,100,0);r=run(zero);assert(r[32]==2&&r[1]==3);passed++;
 auto unsupported=command(2,64.5,100,.25);r=run(unsupported);assert(r[32]==2&&r[1]==1);passed++;
 auto negative=command(1,-1,100,.25);r=run(negative);assert(r[32]==2&&r[1]==2);passed++;
 state=read(banks[0],0,words);state[header+40]=0xffffffffu;put(banks[0],state);
 r=run(command(1,90.5,100,.125));state=read(banks[0],0,words);assert(r[32]==1&&r[7]==0xffffffffu&&r[9]==0&&state[header+127]==1);passed++;
 base=state;r=run(command(1,90.5,100,.125));assert(r[32]==2&&r[1]==7);state=read(banks[0],0,words);assert(state[header+58]==base[header+58]&&state[header+40]==0);passed++;
 r=run(command(1,20.5,0,.125));assert(r[32]==1&&r[30]==1);passed++;
 put(banks[0],initial);put(workspace,Words(spare+words+4096*16));auto fullCommand=command(1,64.5,100,.25);r=run(fullCommand,false,true);auto completion=read(resultBuffer,0,256),sourceEvent=read(workspace,spare+words,16);assert(completion[0]==1381192263&&completion[1]==2&&completion[2]==256&&completion[3]==3235823838u&&completion[28]==1&&completion[49]==3&&completion[97]==255&&completion[99]==1023&&completion[61]==262144&&completion[75]==262144&&completion[144]==262144);assert(sourceEvent[0]==fullCommand[6]&&sourceEvent[4]==1&&sourceEvent[6]==262144&&sourceEvent[8]==1);auto packed=read(publication,0,16*K+20*Kw+16*128+256);assert(packed[16*K]==262144&&packed[16*K+20*Kw+3*128+64]==262144);passed++;
 for(int fault=1;fault<=7;fault++){put(banks[0],initial);put(workspace,Words(spare+words+4096*16));auto injected=command(1,64.5,100,.25);auto failed=run(injected,false,true,fault);assert(failed[32]==4&&failed[33]==15);auto tail=read(resultBuffer,0,256);assert(tail[28]==4&&tail[99]!=1023);assert(read(banks[0],0,words)==initial);passed++;}
 std::cout<<"{\"revision\":\"T240_GPU_SOURCE_CORE_V1\",\"passed\":"<<passed<<",\"failed\":0,\"execution\":\"PC_SWIFTSHADER\",\"completion256\":\"GPU_SOURCE_AUDIT_WITH_SEPARATE_NATIVE_TRANSPORT_TEST\",\"device\":\"NOT_RUN\"}\n";
 glDeleteProgram(program);glDeleteShader(shader);glDeleteBuffers(2,banks);glDeleteBuffers(1,&workspace);glDeleteBuffers(1,&events);eglMakeCurrent(d,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);eglDestroySurface(d,surf);eglDestroyContext(d,ctx);eglTerminate(d);
}
