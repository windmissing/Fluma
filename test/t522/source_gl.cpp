#include <cstdint>
#include <cstring>
#include "../../entry/src/main/cpp/gpu_resident/m3a/shader_variant.h"
#include "../../entry/src/main/cpp/gpu_resident/m3a/profiles_generated.h"
#include "../../entry/src/main/cpp/gpu_resident/angles_generated.h"
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
 std::cout.setf(std::ios::unitbuf);
 const std::string scenario=argc>1?argv[1]:"bulk";
 using F=EGLDisplay(EGLAPIENTRYP)(EGLenum,void*,const EGLint*);PROC(F,eglGetPlatformDisplayEXT);
 const EGLint displayArgs[]={0x3203,0x3450,0x3209,0x3487,EGL_NONE};auto display=f(0x3202,nullptr,displayArgs);assert(eglInitialize(display,nullptr,nullptr));
 const EGLint attributes[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_NONE};EGLConfig config;EGLint count;assert(eglChooseConfig(display,attributes,&config,1,&count)&&count);
 const EGLint contextArgs[]={EGL_CONTEXT_CLIENT_VERSION,3,0x30FB,1,EGL_NONE},surfaceArgs[]={EGL_WIDTH,8,EGL_HEIGHT,8,EGL_NONE};auto context=eglCreateContext(display,config,EGL_NO_CONTEXT,contextArgs);auto surface=eglCreatePbufferSurface(display,config,surfaceArgs);assert(eglMakeCurrent(display,surface,surface,context));
 std::string shader=Read(".codex/local-test/t522-solver/source-driver.glsl");
 GLuint sh=glCreateShader(GL_COMPUTE_SHADER);const char* src=shader.c_str();glShaderSource(sh,1,&src,nullptr);glCompileShader(sh);GLint ok;glGetShaderiv(sh,GL_COMPILE_STATUS,&ok);if(!ok){char log[8192];glGetShaderInfoLog(sh,sizeof(log),nullptr,log);std::cerr<<log;return 1;}auto program=glCreateProgram();glAttachShader(program,sh);glLinkProgram(program);glGetProgramiv(program,GL_LINK_STATUS,&ok);assert(ok);glDeleteShader(sh);glUseProgram(program);
 constexpr uint32_t H=6144,COL=4096,SCRATCH=4096,INPUT=16384,DEDUP=32768,RAW=DEDUP+4096*64,META=RAW+4096*512,LEDGER=META+4096*32,ORIGINS=LEDGER+4096*16;
 constexpr uint32_t ANGLES=ORIGINS+8192;
 std::vector<uint32_t> base(6400),blank(6400),work(ANGLES+61444),events(42680*64),completion(256),publication(16*192+20*64+2048+256);
 for(unsigned i=0;i<61444;++i)work[ANGLES+i]=grs::kAngleWords[i];
 for(uint32_t c=0;c<128;++c){base[COL+6*128+c]=1048576;base[COL+12*128+c]=base[COL+13*128+c]=1048576;}
 auto header=[&](uint32_t field,uint32_t value){base[H+field]=value;};
 header(0,1213420103);header(1,4);header(2,256);header(4,1);header(6,2);header(8,3);header(10,4);header(20,1);header(22,1);header(39,1);header(40,1);header(43,192);header(44,64);header(58,128*1048576);header(66,128*1048576);header(84,1);header(112,128*1048576);header(122,1);header(123,65536);header(124,2);header(125,2);header(145,1);header(146,7);header(156,512);header(160,1);
 const uint32_t versions[]={1,1,1,3,4,4,4,15,0};for(unsigned i=0;i<9;++i)header(24+i,versions[i]);
 auto input=[&](uint32_t field,uint32_t value){work[INPUT+field]=value;};
 input(0,1129534023);input(1,4);input(2,2);input(4,1);input(6,1);input(8,1);input(10,1);input(12,1);input(14,1);input(15,1);input(24,1);input(28,1);
 auto binary=[&](uint32_t at,double value){uint64_t bits;std::memcpy(&bits,&value,8);input(at,uint32_t(bits));input(at+1,uint32_t(bits>>32));};
 binary(16,0.5);binary(18,0.5);binary(20,0.0);binary(32,122333.0/1048576);binary(36,122334.0/1048576);
 uint32_t expectedMass=244667,expectedBirths=3,expectedTrials=1;
 if(scenario=="retry"){
   for(uint32_t c=0;c<128;++c)binary(32+4*c,244666.0/1048576);
   expectedMass=122333*128;expectedBirths=128;expectedTrials=2;
 }else if(scenario=="sand"||scenario=="water-direct"||scenario=="water-flight"){
   for(uint32_t i=0;i<512;++i)input(32+i,0);
   const bool sand=scenario=="sand";
   input(2,sand?0:1);binary(20,122334.0/1048576);
   if(scenario=="water-flight")binary(18,2.0);
   expectedMass=122334;expectedBirths=sand?2:scenario=="water-flight"?1:0;expectedTrials=0;
 }else if(scenario=="seed-resolution"){
   for(uint32_t c=0;c<128;++c){base[COL+6*128+c]=c==1?1:0;base[COL+12*128+c]=base[COL+13*128+c]=c==1?1:0;binary(32+4*c,c==1?1.0/1048576:0);}
   header(58,1);header(66,1);header(112,1);
 }else if(scenario=="zero-source"||scenario=="zero-max-physics"){
   for(uint32_t i=0;i<512;++i)input(32+i,0);input(2,0);binary(20,0);
   if(scenario=="zero-max-physics"){header(16,UINT32_MAX);header(17,UINT32_MAX);header(118,UINT32_MAX);header(119,UINT32_MAX);}
 }else assert(scenario=="bulk"||scenario=="corrupt-replay"||scenario=="quantum"||scenario=="root"||scenario=="root-offset"||scenario=="root-clamped"||scenario=="root-full");
 GLuint buffers[8];glGenBuffers(8,buffers);
 const std::vector<uint32_t>* data[]={&base,&blank,&blank,&blank,&work,&events,&publication,&completion};
 for(unsigned i=0;i<8;++i){glBindBuffer(GL_SHADER_STORAGE_BUFFER,buffers[i]);glBufferData(GL_SHADER_STORAGE_BUFFER,data[i]->size()*4,data[i]->data(),GL_DYNAMIC_COPY);}
 auto uniform=[&](const char* name,uint32_t value){glUniform1ui(glGetUniformLocation(program,name),value);};
 const std::pair<const char*,uint32_t> uniforms[]={{"uK",192},{"uKw",64},{"uWater",3072},{"uColumn",COL},{"uHeader",H},{"uM3Words",6400},{"uM3Scratch",SCRATCH},{"uM3SourceRows",5153},{"uM3Input",INPUT},{"uM3Payload",INPUT+32},{"uM3Dedup",DEDUP},{"uM3RawDedup",RAW},{"uM3SourceMeta",META},{"uM3SourceEvents",LEDGER},{"uM3OriginScratch",ORIGINS},{"uM3Uploaded",2176},{"uM3ExpectedState",24576},{"uM3CandidateBank",1},{"uM3SpareBank",2},{"uM3Index0",2048},{"uM3Index1",2304},{"uM3Settle",65536}};
 for(auto u:uniforms)uniform(u.first,u.second);glUniform1i(glGetUniformLocation(program,"uM3Cos"),1048576);
 for(unsigned pass=0;pass<72;++pass){
   unsigned in=0,out=1,event=5;
   const bool replay=pass>=2&&pass<66&&(pass-2)%8==6;
   if(replay){in=1;out=2;}else if(pass>=2&&pass<66&&(pass-2)%8==5)out=2;
   if(pass==67)out=2;
   if(pass==68){in=1;out=3;}else if(pass>=69){out=3;event=pass==71?7:6;}
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER,0,buffers[in]);glBindBufferBase(GL_SHADER_STORAGE_BUFFER,1,buffers[out]);glBindBufferBase(GL_SHADER_STORAGE_BUFFER,2,buffers[4]);glBindBufferBase(GL_SHADER_STORAGE_BUFFER,3,buffers[event]);
   uniform("uPass",pass);uniform("uM3Trial",pass>=2&&pass<66?(pass-2)/8:0);uniform("uM3ReplayPass",replay?1:0);
   if(scenario=="corrupt-replay"&&pass==8){uint32_t bad=1;glBindBuffer(GL_SHADER_STORAGE_BUFFER,buffers[5]);glBufferSubData(GL_SHADER_STORAGE_BUFFER,43*4,4,&bad);}
   glDispatchCompute(1,1,1);glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT|GL_BUFFER_UPDATE_BARRIER_BIT);assert(glGetError()==GL_NO_ERROR);
 }
 glBindBuffer(GL_COPY_READ_BUFFER,buffers[4]);auto result=static_cast<const uint32_t*>(glMapBufferRange(GL_COPY_READ_BUFFER,0,work.size()*4,GL_MAP_READ_BIT));assert(result);
 std::cout<<"SOURCE error="<<result[0]<<" pass="<<result[2]<<" row="<<result[3]<<" word="<<result[4]<<" rows="<<result[SCRATCH+16]<<" trials="<<result[SCRATCH+57]<<"\n";
 const bool passed=result[0]==0,corruptCaught=result[0]==31&&result[2]==9&&result[4]==43;assert(glUnmapBuffer(GL_COPY_READ_BUFFER));
 if(scenario=="corrupt-replay"){
   assert(corruptCaught);glBindBuffer(GL_COPY_READ_BUFFER,buffers[3]);auto rejected=static_cast<const uint32_t*>(glMapBufferRange(GL_COPY_READ_BUFFER,0,blank.size()*4,GL_MAP_READ_BIT));assert(rejected);for(size_t i=0;i<blank.size();++i)assert(rejected[i]==0);assert(glUnmapBuffer(GL_COPY_READ_BUFFER));
   std::cout<<"T522 corrupt trial event rejected before private promotion PASS\n";
   glDeleteBuffers(8,buffers);glDeleteProgram(program);eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);eglDestroyContext(display,context);eglDestroySurface(display,surface);eglTerminate(display);return 0;
 }
 if(!passed)return 2;
 glBindBuffer(GL_COPY_READ_BUFFER,buffers[7]);auto tail=static_cast<const uint32_t*>(glMapBufferRange(GL_COPY_READ_BUFFER,0,1024,GL_MAP_READ_BIT));assert(tail);
 if(scenario=="zero-source"||scenario=="zero-max-physics"||scenario=="seed-resolution"){
   assert(tail[28]==2&&tail[18]==0&&tail[19]==0&&tail[64]==(scenario=="seed-resolution"?131076u:65539u)&&tail[106]==1&&tail[97]==72&&tail[99]==32767);
   if(scenario=="seed-resolution")assert(tail[176]==1&&tail[61]==0);
   if(scenario=="zero-max-physics")assert(tail[20]==UINT32_MAX&&tail[21]==UINT32_MAX);
   else assert(tail[20]==0&&tail[21]==0);
   assert(glUnmapBuffer(GL_COPY_READ_BUFFER));std::cout<<"T522 SOURCE72 rejection/rollback/no physics overflow "<<scenario<<" PASS\n";
   glDeleteBuffers(8,buffers);glDeleteProgram(program);eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);eglDestroyContext(display,context);eglDestroySurface(display,surface);eglTerminate(display);return 0;
 }
 assert(tail[0]==1381192263&&tail[1]==4&&tail[2]==256&&tail[3]==3235823838u&&tail[28]==1&&tail[97]==72&&tail[99]==32767&&tail[110]==72&&tail[111]==72&&tail[112]==2176&&tail[173]==1&&tail[174]==7&&tail[188]==1);
 for(unsigned i=189;i<256;++i)assert(tail[i]==0);assert(glUnmapBuffer(GL_COPY_READ_BUFFER));
 glBindBuffer(GL_COPY_READ_BUFFER,buffers[3]);auto actual=static_cast<const uint32_t*>(glMapBufferRange(GL_COPY_READ_BUFFER,0,base.size()*4,GL_MAP_READ_BIT));assert(actual);
 const bool isWater=scenario=="water-direct"||scenario=="water-flight";
 assert(actual[H+14]==1&&actual[H+16]==1&&actual[H+(isWater?62:60)]==expectedMass&&actual[H+148]==expectedTrials&&actual[H+(isWater?40:39)]==expectedBirths+1&&actual[H+33]==1);
 assert(actual[H+56]==(isWater?0:expectedMass)&&actual[H+58]==128*1048576+(isWater?expectedMass:0));
 if(scenario=="bulk")assert(actual[0]==122333&&actual[1]==122333&&actual[2]==1);
 if(scenario=="retry")for(uint32_t c=0;c<128;++c)assert(actual[COL+8*128+c]==122333);
 assert(glUnmapBuffer(GL_COPY_READ_BUFFER));
 std::cout<<"T522 actual GPU SOURCE "<<scenario<<" SOURCE72, independent trial/root SPARE replay, ledger, publication and V4 seal PASS\n";
 if(scenario=="quantum"){
   glDeleteProgram(program);shader=Read(".codex/local-test/t522-solver/occupied.glsl");src=shader.c_str();sh=glCreateShader(GL_COMPUTE_SHADER);glShaderSource(sh,1,&src,nullptr);glCompileShader(sh);glGetShaderiv(sh,GL_COMPILE_STATUS,&ok);assert(ok);program=glCreateProgram();glAttachShader(program,sh);glLinkProgram(program);glGetProgramiv(program,GL_LINK_STATUS,&ok);assert(ok);glDeleteShader(sh);glUseProgram(program);
   for(auto u:uniforms)uniform(u.first,u.second);
   const std::pair<const char*,uint32_t> quniforms[]={{"uWords",6400},{"uPad",256},{"uSandIndex",20000},{"uWaterIndex",20256},{"uArrivalIndex",20320},{"uEdges",20480},{"uFrozen",23000},{"uRows",23600},{"uReplayFrozen",24000},{"uSpare",24576},{"uAngles",ANGLES},{"uSourceEvents",LEDGER},{"uEventCapacity",10622},{"uM3EventCapacity",10622},{"uM3Role",1},{"uM3Ordinal",1},{"uM3BaseSourceLo",1},{"uM3ProposedPhysicsLo",2},{"uQuantumOrdinal",1},{"uGravity",120},{"uSettle",65536},{"uKWet",120},{"uSoft",605393},{"uM3WaterRate",8388608}};
   for(auto u:quniforms)uniform(u.first,u.second);
   auto dispatch=[&](uint32_t stage,uint32_t n,uint32_t out){
     glBindBufferBase(GL_SHADER_STORAGE_BUFFER,0,buffers[3]);glBindBufferBase(GL_SHADER_STORAGE_BUFFER,1,buffers[out]);glBindBufferBase(GL_SHADER_STORAGE_BUFFER,2,buffers[4]);glBindBufferBase(GL_SHADER_STORAGE_BUFFER,3,buffers[5]);
     uniform("uPass",stage);glDispatchCompute((n+63)/64,1,1);glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT|GL_BUFFER_UPDATE_BARRIER_BIT);assert(glGetError()==GL_NO_ERROR);
   };
   dispatch(30,1,1);
   uint32_t submitted=0;
   for(const auto& pass:grs::m3a::kPasses192_64){
     if(pass.role!=1)continue;
     using grs::m3a::Opcode;
     if(pass.opcode==Opcode::SCOPE_INIT||pass.opcode==Opcode::SCOPE_FINALIZE)continue;
     uint32_t stage=pass.nativeStage;
     if(pass.opcode==Opcode::WATER_TARGET_40)stage=2;
     if(pass.opcode==Opcode::WATER_MATCH_ORDERED)stage=3;
     if(pass.opcode==Opcode::WATER_PROJECTION_AUDIT)stage=4;
     if(stage==24)continue; // Root metadata/five-role finalization is separate.
     uniform("uSortK",pass.sortK);uniform("uSortJ",pass.sortJ);uniform("uColor",pass.color);
     dispatch(stage,pass.count,stage==22||stage==23?2:1);++submitted;
   }
   glBindBuffer(GL_COPY_READ_BUFFER,buffers[4]);auto q=static_cast<const uint32_t*>(glMapBufferRange(GL_COPY_READ_BUFFER,0,work.size()*4,GL_MAP_READ_BIT));assert(q);
   std::cout<<"occupied core submitted="<<submitted<<" error="<<q[0]<<" pass="<<q[2]<<" row="<<q[3]<<" replay="<<q[17]<<" compare="<<q[19]<<" expected="<<q[26]<<" actual="<<q[27]<<"\n";
   const bool quantumPassed=q[0]==0&&q[17]==1;assert(glUnmapBuffer(GL_COPY_READ_BUFFER));if(!quantumPassed)return 3;
   std::cout<<"T522 actual GPU occupied motion core/global water/independent SPARE replay PASS (one scope, not full QUANTUM root)\n";
 }
#include "quantum_root.inc"
 glDeleteBuffers(8,buffers);glDeleteProgram(program);eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);eglDestroyContext(display,context);eglDestroySurface(display,surface);eglTerminate(display);
}
