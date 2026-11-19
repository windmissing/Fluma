// T566 (problem T563): composite visual transport implementation.
#include "visual_transport.h"
#include "../g9spf/schema_generated.h"
#include "../angles_generated.h"
#include "../program_cache/program_factory.h"
#include <chrono>
#include <cstring>
#include <limits>
#include <vector>
namespace grs::g11vis {
#include "g11vis_pipeline.h"
namespace {
uint64_t Now(){return uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());}
const char* PipelineAbi(uint32_t parent){return parent==1?kPipelineAbiDm:kPipelineAbiR1;}
}
// Pipeline sources live in the generated header; kernel_assets.cpp re-exports.
bool VisualTransport::Current()const{return context_!=EGL_NO_CONTEXT&&thread_==std::this_thread::get_id()&&eglGetCurrentContext()==context_&&eglGetCurrentDisplay()==display_&&eglGetCurrentSurface(EGL_DRAW)==surface_;}
bool VisualTransport::Good(){const auto gl=glGetError();const auto egl=eglGetError();if(gl&&!firstGl_)firstGl_=gl;if(egl!=EGL_SUCCESS&&!firstEgl_)firstEgl_=egl;return !firstGl_&&!firstEgl_;}
bool VisualTransport::Allowed()const{return Current()&&Now()<=permit_.parent.deadline&&check_&&check_(checkUser_,permit_.parent,Now());}
VisualTransport::Result VisualTransport::Denied()const{return !Current()?Result::CONTEXT_LOST:Now()>permit_.parent.deadline?Result::DEADLINE:Result::STALE;}
void VisualTransport::U(const char* n,uint32_t v){glUniform1ui(program_.uniforms.Find(n),v);}
void VisualTransport::I(const char* n,int32_t v){glUniform1i(program_.uniforms.Find(n),v);}
void VisualTransport::P(const char* n,uint64_t v){glUniform2ui(program_.uniforms.Find(n),uint32_t(v),uint32_t(v>>32));}
bool VisualTransport::Copy(GLuint from,size_t fromWord,GLuint to,size_t toWord,size_t words){
 if(from==to||fromWord>SIZE_MAX/4||toWord>SIZE_MAX/4||words>SIZE_MAX/4)return false;
 glBindBuffer(GL_COPY_READ_BUFFER,from);glBindBuffer(GL_COPY_WRITE_BUFFER,to);glCopyBufferSubData(GL_COPY_READ_BUFFER,GL_COPY_WRITE_BUFFER,GLintptr(fromWord*4),GLintptr(toWord*4),GLsizeiptr(words*4));glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT|GL_BUFFER_UPDATE_BARRIER_BIT);return Good();
}
VisualTransport::Result VisualTransport::QueryComputeCaps(const g9spf::ProfileRequirements& need,g9spf::StageCapabilityV2* out){
 // Composite compute eligibility mirrors the parent T564 gates: compute-stage
 // limits only; vertex/fragment counts are recorded but do not gate compute.
 if(!out||!Current())return Result::CONTEXT_LOST;
 g9spf::StageCapabilityV2 c{};c.context=context_;c.route=3;c.buildGeneration=identity_.context;
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
VisualTransport::Result VisualTransport::Create(uint32_t parentKind,uint32_t k,uint32_t v,const g9spf::Config& config,const g9spf::ControlIdentity& id,const g9spf::SeedFileIdentity& seed){
 if(parentKind!=0)return Result::UNSUPPORTED; // DM requires its real typed adapter, never R1 fallback.
 if(ready_||busy_||!config.Valid()||config.parameterSha!=g9spf::ParameterHash(config)||!id.run||!id.owner||!id.context)return Result::CALIBRATION_CONFIG_REQUIRED;
 const auto* parent=g9spf::SelectProfile(config.k);if(!parent)return Result::UNSUPPORTED;
 const auto* visual=SelectVisual(parentKind,config.k,v);if(!visual)return Result::UNSUPPORTED;
 context_=eglGetCurrentContext();display_=eglGetCurrentDisplay();surface_=eglGetCurrentSurface(EGL_DRAW);thread_=std::this_thread::get_id();if(!Current())return Result::CONTEXT_LOST;
 GLint bindings=0,blocks=0,shared=0,lanes=0;GLint64 maximum=0;
 glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS,&bindings);glGetIntegerv(GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS,&blocks);glGetIntegerv(GL_MAX_COMPUTE_SHARED_MEMORY_SIZE,&shared);glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS,&lanes);glGetInteger64v(GL_MAX_SHADER_STORAGE_BLOCK_SIZE,&maximum);
 if(!Good()||bindings<4||blocks<4||shared<4096||lanes<64||maximum<=0||uint64_t(maximum)<std::max(parent->arenaBytes,std::max(parent->stateBytes,visual->workBytes)))return Result::UNSUPPORTED;
 GLenum error=0;const char* source=parentKind==1?PipelineSourceDm():PipelineSourceR1();const char* hash=parentKind==1?PipelineSourceHashDm():PipelineSourceHashR1();
 if(!programcache::Build(1,source,hash,PipelineAbi(parentKind),program_.id,error)||!programcache::Reflect(program_.uniforms,program_.id)){if(error&&!firstGl_)firstGl_=error;Release(true);return Result::SHADER_FAILED;}
 parentProfile_=parent;visual_=visual;parentKind_=parentKind;config_=config;identity_=id;
 glGenBuffers(GLsizei(buffers_.size()),buffers_.data());
 for(size_t i=0;i<buffers_.size();++i){size_t bytes=i<5?parent->stateBytes:(i==5?visual->workBytes:parent->arenaBytes);glBindBuffer(GL_SHADER_STORAGE_BUFFER,buffers_[i]);glBufferData(GL_SHADER_STORAGE_BUFFER,GLsizeiptr(bytes),nullptr,GL_DYNAMIC_COPY);if(!Good()){Release(true);return Result::GL_FAILED;}}
 // Cold raw seed upload (input file, not host simulation); the GPU audits.
 std::vector<uint32_t> state;
 if(seed.state.empty())state.resize(parent->stateBytes/4);
 else{if(seed.state.size()!=parent->stateBytes/4||seed.sha!=g9spf::HashWords(seed.state.data(),seed.state.size())){Release(true);return Result::INVALID;}state=seed.state;}
 config_.seedSha=seed.state.empty()?g9spf::HashWords(state.data(),state.size()):seed.sha;
 auto put=[&](size_t at,uint64_t n){g9spf::Put(state.data(),at,n);};
 state[0]=g9spf::kStateMagic;state[1]=1;state[2]=parentKind==1?0u:g9spf::kProfileCode;put(4,id.run);put(6,id.owner);put(8,id.context);put(10,0);put(12,0);put(14,id.inputEpoch);put(16,0);
 if(seed.state.empty())state[18]=1;
 state[20]=0;state[21]=config.g;state[22]=0;state[23]=config.g;state[24]=config.k;state[25]=128;state[26]=state[27]=64;
 for(size_t i=0;i<8;++i)state[42+i]=config.parameterSha[i];
 state[98]=config.muStart;state[99]=config.muStop;state[100]=config.band;state[101]=config.entrainRate;state[102]=config.shear;state[103]=config.settleTicks;
 for(size_t i=0;i<5;++i){glBindBuffer(GL_SHADER_STORAGE_BUFFER,buffers_[i]);glBufferSubData(GL_SHADER_STORAGE_BUFFER,0,GLsizeiptr(parent->stateBytes),state.data());}
 // Visual WORK control segment init: bank headers for the three banks.
 std::vector<uint32_t> control(visual->visual[kSegControl]/4+16384,0);
 for(uint32_t bank=0;bank<3;++bank){size_t at=visual->visual[kSegControl]/4+bank*128;control[at+0]=kBankHeaderMagic;control[at+1]=kConfigRevision;control[at+2]=parentKind==1?kCompositeDmCode:kCompositeR1Code;control[at+3]=g9spf::kProfileCode;control[at+4]=config.k;control[at+5]=v;control[at+127]=kBankHeaderSeal;}
 glBindBuffer(GL_SHADER_STORAGE_BUFFER,buffers_[5]);glBufferSubData(GL_SHADER_STORAGE_BUFFER,GLintptr(visual_->visual[kSegControl]),GLsizeiptr(65536),control.data()+visual->visual[kSegControl]/4);
 if(!Good()){Release(true);return Result::GL_FAILED;}
 // Read-only warmup: ROOT_INIT writes only private WORK controls.
 glUseProgram(program_.id);g9spf::Input128 warm{};Common(warm);
 U("uOp",parentKind==1?12u:0u);U("uPass",0u);
 const auto begin=Now();glDispatchCompute(1,1,1);glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT|GL_BUFFER_UPDATE_BARRIER_BIT);glFinish();const auto end=Now();const auto errorWarm=glGetError();
 programcache::Warmup(program_.id,begin,end,errorWarm);
 if(errorWarm||programcache::Prepared(program_.id)!=GL_NO_ERROR||!Good()){if(errorWarm&&!firstGl_)firstGl_=errorWarm;Release(true);return Result::GL_FAILED;}
 current_=0;ready_=true;return Result::OK;
}
void VisualTransport::Common(const g9spf::Input128& input){
 const auto& p=*parentProfile_;const auto& vis=*visual_;
 U("uK",p.k);U("uStateWords",uint32_t(p.stateBytes/4));U("uColumn",uint32_t(p.state[3].begin/4));U("uBed",uint32_t(p.state[4].begin/4));U("uFlow",uint32_t(p.state[5].begin/4));U("uPacket",uint32_t(p.state[6].begin/4));U("uSources",uint32_t(p.state[7].begin/4));
 const char* names[]={"uFaces","uColumnStage","uHamilton","uOriginAudit","uIndexes","uInput","uSummary","uControls","uCompletion","uConfig","uRootExtension","uWideLedger","uContacts","uOriginExtension","uStageDiagnostic"};
 for(size_t i=0;i<15;++i)U(names[i],uint32_t(p.work[i].begin/4));
 // Visual segments: same WORK buffer, appended after the parent layout.
 U("uVisMarkerCurrent",uint32_t(vis.visual[kSegMarkerCurrent]/4));U("uVisMarkerActual",uint32_t(vis.visual[kSegMarkerActual]/4));U("uVisMarkerReplay",uint32_t(vis.visual[kSegMarkerReplay]/4));
 U("uVisGlyph",uint32_t(vis.visual[kSegGlyph]/4));U("uVisPlan",uint32_t(vis.visual[kSegMaterialPlan]/4));U("uVisControl",uint32_t(vis.visual[kSegControl]/4));U("uVisMaps",uint32_t(vis.visual[kSegFourMaps]/4));
 U("uVisV",vis.v);
 if(permit_.parent.quanta<=UINT64_MAX/32-1)P("uVisTicks",(permit_.parent.quanta+(permit_.parent.kind==2?1:0))*32);
 const auto& permit=permit_.parent;
 P("uNonce",permit.nonce);P("uRun",permit.identity.run);P("uOwner",permit.identity.owner);P("uContext",permit.identity.context);P("uSurface",permit.identity.surface);P("uGeometry",permit.geometry);P("uConfigId",permit.identity.config);P("uInputGeneration",permit.identity.inputEpoch);P("uSession",permit.identity.pageSession);P("uPlan",permit.plan);P("uBaseCommit",permit.baseCommit);P("uArenaGeneration",permit.arenaGeneration);
 U("uCandidateBank",permit.candidateBank);U("uBaseBank",permit.baseBank);U("uArena",permit.arena);
 U("uRootKind",permit.kind);U("uRequiredPasses",permit.kind==1?(parentKind_==1?kDmSourcePasses:kR1SourcePasses):(parentKind_==1?kDmQuantumPasses:kR1QuantumPasses));
 U("uRequiredAudits",permit.kind==1?(parentKind_==1?kDmSourceMask:kR1SourceMask):(parentKind_==1?kDmQuantumMask:kR1QuantumMask));
 U("uEventCapacity",p.eventCapacity);U("uPathCapacity",p.pathCapacity);U("uPathWord",p.eventCapacity*64);
 const auto angle=int32_t(input[24]);I("uAcceptedAngle",angle);const auto at=size_t(angle+7680)*4;I("uAngleSin",int32_t(kAngleWords[at+1]));I("uAngleCos",int32_t(kAngleWords[at+2]));
}
VisualTransport::Result VisualTransport::Begin(const CompositePermit& p,const g9spf::Input128& input,g9spf::PermitCheck check,void* user){
 if(!ready_||busy_||!Current())return Result::WOULD_BLOCK;
 if(!check||p.visualGeneration==UINT64_MAX||p.parent.quanta>UINT64_MAX/32-1)return Result::INVALID;
 if(p.parent.baseBank!=current_||p.parent.candidateBank>=5||p.parent.baseBank==p.parent.candidateBank||p.parent.kind<1||p.parent.kind>2||int32_t(input[24])<-7680||int32_t(input[24])>7680)return Result::INVALID;
 permit_=p;check_=check;checkUser_=user;if(!Allowed())return Denied();
 std::array<uint32_t,128> config{};config[0]=config_.muStart;config[1]=config_.muStop;config[2]=config_.band;config[3]=config_.entrainRate;config[4]=config_.shear;config[5]=config_.settleTicks;config[6]=config_.revision;config[7]=config_.g;
 for(size_t i=0;i<8;++i){config[16+i]=p.parent.parameterSha[i];config[24+i]=p.parent.layoutSha[i];config[32+i]=p.parent.shaderSha[i];config[40+i]=p.parent.inputSha[i];config[48+i]=config_.seedSha[i];}
 glBindBuffer(GL_SHADER_STORAGE_BUFFER,buffers_[5]);glBufferSubData(GL_SHADER_STORAGE_BUFFER,GLintptr(parentProfile_->work[5].begin),512,input.data());glBufferSubData(GL_SHADER_STORAGE_BUFFER,GLintptr(parentProfile_->work[9].begin),512,config.data());
 glUseProgram(program_.id);Common(input);if(!Good())return Result::GL_FAILED;
 evidence_={};tail_={};next_=0;busy_=true;mapped_=false;return Result::OK;
}
void VisualTransport::Bind(const G11Pass& pass){
 // Parent binding logic mirrors g9spf::Transport::Bind; VIS rows ride the
 // same four bindings with the extended WORK at binding 2.
 GLuint read=buffers_[1],write=buffers_[permit_.parent.candidateBank],events=buffers_[6];
 const auto& permit=permit_.parent;
 if(!pass.vis&&(pass.op==uint32_t(parentKind_==1?13u:1u))){read=buffers_[current_];write=permit.kind==1?buffers_[permit.candidateBank]:buffers_[1];}
 if(pass.role==3||pass.role==4)write=buffers_[4];
 glBindBufferBase(GL_SHADER_STORAGE_BUFFER,0,read);glBindBufferBase(GL_SHADER_STORAGE_BUFFER,1,write);glBindBufferBase(GL_SHADER_STORAGE_BUFFER,2,buffers_[5]);glBindBufferBase(GL_SHADER_STORAGE_BUFFER,3,events);
}
bool VisualTransport::Dispatch(const G11Pass& pass,uint32_t index){
 if(!Allowed())return false;
 // Marker bank pair for this dispatch: VIS reads ACTUAL for authoritative and
 // REPLAY for replay roles; the write target stays within the same bank.
 U("uVisOldBank",pass.role==4?2u:1u);U("uVisNewBank",pass.role==4?2u:1u);
 U("uVisPhase",pass.phase);U("uVisOrdinal",uint32_t(index));
 Bind(pass);U("uOp",pass.op);U("uRole",pass.role);U("uJslot",pass.j);U("uMicro",uint32_t(pass.micro));U("uPass",index);
 if(!Allowed())return false;glDispatchCompute(1,1,1);++evidence_.dispatches;glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT|GL_BUFFER_UPDATE_BARRIER_BIT);++evidence_.barriers;
 if(!Good())return false;
 return Allowed();
}
VisualTransport::Result VisualTransport::Pump(){
 if(!busy_)return Result::INVALID;if(!Allowed())return Denied();if(mapped_)return Result::OK;
 // Composite tables include the VIS rows; kind selects source vs quantum.
 const bool isSource=permit_.parent.kind==1;
 const G11Pass* table=nullptr;uint32_t count=0;
 if(parentKind_==0){table=isSource?kG11R1SourcePasses.data():kG11R1QuantumPasses.data();count=isSource?uint32_t(kG11R1SourcePasses.size()):uint32_t(kG11R1QuantumPasses.size());}
 else{table=isSource?kG11DmSourcePasses.data():kG11DmQuantumPasses.data();count=isSource?uint32_t(kG11DmSourcePasses.size()):uint32_t(kG11DmQuantumPasses.size());}
 if(!fence_){const auto begin=Now();uint32_t submitted=0;
  while(next_<count&&submitted<10&&Now()-begin<8000000){if(!Dispatch(table[next_],next_))return !Allowed()?Denied():Result::GL_FAILED;++next_;++submitted;}
  if(!Allowed())return Denied();glFlush();if(!Good())return Result::GL_FAILED;if(next_!=count)return Result::IN_PROGRESS;
  if(!Allowed())return Denied();fence_=glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE,0);glFlush();if(!fence_||!Good())return Result::GL_FAILED;
 }
 if(!Allowed())return Denied();const auto result=glClientWaitSync(fence_,0,0);if(result==GL_WAIT_FAILED||!Good())return Result::GL_FAILED;if(result==GL_TIMEOUT_EXPIRED)return Result::IN_PROGRESS;
 evidence_.fenceNs=Now();if(!Allowed())return Denied();
 // Read the composite completion tail (1024B) from the parent layout.
 glBindBuffer(GL_COPY_READ_BUFFER,buffers_[5]);const auto offset=parentProfile_->work[8].begin;auto* mapped=glMapBufferRange(GL_COPY_READ_BUFFER,GLintptr(offset),1024,GL_MAP_READ_BIT);
 if(!mapped)return Result::GL_FAILED;std::memcpy(tail_.words.data(),mapped,1024);const bool unmapped=glUnmapBuffer(GL_COPY_READ_BUFFER)==GL_TRUE;if(!unmapped||!Good())return Result::GL_FAILED;
 glDeleteSync(fence_);fence_=nullptr;evidence_.gl=firstGl_;evidence_.egl=firstEgl_;mapped_=true;
 // Composite audit bits are set after VIS_ROOT_AUDIT+GLYPH complete.
 tail_.visAuditBits=tail_.words[42]&((1u<<kAuditPlanMapping)|(1u<<kAuditVisReplayGlyph));
 return Result::OK;
}
bool VisualTransport::Commit(const CompositePermit& p,const CompositeCompletion& w){
 if((w.visAuditBits&((1u<<kAuditPlanMapping)|(1u<<kAuditVisReplayGlyph)))!=((1u<<kAuditPlanMapping)|(1u<<kAuditVisReplayGlyph)))return false;
 if(!busy_||!mapped_||!(p==permit_)||w.words!=tail_.words||w.visAuditBits!=tail_.visAuditBits||w.words[6]<1||w.words[6]>3||!Current()||Now()>p.parent.deadline)return false;
 // T639: awaiting formal parent composite typed proof; no bank can switch.
 return false;
}
bool VisualTransport::RetireRejected(const CompositePermit& p){
 if(!busy_)return true;if(!(p==permit_)||!Current())return false;
 if(!mapped_){if(!fence_){fence_=glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE,0);glFlush();return false;}const auto result=glClientWaitSync(fence_,0,0);if(result!=GL_ALREADY_SIGNALED&&result!=GL_CONDITION_SATISFIED)return false;glDeleteSync(fence_);fence_=nullptr;}
 busy_=mapped_=false;return Good();
}
void VisualTransport::Release(bool current){
 current=current&&Current();if(current){if(fence_)glDeleteSync(fence_);glDeleteBuffers(GLsizei(buffers_.size()),buffers_.data());if(program_.id)glDeleteProgram(program_.id);}
 if(program_.id)programcache::Forget(program_.id);buffers_={};program_={};fence_=nullptr;ready_=busy_=mapped_=false;parentProfile_=nullptr;visual_=nullptr;context_=EGL_NO_CONTEXT;
}
}
