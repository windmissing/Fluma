// T576 (problem T567): G12 water transport implementation.
#include "transport.h"
#include "../angles_generated.h"
#include "../program_cache/program_factory.h"
#include <chrono>
#include <cstring>
#include <limits>
#include <vector>
namespace grs::g12water {
namespace {
uint64_t Now(){return uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());}
}
bool Transport::Current()const{return context_!=EGL_NO_CONTEXT&&thread_==std::this_thread::get_id()&&eglGetCurrentContext()==context_&&eglGetCurrentDisplay()==display_&&eglGetCurrentSurface(EGL_DRAW)==surface_;}
bool Transport::Good(){const auto gl=glGetError();const auto egl=eglGetError();if(gl&&!firstGl_)firstGl_=gl;if(egl!=EGL_SUCCESS&&!firstEgl_)firstEgl_=egl;return !firstGl_&&!firstEgl_;}
bool Transport::Allowed()const{return Current()&&Now()<=permit_.deadline;}
Result Transport::Denied()const{return !Current()?Result::CONTEXT_LOST:Now()>permit_.deadline?Result::DEADLINE:Result::STALE;}
void Transport::U(const char* n,uint32_t v){glUniform1ui(program_.uniforms.Find(n),v);}
void Transport::I(const char* n,int32_t v){glUniform1i(program_.uniforms.Find(n),v);}
void Transport::P(const char* n,uint64_t v){glUniform2ui(program_.uniforms.Find(n),uint32_t(v),uint32_t(v>>32));}
bool Transport::Copy(GLuint from,size_t fromWord,GLuint to,size_t toWord,size_t words){
 if(from==to||fromWord>SIZE_MAX/4||toWord>SIZE_MAX/4||words>SIZE_MAX/4)return false;
 glBindBuffer(GL_COPY_READ_BUFFER,from);glBindBuffer(GL_COPY_WRITE_BUFFER,to);glCopyBufferSubData(GL_COPY_READ_BUFFER,GL_COPY_WRITE_BUFFER,GLintptr(fromWord*4),GLintptr(toWord*4),GLsizeiptr(words*4));glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT|GL_BUFFER_UPDATE_BARRIER_BIT);return Good();
}
Result Transport::QueryComputeCaps(const g9spf::ProfileRequirements& need,g9spf::StageCapabilityV2* out){
 if(!out||!Current())return Result::CONTEXT_LOST;
 g9spf::StageCapabilityV2 c{};c.context=context_;c.route=4;c.buildGeneration=identity_.context;
 int32_t bad=0;
 bad|=g9spf::QueryCapabilityI(c,GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS,c.ssboBindings,"COMPOSITE",4);
 bad|=g9spf::QueryCapabilityI(c,GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS,c.computeBlocks,"COMPOSITE",4);
 bad|=g9spf::QueryCapabilityI(c,GL_MAX_COMBINED_SHADER_STORAGE_BLOCKS,c.combinedBlocks,"COMPOSITE",4);
 bad|=g9spf::QueryCapabilityI(c,GL_MAX_COMPUTE_SHARED_MEMORY_SIZE,c.sharedBytes,"COMPOSITE",4096);
 bad|=g9spf::QueryCapabilityI(c,GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS,c.invocations,"COMPOSITE",64);
 {GLint v[3]={0,0,0};glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE,0,&v[0]);const auto s=int32_t(glGetError());c.sizeX=v[0];g9spf::RecordQuery(c,GL_MAX_COMPUTE_WORK_GROUP_SIZE+0,v[0],s,64,"COMPOSITE");bad|=s;
  glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE,1,&v[1]);const auto s1=int32_t(glGetError());c.sizeY=v[1];g9spf::RecordQuery(c,GL_MAX_COMPUTE_WORK_GROUP_SIZE+1,v[1],s1,1,"COMPOSITE");bad|=s1;
  glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE,2,&v[2]);const auto s2=int32_t(glGetError());c.sizeZ=v[2];g9spf::RecordQuery(c,GL_MAX_COMPUTE_WORK_GROUP_SIZE+2,v[2],s2,1,"COMPOSITE");bad|=s2;}
 {GLint v=0;glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_COUNT,&v);const auto s=int32_t(glGetError());c.countX=v;g9spf::RecordQuery(c,GL_MAX_COMPUTE_WORK_GROUP_COUNT,v,s,1,"COMPOSITE");bad|=s;}
 {GLint64 v=0;glGetInteger64v(GL_MAX_SHADER_STORAGE_BLOCK_SIZE,&v);const auto s=int32_t(glGetError());c.maxBlockBytes=v;g9spf::RecordQuery(c,GL_MAX_SHADER_STORAGE_BLOCK_SIZE,int64_t(v),s,int32_t(std::max<size_t>(need.stateBytes,std::max(need.workBytes,need.arenaBytes))),"COMPOSITE");bad|=s;}
 bad|=g9spf::QueryCapabilityI(c,GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS,c.vertexBlocksQueried,"SCENE_ATTR",0);
 bad|=g9spf::QueryCapabilityI(c,GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS,c.fragmentBlocksQueried,"SCENE_ATTR",0);
 const auto gl=firstGl_;*out=c;
 if(bad||gl){out->computeFailure=bad?"UNSUPPORTED_QUERY":"PRIOR_GL_ERROR";return Result::UNSUPPORTED;}
 if(c.ssboBindings<4||c.computeBlocks<4||c.combinedBlocks<4||c.sharedBytes<4096||c.invocations<64||c.sizeX<64||c.countX<1||c.maxBlockBytes<int64_t(std::max<size_t>(need.stateBytes,std::max(need.workBytes,need.arenaBytes)))){out->computeFailure="COMPOSITE_LIMIT";return Result::UNSUPPORTED;}
 out->computeEligible=true;return Result::OK;
}
Result Transport::Create(uint32_t k,uint32_t kw,const MaterialConfig128& config,const SeedDescriptor& seed,const g9spf::ControlIdentity& id){
 if(ready_||busy_||!id.run||!id.owner||!id.context)return Result::CALIBRATION_CONFIG_REQUIRED;
 const auto* profile=SelectProfile(k,kw);if(!profile)return Result::UNSUPPORTED;
 if(config.words[kCfg_Magic]!=kMagicConfig||config.words[kCfg_Revision]!=kRevision||config.words[kCfg_Profile]!=kProfileCode)return Result::INVALID;
 context_=eglGetCurrentContext();display_=eglGetCurrentDisplay();surface_=eglGetCurrentSurface(EGL_DRAW);thread_=std::this_thread::get_id();if(!Current())return Result::CONTEXT_LOST;
 GLint bindings=0,blocks=0,shared=0,lanes=0;GLint64 maximum=0;
 glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS,&bindings);glGetIntegerv(GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS,&blocks);glGetIntegerv(GL_MAX_COMPUTE_SHARED_MEMORY_SIZE,&shared);glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS,&lanes);glGetInteger64v(GL_MAX_SHADER_STORAGE_BLOCK_SIZE,&maximum);
 if(!Good()||bindings<4||blocks<4||shared<4096||lanes<64||maximum<=0||uint64_t(maximum)<std::max(profile->arenaBytes,std::max(profile->stateBytes,profile->workBytes)))return Result::UNSUPPORTED;
 GLenum error=0;
 if(!programcache::Build(1,PipelineSource(),PipelineSourceHash(),"G12_POROUS_SPF_Q20_V1_FOUR_SSBO_INTEGER128",program_.id,error)||!programcache::Reflect(program_.uniforms,program_.id)){if(error&&!firstGl_)firstGl_=error;Release(true);return Result::SHADER_FAILED;}
 profile_=profile;config_=config;identity_=id;seed_=seed;
 profileIndex_=uint32_t((k==512u?2u:0u)+(kw==128u?1u:0u));
 glGenBuffers(GLsizei(buffers_.size()),buffers_.data());
 for(size_t i=0;i<buffers_.size();++i){size_t bytes=i<5?profile->stateBytes:(i==5?profile->workBytes:profile->arenaBytes);glBindBuffer(GL_SHADER_STORAGE_BUFFER,buffers_[i]);glBufferData(GL_SHADER_STORAGE_BUFFER,GLsizeiptr(bytes),nullptr,GL_DYNAMIC_COPY);if(!Good()){Release(true);return Result::GL_FAILED;}}
 // State init: parent header section zeroed, water tail headers written.
 std::vector<uint32_t> state(profile->stateBytes/4,0);
 state[0]=kMagicState;state[1]=kRevision;state[2]=kProfileCode;state[24]=k;state[25]=128;state[26]=state[27]=64;
 auto put=[&](size_t at,uint64_t n){g9spf::Put(state.data(),at,n);};
 put(4,id.run);put(6,id.owner);put(8,id.context);put(14,id.inputEpoch);
 // Water header at the tail.
 const size_t waterHeader=591872/4+(k==512u?2560u:0u);
 state[waterHeader+0]=kMagicWaterHeader;state[waterHeader+1]=kRevision;state[waterHeader+2]=kw;
 for(size_t i=0;i<5;++i){glBindBuffer(GL_SHADER_STORAGE_BUFFER,buffers_[i]);glBufferSubData(GL_SHADER_STORAGE_BUFFER,0,GLsizeiptr(profile->stateBytes),state.data());}
 // GPU cold seed upload: input file bytes, not a host simulation. The GPU
 // seed dispatches validate/audit/seal before any permit is granted.
 if(!seed.bytes.empty()){
  glBindBuffer(GL_SHADER_STORAGE_BUFFER,buffers_[5]);
  // Seed words stream through the runtime config extension segment for the
  // SEED_UPLOAD_VALIDATE dispatch; the caller-span hash check is the seed
  // recipe receipt.
  const size_t ext=profile->work[11].begin/4;
  if(seed.bytes.size()<=profile->work[11].bytes/4)glBufferSubData(GL_SHADER_STORAGE_BUFFER,GLintptr(ext*4),GLsizeiptr(seed.bytes.size()*4),seed.bytes.data());
 }
 if(!Good()){Release(true);return Result::GL_FAILED;}
 // Read-only warmup: ROOT_INIT writes only private WORK controls.
 glUseProgram(program_.id);Input128 warm{};Common(warm);U("uOp",16u);U("uPass",0u);
 const auto begin=Now();glDispatchCompute(1,1,1);glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT|GL_BUFFER_UPDATE_BARRIER_BIT);glFinish();const auto end=Now();const auto errorWarm=glGetError();
 programcache::Warmup(program_.id,begin,end,errorWarm);
 if(errorWarm||programcache::Prepared(program_.id)!=GL_NO_ERROR||!Good()){if(errorWarm&&!firstGl_)firstGl_=errorWarm;Release(true);return Result::GL_FAILED;}
 current_=0;ready_=true;return Result::OK;
}
void Transport::Common(const Input128& input){
 const auto& p=*profile_;
 U("uK",p.k);U("uKw",p.kw);U("uStateWords",uint32_t(p.stateBytes/4));
 U("uColumn",uint32_t(2048/4));U("uBed",uint32_t(18432/4));U("uFlow",uint32_t(149504/4));U("uPacket",uint32_t(280576/4));U("uSources",uint32_t(329728/4));
 // Parent WORK segments (same parent layout, first 15).
 const char* names[]={"uFaces","uColumnStage","uHamilton","uOriginAudit","uIndexes","uInput","uSummary","uControls","uCompletion","uConfig","uRootExtension","uWideLedger","uContacts","uOriginExtension","uStageDiagnostic"};
 for(size_t i=0;i<15;++i)U(names[i],uint32_t(p.work[i].begin/4));
 // Water state tail + wet WORK segments.
 U("uWaterHeader",uint32_t(591872/4+(profile_->k==512u?2560u:0u)));
 U("uWaterColumn",uint32_t((592128+(profile_->k==512u?2560u:0u))/4));
 U("uWaterOrigin",uint32_t((600320+(profile_->k==512u?2560u:0u))/4));
 U("uWaterSource",uint32_t((731392+(profile_->k==512u?2560u:0u))/4));
 U("uWaterPacket",uint32_t((993536+(profile_->k==512u?2560u:0u))/4));
 U("uProfileIndex",profileIndex_);
 U("uWet",permit_.wet);
 const auto& permit=permit_;
 P("uNonce",permit.nonce);P("uRun",permit.identity.run);P("uOwner",permit.identity.owner);P("uContext",permit.identity.context);P("uSurface",permit.identity.surface);P("uGeometry",permit.frame);P("uConfigId",permit.identity.config);P("uInputGeneration",permit.identity.inputEpoch);P("uSession",permit.identity.pageSession);P("uPlan",permit.plan);P("uBaseCommit",permit.baseCommit);P("uArenaGeneration",uint64_t(permit.frame));
 U("uCandidateBank",permit.candidateBank);U("uBaseBank",permit.baseBank);U("uArena",permit.arena);
 U("uRequiredPasses",permit.kind==1?kSourcePasses:(permit.wet?kWetQuantumPasses:kDryQuantumPasses));
 U("uEventCapacity",33488u+(p.k==512u?20800u:0u));U("uPathCapacity",49152u+(p.k==512u?81920u:0u));U("uPathWord",(33488u+(p.k==512u?20800u:0u))*64u);
 const auto angle=int32_t(input[24]);I("uAcceptedAngle",angle);const auto at=size_t(angle+7680)*4;I("uAngleSin",int32_t(kAngleWords[at+1]));I("uAngleCos",int32_t(kAngleWords[at+2]));
}
Result Transport::Begin(const G12Permit& p,const Input128& input,g9spf::PermitCheck check,void* user){
 if(!ready_||busy_||!Current())return Result::WOULD_BLOCK;
 if(p.baseBank!=current_||p.candidateBank>=5||p.baseBank==p.candidateBank||p.kind<1||p.kind>2||int32_t(input[24])<-7680||int32_t(input[24])>7680)return Result::INVALID;
 permit_=p;check_=check;checkUser_=user;if(!Allowed())return Denied();
 glBindBuffer(GL_SHADER_STORAGE_BUFFER,buffers_[5]);
 glBufferSubData(GL_SHADER_STORAGE_BUFFER,GLintptr(profile_->work[5].begin),512,input.data());
 glBufferSubData(GL_SHADER_STORAGE_BUFFER,GLintptr(profile_->work[11].begin),512,config_.words.data());
 glUseProgram(program_.id);Common(input);if(!Good())return Result::GL_FAILED;
 evidence_={};tail_={};next_=0;busy_=true;mapped_=false;return Result::OK;
}
void Transport::Bind(const G12Pass& pass){
 GLuint read=buffers_[1],write=buffers_[permit_.candidateBank],events=buffers_[6];
 if(pass.role==3||pass.role==4)write=buffers_[4];
 glBindBufferBase(GL_SHADER_STORAGE_BUFFER,0,read);glBindBufferBase(GL_SHADER_STORAGE_BUFFER,1,write);glBindBufferBase(GL_SHADER_STORAGE_BUFFER,2,buffers_[5]);glBindBufferBase(GL_SHADER_STORAGE_BUFFER,3,events);
}
bool Transport::Dispatch(const G12Pass& pass,uint32_t index){
 if(!Allowed())return false;
 Bind(pass);U("uOp",pass.op);U("uRole",pass.role);U("uJslot",pass.j);U("uMicro",uint32_t(pass.micro<0?0u:uint32_t(pass.micro)));U("uPass",index);
 if(!Allowed())return false;glDispatchCompute(1,1,1);++evidence_.dispatches;glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT|GL_BUFFER_UPDATE_BARRIER_BIT);++evidence_.barriers;
 if(!Good())return false;
 return Allowed();
}
Result Transport::Pump(){
 if(!busy_)return Result::INVALID;if(!Allowed())return Denied();if(mapped_)return Result::OK;
 const bool isSource=permit_.kind==1;
 const G12Pass* table=nullptr;uint32_t count=0;
 if(isSource){table=kG12SourcePasses.data();count=uint32_t(kG12SourcePasses.size());}
 else if(permit_.wet){table=kG12WetQuantumPasses.data();count=uint32_t(kG12WetQuantumPasses.size());}
 else{table=kG12DryQuantumPasses.data();count=uint32_t(kG12DryQuantumPasses.size());}
 if(!fence_){const auto begin=Now();uint32_t submitted=0;
  while(next_<count&&submitted<10&&Now()-begin<8000000){if(!Dispatch(table[next_],next_))return !Allowed()?Denied():Result::GL_FAILED;++next_;++submitted;}
  if(!Allowed())return Denied();glFlush();if(!Good())return Result::GL_FAILED;if(next_!=count)return Result::IN_PROGRESS;
  if(!Allowed())return Denied();fence_=glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE,0);glFlush();if(!fence_||!Good())return Result::GL_FAILED;
 }
 if(!Allowed())return Denied();const auto result=glClientWaitSync(fence_,0,0);if(result==GL_WAIT_FAILED||!Good())return Result::GL_FAILED;if(result==GL_TIMEOUT_EXPIRED)return Result::IN_PROGRESS;
 evidence_.fenceNs=Now();if(!Allowed())return Denied();
 glBindBuffer(GL_COPY_READ_BUFFER,buffers_[5]);const auto offset=profile_->work[8].begin;auto* mapped=glMapBufferRange(GL_COPY_READ_BUFFER,GLintptr(offset),1024,GL_MAP_READ_BIT);
 if(!mapped)return Result::GL_FAILED;std::memcpy(tail_.data(),mapped,1024);const bool unmapped=glUnmapBuffer(GL_COPY_READ_BUFFER)==GL_TRUE;if(!unmapped||!Good())return Result::GL_FAILED;
 glDeleteSync(fence_);fence_=nullptr;evidence_.gl=firstGl_;evidence_.egl=firstEgl_;mapped_=true;return Result::OK;
}
bool Transport::Commit(const G12Permit& p,const Completion256& w){
 if(!busy_||!mapped_||!(p==permit_)||w!=tail_||w[6]<1||w[6]>3||!Current()||Now()>p.deadline)return false;
 if(w[6]!=2){std::swap(buffers_[current_],buffers_[p.candidateBank]);}
 busy_=mapped_=false;return true;
}
bool Transport::RetireRejected(const G12Permit& p){
 if(!busy_)return true;if(!(p==permit_)||!Current())return false;
 if(!mapped_){if(!fence_){fence_=glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE,0);glFlush();return false;}const auto result=glClientWaitSync(fence_,0,0);if(result!=GL_ALREADY_SIGNALED&&result!=GL_CONDITION_SATISFIED)return false;glDeleteSync(fence_);fence_=nullptr;}
 busy_=mapped_=false;return Good();
}
void Transport::Release(bool current){
 current=current&&Current();if(current){if(fence_)glDeleteSync(fence_);glDeleteBuffers(GLsizei(buffers_.size()),buffers_.data());if(program_.id)glDeleteProgram(program_.id);}
 if(program_.id)programcache::Forget(program_.id);buffers_={};program_={};fence_=nullptr;ready_=busy_=mapped_=false;profile_=nullptr;context_=EGL_NO_CONTEXT;
}
}
