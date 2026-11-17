#include "transport.h"
#include "../angles_generated.h"
#include "../program_cache/program_factory.h"
#include <chrono>
#include <cstring>
#include <limits>
namespace grs::g9dm {
namespace {
uint64_t Now(){return uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());}
std::array<uint32_t,8> ParseHash(const char* hex){std::array<uint32_t,8> result{};auto digit=[](char c){return uint32_t(c<='9'?c-'0':c-'a'+10);};for(size_t i=0;i<32;++i){uint32_t byte=(digit(hex[2*i])<<4)|digit(hex[2*i+1]);result[i/4]|=byte<<(8*(i%4));}return result;}
}
bool Transport::Current()const{return context_!=EGL_NO_CONTEXT&&thread_==std::this_thread::get_id()&&eglGetCurrentContext()==context_&&eglGetCurrentDisplay()==display_&&eglGetCurrentSurface(EGL_DRAW)==surface_;}
bool Transport::Good(){const auto gl=glGetError();const auto egl=eglGetError();if(gl&&!firstGl_)firstGl_=gl;if(egl!=EGL_SUCCESS&&!firstEgl_)firstEgl_=egl;return !firstGl_&&!firstEgl_;}
bool Transport::Allowed()const{return Current()&&Now()<=permit_.deadline&&check_&&check_(checkUser_,permit_,Now());}
Result Transport::Denied()const{return !Current()?Result::CONTEXT_LOST:Now()>permit_.deadline?Result::DEADLINE:Result::STALE;}
void Transport::U(const char* n,uint32_t v){glUniform1ui(program_.uniforms.Find(n),v);}
void Transport::I(const char* n,int32_t v){glUniform1i(program_.uniforms.Find(n),v);}
void Transport::P(const char* n,uint64_t v){glUniform2ui(program_.uniforms.Find(n),uint32_t(v),uint32_t(v>>32));}
void Transport::Quad(const char* n,const uint32_t* p){glUniform4uiv(program_.uniforms.Find(n),1,p);}
std::array<uint32_t,8> Transport::ShaderHash()const{return ParseHash(PipelineSourceHash());}
Result Transport::QueryComputeCaps(const ProfileRequirements& need,StageCapabilityV2* out){
 // T564 (problem T560 §2): compute-only eligibility. Called only on the cold
 // path (owner context, create/capability export); per-frame code never
 // queries the driver. Vertex/fragment shader-storage block counts are
 // recorded for the SCENE_ATTR domain but do not gate compute.
 if(!out||!Current())return Result::CONTEXT_LOST;
 StageCapabilityV2 c{};c.context=context_;c.route=2;c.buildGeneration=identity_.context;
 int32_t bad=0;
 bad|=QueryCapabilityI(c,GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS,c.ssboBindings,"COMPUTE",4);
 bad|=QueryCapabilityI(c,GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS,c.computeBlocks,"COMPUTE",4);
 bad|=QueryCapabilityI(c,GL_MAX_COMBINED_SHADER_STORAGE_BLOCKS,c.combinedBlocks,"COMPUTE",4);
 bad|=QueryCapabilityI(c,GL_MAX_COMPUTE_SHARED_MEMORY_SIZE,c.sharedBytes,"COMPUTE",4096);
 bad|=QueryCapabilityI(c,GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS,c.invocations,"COMPUTE",64);
 {
  GLint v[3]={0,0,0};glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE,0,&v[0]);const auto s=int32_t(glGetError());
  c.sizeX=v[0];RecordQuery(c,GL_MAX_COMPUTE_WORK_GROUP_SIZE+0,v[0],s,64,"COMPUTE");bad|=s;
  glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE,1,&v[1]);const auto s1=int32_t(glGetError());
  c.sizeY=v[1];RecordQuery(c,GL_MAX_COMPUTE_WORK_GROUP_SIZE+1,v[1],s1,1,"COMPUTE");bad|=s1;
  glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE,2,&v[2]);const auto s2=int32_t(glGetError());
  c.sizeZ=v[2];RecordQuery(c,GL_MAX_COMPUTE_WORK_GROUP_SIZE+2,v[2],s2,1,"COMPUTE");bad|=s2;
 }
 {
  GLint v=0;glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_COUNT,&v);const auto s=int32_t(glGetError());
  c.countX=v;RecordQuery(c,GL_MAX_COMPUTE_WORK_GROUP_COUNT,v,s,1,"COMPUTE");bad|=s;
 }
 {
  GLint64 v=0;glGetInteger64v(GL_MAX_SHADER_STORAGE_BLOCK_SIZE,&v);const auto s=int32_t(glGetError());
  c.maxBlockBytes=v;RecordQuery(c,GL_MAX_SHADER_STORAGE_BLOCK_SIZE,int64_t(v),s,int32_t(std::max<size_t>(need.stateBytes,std::max(need.workBytes,need.arenaBytes))),"COMPUTE");bad|=s;
 }
 bad|=QueryCapabilityI(c,GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS,c.vertexBlocksQueried,"SCENE_ATTR",0);
 bad|=QueryCapabilityI(c,GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS,c.fragmentBlocksQueried,"SCENE_ATTR",0);
 const auto gl=firstGl_;
 *out=c;
 if(bad||gl){out->computeFailure=bad?"UNSUPPORTED_QUERY":"PRIOR_GL_ERROR";return Result::UNSUPPORTED;}
 if(c.ssboBindings<4||c.computeBlocks<4||c.combinedBlocks<4||c.sharedBytes<4096||c.invocations<64||c.sizeX<64||c.countX<1||c.maxBlockBytes<int64_t(std::max<size_t>(need.stateBytes,std::max(need.workBytes,need.arenaBytes)))){out->computeFailure="COMPUTE_LIMIT";return Result::UNSUPPORTED;}
 out->computeEligible=true;return Result::OK;
}
Result Transport::Create(const Config& config,const ControlIdentity& id,const SeedFileIdentity& seed){
 if(ready_||busy_||!config.Valid()||config.parameterSha!=ParameterHash(config)||!id.run||!id.owner||!id.context)return Result::CALIBRATION_CONFIG_REQUIRED;
 const auto* p=SelectProfile(config.k);if(!p)return Result::UNSUPPORTED;
 context_=eglGetCurrentContext();display_=eglGetCurrentDisplay();surface_=eglGetCurrentSurface(EGL_DRAW);thread_=std::this_thread::get_id();if(!Current())return Result::CONTEXT_LOST;
 GLint bindings=0,blocks=0,shared=0,lanes=0,vertex=0,fragment=0;GLint64 maximum=0;
 glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS,&bindings);glGetIntegerv(GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS,&blocks);glGetIntegerv(GL_MAX_COMPUTE_SHARED_MEMORY_SIZE,&shared);glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS,&lanes);glGetIntegerv(GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS,&vertex);glGetIntegerv(GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS,&fragment);glGetInteger64v(GL_MAX_SHADER_STORAGE_BLOCK_SIZE,&maximum);
 // T564 (problem T560 §2): compute init gates only on compute-stage limits.
 // Vertex/fragment shader-storage counts stay recorded (cold capability
 // export) but no longer gate the compute pipeline; the packet vertex stage
 // now uses integer instance attributes instead of a vertex SSBO.
 if(!Good()||bindings<4||blocks<4||shared<4096||lanes<64||maximum<=0||uint64_t(maximum)<std::max(p->arenaBytes,std::max(p->stateBytes,p->workBytes)))return Result::UNSUPPORTED;
 GLenum error=0;if(!programcache::Build(1,PipelineSource(),PipelineSourceHash(),"G9_SPF_DM_Q20_V2_FOUR_SSBO_INTEGER128",program_.id,error)||!programcache::Reflect(program_.uniforms,program_.id)){if(error&&!firstGl_)firstGl_=error;Release(true);return Result::SHADER_FAILED;}
 profile_=p;config_=config;identity_=id;
 glGenBuffers(GLsizei(buffers_.size()),buffers_.data());
 for(size_t i=0;i<buffers_.size();++i){size_t bytes=i<5?p->stateBytes:i==5?p->workBytes:p->arenaBytes;glBindBuffer(GL_SHADER_STORAGE_BUFFER,buffers_[i]);glBufferData(GL_SHADER_STORAGE_BUFFER,GLsizeiptr(bytes),nullptr,GL_DYNAMIC_COPY);if(!Good()){Release(true);return Result::GL_FAILED;}}
 if(config.fullTrialTrace){glGenBuffers(GLsizei(full_.size()),full_.data());for(auto buffer:full_){glBindBuffer(GL_SHADER_STORAGE_BUFFER,buffer);glBufferData(GL_SHADER_STORAGE_BUFFER,GLsizeiptr(p->arenaBytes),nullptr,GL_DYNAMIC_COPY);if(!Good()){Release(true);return Result::GL_FAILED;}}}
 // Cold raw seed upload is an input file, not a host simulation. The first
 // GPU identity/map/mass audit validates all supplied seed words before use.
 std::vector<uint32_t> state;
 if(seed.state.empty())state.resize(p->stateBytes/4);else{if(seed.state.size()!=p->stateBytes/4||seed.sha!=HashWords(seed.state.data(),seed.state.size())){Release(true);return Result::INVALID;}state=seed.state;}
 config_.seedSha=seed.state.empty()?HashWords(state.data(),state.size()):seed.sha;
 auto put=[&](size_t at,uint64_t n){Put(state.data(),at,n);};
 state[0]=kStateMagic;state[1]=1;state[2]=kProfileCode;put(4,id.run);put(6,id.owner);put(8,id.context);put(10,0);put(12,0);put(14,id.inputEpoch);put(16,0);
 if(seed.state.empty())state[18]=1;
 state[20]=0;state[21]=config.g;state[22]=0;state[23]=config.g;state[24]=config.k;state[25]=128;state[26]=state[27]=64;
 for(size_t i=0;i<8;++i)state[42+i]=config.parameterSha[i];
 state[98]=config.muStart;state[99]=config.muStop;state[100]=config.band;state[101]=config.entrainRate;state[102]=config.shear;state[103]=config.settleTicks;
 for(size_t i=0;i<5;++i){glBindBuffer(GL_SHADER_STORAGE_BUFFER,buffers_[i]);glBufferSubData(GL_SHADER_STORAGE_BUFFER,0,GLsizeiptr(p->stateBytes),state.data());}
 if(!Good()){Release(true);return Result::GL_FAILED;}
 // Read-only physical warmup: ROOT_INIT writes only private WORK controls.
 glUseProgram(program_.id);Input128 warm{};Common(warm,{},{});Bind(kQuantumPasses[0]);U("uOp",uint32_t(Op::ROOT_INIT));U("uPass",0);
 const auto begin=Now();glDispatchCompute(1,1,1);glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT|GL_BUFFER_UPDATE_BARRIER_BIT);glFinish();const auto end=Now();const auto errorWarm=glGetError();programcache::Warmup(program_.id,begin,end,errorWarm);
 if(errorWarm||programcache::Prepared(program_.id)!=GL_NO_ERROR||!Good()){if(errorWarm&&!firstGl_)firstGl_=errorWarm;Release(true);return Result::GL_FAILED;}
 current_=0;ready_=true;return Result::OK;
}
bool Transport::Copy(GLuint from,size_t fromWord,GLuint to,size_t toWord,size_t words){
 if(!Allowed()||from==to||fromWord>SIZE_MAX/4||toWord>SIZE_MAX/4||words>SIZE_MAX/4)return false;
 glBindBuffer(GL_COPY_READ_BUFFER,from);glBindBuffer(GL_COPY_WRITE_BUFFER,to);glCopyBufferSubData(GL_COPY_READ_BUFFER,GL_COPY_WRITE_BUFFER,GLintptr(fromWord*4),GLintptr(toWord*4),GLsizeiptr(words*4));glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT|GL_BUFFER_UPDATE_BARRIER_BIT);return Good()&&Allowed();
}
void Transport::Common(const Input128& input,const std::array<uint32_t,8>& layout,const std::array<uint32_t,8>& shader){
 const auto& p=*profile_;U("uK",p.k);U("uStateWords",uint32_t(p.stateBytes/4));U("uColumn",uint32_t(p.state[3].begin/4));U("uBed",uint32_t(p.state[4].begin/4));U("uFlow",uint32_t(p.state[5].begin/4));U("uPacket",uint32_t(p.state[6].begin/4));U("uSources",uint32_t(p.state[7].begin/4));
 const char* names[]={"uFaces","uColumnStage","uHamilton","uOriginAudit","uIndexes","uInput","uSummary","uControls","uCompletion","uConfig","uRootExtension","uWideLedger","uContacts","uOriginExtension","uStageDiagnostic"};for(size_t i=0;i<15;++i)U(names[i],uint32_t(p.work[i].begin/4));
 P("uNonce",permit_.nonce);P("uRun",permit_.identity.run);P("uOwner",permit_.identity.owner);P("uContext",permit_.identity.context);P("uSurface",permit_.identity.surface);P("uGeometry",permit_.geometry);P("uConfigId",permit_.identity.config);P("uInputGeneration",permit_.identity.inputEpoch);P("uSession",permit_.identity.pageSession);P("uPlan",permit_.plan);P("uBaseCommit",permit_.baseCommit);P("uArenaGeneration",permit_.arenaGeneration);
 U("uCandidateBank",permit_.candidateBank);U("uBaseBank",permit_.baseBank);U("uArena",permit_.arena);U("uRequiredPasses",permit_.kind==1?12:2404);U("uEventCapacity",p.eventCapacity);U("uPathCapacity",p.pathCapacity);U("uPathWord",p.eventCapacity*64);
 const auto angle=int32_t(input[24]);const auto at=size_t(angle+7680)*4;I("uAcceptedAngle",angle);I("uAngleSin",int32_t(kAngleWords[at+1]));I("uAngleCos",int32_t(kAngleWords[at+2]));
 (void)layout;(void)shader;
}
Result Transport::Begin(const Permit& p,const Input128& input,Check check,void* user){
 if(!ready_||busy_||!Current())return Result::WOULD_BLOCK;
 if(p.baseBank!=current_||p.candidateBank>=5||p.baseBank==p.candidateBank||p.kind<1||p.kind>2||int32_t(input[24])< -7680||int32_t(input[24])>7680||!check)return Result::INVALID;
 permit_=p;check_=check;checkUser_=user;if(!Allowed())return Denied();
 std::array<uint32_t,128> config{};config[0]=config_.muStart;config[1]=config_.muStop;config[2]=config_.band;config[3]=config_.entrainRate;config[4]=config_.shear;config[5]=config_.settleTicks;config[6]=config_.revision;config[7]=config_.g;
 for(size_t i=0;i<8;++i){config[16+i]=p.parameterSha[i];config[24+i]=p.layoutSha[i];config[32+i]=p.shaderSha[i];config[40+i]=p.inputSha[i];config[48+i]=config_.seedSha[i];}
 lastInput_=input;lastConfig_=config;
 glBindBuffer(GL_SHADER_STORAGE_BUFFER,buffers_[5]);glBufferSubData(GL_SHADER_STORAGE_BUFFER,GLintptr(profile_->work[5].begin),512,input.data());glBufferSubData(GL_SHADER_STORAGE_BUFFER,GLintptr(profile_->work[9].begin),512,config.data());
 glUseProgram(program_.id);Common(input,p.layoutSha,p.shaderSha);if(!Good())return Result::GL_FAILED;
 evidence_={};tail_={};next_=0;busy_=true;mapped_=false;return Result::OK;
}
void Transport::Bind(const Pass& pass){
 GLuint read=buffers_[1],write=buffers_[permit_.candidateBank],events=buffers_[6];
 if(pass.op==Op::BASE_FREEZE||pass.op==Op::SOURCE_VALIDATE_INPUT){read=buffers_[current_];write=permit_.kind==1?buffers_[permit_.candidateBank]:buffers_[1];}
 if(pass.role==3||pass.role==4)write=buffers_[4];
 if(pass.op==Op::FINAL_ENERGY_CURSORS&&(pass.role==2||pass.role==4)){read=buffers_[7];write=buffers_[6];events=buffers_[1];}
 if(pass.op==Op::SCOPE_SEAL_AND_CHOICE&&pass.role==4){read=buffers_[permit_.candidateBank];write=buffers_[4];}
 if(pass.op==Op::COMPARE_SELECTED_AUTHORITATIVE){read=buffers_[3];write=buffers_[permit_.candidateBank];}
 if(pass.op==Op::SOURCE_AUDIT_REPLAY){read=buffers_[current_];write=buffers_[4];}
 if(pass.op==Op::SOURCE_COMPARE){read=buffers_[permit_.candidateBank];write=buffers_[4];}
 if(pass.op==Op::SOURCE_SEAL||pass.op==Op::ROOT_SEAL){read=buffers_[1];write=buffers_[permit_.candidateBank];}
 glBindBufferBase(GL_SHADER_STORAGE_BUFFER,0,read);glBindBufferBase(GL_SHADER_STORAGE_BUFFER,1,write);glBindBufferBase(GL_SHADER_STORAGE_BUFFER,2,buffers_[5]);glBindBufferBase(GL_SHADER_STORAGE_BUFFER,3,events);
}
bool Transport::Dispatch(const Pass& pass,uint32_t index){
 if(!Allowed())return false;
 // BASE is frozen once; every trial starts from this exact complete copy.
 if(permit_.kind==1&&index==0&&!Copy(buffers_[current_],0,buffers_[1],0,profile_->stateBytes/4))return false;
 if(pass.op==Op::DISCOVERY_COMMIT_SELECTION){
  if(!Copy(buffers_[permit_.candidateBank],0,buffers_[3],0,profile_->stateBytes/4)||!Copy(buffers_[6],0,buffers_[7],0,profile_->arenaBytes/4))return false;
 }
 Bind(pass);U("uOp",uint32_t(pass.op));U("uRole",pass.role);U("uJslot",pass.j);U("uMicro",uint32_t(pass.micro));U("uPass",index);
 if(!Allowed())return false;glDispatchCompute(1,1,1);++evidence_.dispatches;glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT|GL_BUFFER_UPDATE_BARRIER_BIT);++evidence_.barriers;if(!Good())return false;
 if(config_.fullTrialTrace&&pass.role==1&&pass.op==Op::SCOPE_SEAL_AND_CHOICE){uint32_t trial=0;for(uint32_t j=pass.j;j>1;j>>=1)++trial;if(!Copy(buffers_[6],0,full_[trial],0,profile_->arenaBytes/4))return false;}
 if(pass.op==Op::FINAL_ENERGY_CURSORS&&pass.role==2){if(!Copy(buffers_[6],0,buffers_[8+permit_.arena],0,profile_->arenaBytes/4))return false;}
 if(pass.op==Op::SOURCE_COMPARE){if(!Copy(buffers_[6],0,buffers_[7],0,profile_->arenaBytes/4)||!Copy(buffers_[6],0,buffers_[8+permit_.arena],0,profile_->arenaBytes/4))return false;}
 return Allowed();
}
Result Transport::Pump(){
 if(!busy_)return Result::INVALID;if(!Allowed())return Denied();if(mapped_)return Result::OK;
 const uint32_t count=permit_.kind==1?12:2404;
 if(!fence_){const auto begin=Now();uint32_t submitted=0;while(next_<count&&submitted<10&&Now()-begin<8000000){const auto& pass=permit_.kind==1?kSourcePasses[next_]:kQuantumPasses[next_];if(!Dispatch(pass,next_))return !Allowed()?Denied():Result::GL_FAILED;++next_;++submitted;}
  if(!Allowed())return Denied();glFlush();if(!Good())return Result::GL_FAILED;if(next_!=count)return Result::IN_PROGRESS;
  if(!Allowed())return Denied();fence_=glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE,0);glFlush();if(!fence_||!Good())return Result::GL_FAILED;
 }
 if(!Allowed())return Denied();const auto result=glClientWaitSync(fence_,0,0);if(result==GL_WAIT_FAILED||!Good())return Result::GL_FAILED;if(result==GL_TIMEOUT_EXPIRED)return Result::IN_PROGRESS;evidence_.fenceNs=Now();if(!Allowed())return Denied();
 glBindBuffer(GL_COPY_READ_BUFFER,buffers_[5]);const auto offset=profile_->work[8].begin;auto* mapped=glMapBufferRange(GL_COPY_READ_BUFFER,GLintptr(offset),1024,GL_MAP_READ_BIT);if(!mapped)return Result::GL_FAILED;std::memcpy(tail_.data(),mapped,1024);const bool unmapped=glUnmapBuffer(GL_COPY_READ_BUFFER)==GL_TRUE;if(!unmapped||!Good())return Result::GL_FAILED;
 glDeleteSync(fence_);fence_=nullptr;evidence_.gl=firstGl_;evidence_.egl=firstEgl_;mapped_=true;return Result::OK;
}
bool Transport::Commit(const Permit& p,const Completion256& w){
 if(!busy_||!mapped_||!(p==permit_)||w!=tail_||w[6]<1||w[6]>3||!Current()||Now()>p.deadline)return false;
 if(w[6]!=2){std::swap(buffers_[current_],buffers_[p.candidateBank]);}
 busy_=mapped_=false;return true;
}
bool Transport::RetireRejected(const Permit& p){
 if(!busy_)return true;if(!(p==permit_)||!Current())return false;
 if(!mapped_){if(!fence_){fence_=glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE,0);glFlush();return false;}const auto result=glClientWaitSync(fence_,0,0);if(result!=GL_ALREADY_SIGNALED&&result!=GL_CONDITION_SATISFIED)return false;glDeleteSync(fence_);fence_=nullptr;}
 busy_=mapped_=false;return Good();
}
void Transport::Release(bool current){
 current=current&&Current();if(current){if(fence_)glDeleteSync(fence_);glDeleteBuffers(GLsizei(buffers_.size()),buffers_.data());glDeleteBuffers(GLsizei(full_.size()),full_.data());if(program_.id)glDeleteProgram(program_.id);}
 if(program_.id)programcache::Forget(program_.id);buffers_={};full_={};program_={};fence_=nullptr;ready_=busy_=mapped_=false;profile_=nullptr;context_=EGL_NO_CONTEXT;
}
}
