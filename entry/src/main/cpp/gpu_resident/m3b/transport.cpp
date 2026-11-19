#include "transport.h"
#include "init_program_watch.h"
#include "kernel_assets.h"
#include "shader_variant.h"
#include "cold_contract.h"
#include "../angles_generated.h"
#include "../program_cache/program_factory.h"
#include <chrono>
#include <algorithm>
#include <cstring>
#include <string>
namespace grs::m3b {
namespace {
uint64_t Now(){return uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());}
uint64_t Value(const Words256& w,size_t at){return uint64_t(w[at])|(uint64_t(w[at+1])<<32);}
}
bool TransportV5::Current()const{return context_!=EGL_NO_CONTEXT&&eglGetCurrentContext()==context_&&eglGetCurrentDisplay()==display_&&eglGetCurrentSurface(EGL_DRAW)==surface_&&thread_==std::this_thread::get_id();}
bool TransportV5::Good(){auto gl=glGetError();auto egl=eglGetError();if(gl&&!rawGl_)rawGl_=gl;if(egl!=EGL_SUCCESS&&rawEgl_==EGL_SUCCESS)rawEgl_=egl;return !rawGl_&&rawEgl_==EGL_SUCCESS;}
bool TransportV5::Allowed(){return Current()&&Now()<=permit_.deadline&&authorize_&&authorize_(authorization_,permit_,Now());}
TransportResultV5 TransportV5::Denied()const{return !Current()?TransportResultV5::CONTEXT_LOST:Now()>permit_.deadline?TransportResultV5::DEADLINE:TransportResultV5::STALE;}
void TransportV5::Use(uint32_t index){activeProgram_=index;if(!dryUniforms_)glUseProgram(programs_[index].id);}
void TransportV5::U(const char* name,uint32_t value){uploadCounter_+=4;if(!dryUniforms_)glUniform1ui(programs_[activeProgram_].locations.Find(name),value);}
void TransportV5::I(const char* name,int32_t value){uploadCounter_+=4;if(!dryUniforms_)glUniform1i(programs_[activeProgram_].locations.Find(name),value);}
void TransportV5::Pair(const char* name,uint64_t value){uploadCounter_+=8;if(!dryUniforms_)glUniform2ui(programs_[activeProgram_].locations.Find(name),uint32_t(value),uint32_t(value>>32));}
void TransportV5::Quad(const char* name,const uint32_t* value){uploadCounter_+=16;if(!dryUniforms_)glUniform4uiv(programs_[activeProgram_].locations.Find(name),1,value);}
void TransportV5::Barrier(){glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT|GL_BUFFER_UPDATE_BARRIER_BIT);++evidence_.barriers;}
void TransportV5::Copy(GLuint from,size_t fromWord,GLuint to,size_t toWord,size_t words){
 glBindBuffer(GL_COPY_READ_BUFFER,from);glBindBuffer(GL_COPY_WRITE_BUFFER,to);glCopyBufferSubData(GL_COPY_READ_BUFFER,GL_COPY_WRITE_BUFFER,GLintptr(fromWord*4),GLintptr(toWord*4),GLsizeiptr(words*4));
}
TransportResultV5 TransportV5::Create(const Profile& p,const ControlIdentityV5& id,uint64_t geometry,const PhysicalConfigV5& config,
 const std::array<uint32_t,8>& shader,const std::array<uint32_t,8>& angle,const std::array<uint32_t,8>& layoutSha){
 if(busy_||ready_||!config.Valid()||!id.run||!geometry)return TransportResultV5::INVALID;
 context_=eglGetCurrentContext();display_=eglGetCurrentDisplay();surface_=eglGetCurrentSurface(EGL_DRAW);thread_=std::this_thread::get_id();if(!Current())return TransportResultV5::CONTEXT_LOST;
 GLint vertexBlocks=0;glGetIntegerv(GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS,&vertexBlocks);if(vertexBlocks<1)return TransportResultV5::UNSUPPORTED;
 GLint alignment=0,bindings=0,blocks=0,fragment=0,invocations=0,shared=0;GLint64 maximum=0;
 glGetIntegerv(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT,&alignment);glGetInteger64v(GL_MAX_SHADER_STORAGE_BLOCK_SIZE,&maximum);glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS,&bindings);glGetIntegerv(GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS,&blocks);glGetIntegerv(GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS,&fragment);glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS,&invocations);glGetIntegerv(GL_MAX_COMPUTE_SHARED_MEMORY_SIZE,&shared);
 if(alignment<=0||maximum<=0||!WorkLayout::Make(p.k,p.kw,size_t(alignment),layout_)||!layout_.Supports(size_t(maximum),bindings,blocks,fragment,invocations,shared)||!ResidentBuffersV5::Make(p,size_t(alignment),allocation_))return TransportResultV5::UNSUPPORTED;
 for(auto n:allocation_.bytes)if(n>size_t(maximum)||n>size_t(PTRDIFF_MAX))return TransportResultV5::UNSUPPORTED;
 alignment_=size_t(alignment);profile_=&p;for(const auto& old:m2::kProfiles)if(old.k==p.k&&old.kw==p.kw)old_=&old;
 if(!old_)return TransportResultV5::INVALID;config_=config;
 const auto extension=uint32_t(SelectLineageProfile(p.k,p.kw)->stateBytes-SelectLineageProfile(p.k,p.kw)->oldStateBytes)/4;
 oldStorage_=*old_;headerWord_=old_->stateWords-256;
 auto move=[&](m2::WordRange& r){r.begin+=extension;r.end+=extension;};
 move(oldStorage_.sourceLedger);move(oldStorage_.index0);move(oldStorage_.index1);move(oldStorage_.control);move(oldStorage_.sourceOrigins);move(oldStorage_.config);move(oldStorage_.quantumOrigins);
 oldStorage_.sourceBytes+=extension*4;oldStorage_.quantumBytes+=extension*4;oldStorage_.stateWords+=extension;old_=&oldStorage_;
 sandIndex_=32;waterIndex_=sandIndex_+old_->pad;arrivalIndex_=waterIndex_+p.kw;edges_=arrivalIndex_+p.kw;frozen_=edges_+127*16;rows_=frozen_+512;angles_=rows_+p.k+p.kw;ledger_=angles_+61444;
 for(uint32_t n=0;n<20;++n){
  std::string source=n==0?SourceKernelV5():QuantumKernelV5();
  if(n){const auto at=source.find('\n');if(at==std::string::npos)return TransportResultV5::SHADER_FAILED;source.insert(at+1,kOpcodeDefines[n-1]);}
  if(source.size()>kMaximumKernelSourceBytes)return TransportResultV5::UNSUPPORTED;
  const auto role=n==0?1u:n==19?3u:2u;const auto stage=n==0?init650::Code::PROGRAM_SOURCE:n==19?init650::Code::PROGRAM_FINAL:init650::Code::PROGRAM_INTERMEDIATE;
  const auto sourceDigest=init650::Hex(programcache::Hash(source).c_str());
  init650::ProgramScope watch(initDiagnostics_,stage,role,sourceDigest);
  if(initDiagnostics_.store)initDiagnostics_.store->Emit(initDiagnostics_.ticket,stage,"ENTER",-1,init650::Life::NONE,&sourceDigest,role);
  GLenum original=0;auto& program=programs_[n];
  // One existing T475 Store; exact specialized bytes and independent V5 ABI
  // enter the key. No second file worker, CPU decision or global route patch.
  if(!programcache::Build(n==0?0:1,source.c_str(),programcache::Hash(source).c_str(),"GRS_M3_V5_FEATURES31_FOUR_SSBO",program.id,original)||!programcache::Reflect(program.locations,program.id)){if(initDiagnostics_.store)initDiagnostics_.store->Emit(initDiagnostics_.ticket,stage,"EXIT",0,init650::Life::FAILED,&sourceDigest,role,original?int32_t(original):-1);if(original&&!rawGl_)rawGl_=original;Release(true);return TransportResultV5::SHADER_FAILED;}
  if(initDiagnostics_.store)initDiagnostics_.store->Emit(initDiagnostics_.ticket,stage,"EXIT",1,init650::Life::NONE,&sourceDigest,role);
 }
 glGenBuffers(GLsizei(buffers_.size()),buffers_.data());
 for(size_t i=0;i<buffers_.size();++i){glBindBuffer(GL_SHADER_STORAGE_BUFFER,buffers_[i]);glBufferData(GL_SHADER_STORAGE_BUFFER,GLsizeiptr(allocation_.bytes[i]),nullptr,GL_DYNAMIC_COPY);if(!Good()){Release(true);return TransportResultV5::GL_FAILED;}}
 // A fresh independent run, initialized only once at cold creation.
 std::vector<uint32_t> state(p.stateBytes/4);const uint32_t h=headerWord_;
 auto pair=[&](uint32_t at,uint64_t value){state[h+at]=uint32_t(value);state[h+at+1]=uint32_t(value>>32);};
 state[h]=1213420103;state[h+1]=5;state[h+2]=256;pair(4,id.run);pair(6,id.owner);pair(8,id.context);pair(10,id.surface);pair(20,geometry);pair(22,id.config);pair(84,id.window);
 const uint32_t versions[]={1,1,1,3,5,5,5,31,0};for(size_t i=0;i<9;++i)state[h+24+i]=versions[i];
 state[h+39]=state[h+40]=1;state[h+43]=p.k;state[h+44]=p.kw;state[h+122]=1;state[h+123]=65536;state[h+124]=state[h+125]=2;state[h+145]=1;state[h+146]=15;state[h+156]=512;state[h+160]=1;state[h+161]=1;state[h+162]=uint32_t(config.lift.mode);state[h+181]=64;
 for(size_t i=0;i<8;++i){state[h+88+i]=shader[i];state[h+96+i]=angle[i];state[h+104+i]=layoutSha[i];}
 for(size_t i=0;i<3;++i){glBindBuffer(GL_SHADER_STORAGE_BUFFER,buffers_[i]);glBufferSubData(GL_SHADER_STORAGE_BUFFER,0,GLsizeiptr(p.stateBytes),state.data());}
 glBindBuffer(GL_SHADER_STORAGE_BUFFER,buffers_[16]);glBufferSubData(GL_SHADER_STORAGE_BUFFER,GLintptr(angles_*4),sizeof(kAngleWords),kAngleWords);
 if(!Good()){Release(true);return TransportResultV5::GL_FAILED;}
 // Materialize each specialized program before READY. This is a private cold
 // no-op/JIT warmup, never an accepted SOURCE/quantum or a device deadline
 // claim. Binding0 remains immutable and all scratch outputs are discarded.
 for(uint32_t n=0;n<20;++n){
  Use(n);U("uKappa",0);Common(n==0);U("uPass",0);U("uM3Role",1);U("uM3RootPass",0);
  const uint32_t failed=1;const GLuint work=buffers_[n==0?15:16];
  glBindBuffer(GL_SHADER_STORAGE_BUFFER,work);glBufferSubData(GL_SHADER_STORAGE_BUFFER,0,4,&failed);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER,0,buffers_[0]);glBindBufferBase(GL_SHADER_STORAGE_BUFFER,1,buffers_[1]);glBindBufferBase(GL_SHADER_STORAGE_BUFFER,2,work);glBindBufferBase(GL_SHADER_STORAGE_BUFFER,3,buffers_[8]);
  const auto begin=Now();glDispatchCompute(1,1,1);glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT|GL_BUFFER_UPDATE_BARRIER_BIT);glFinish();const auto end=Now();
  const auto error=glGetError();programcache::Warmup(programs_[n].id,begin,end,error);
  if(error||programcache::Prepared(programs_[n].id)!=GL_NO_ERROR||!Good()){if(error&&!rawGl_)rawGl_=error;Release(true);return TransportResultV5::GL_FAILED;}
 }
 // Restore private state banks changed by the warmup; accepted bank was never
 // bound for writing. The first real root resets all WORK scratch itself.
 for(size_t i=1;i<3;++i){glBindBuffer(GL_SHADER_STORAGE_BUFFER,buffers_[i]);glBufferSubData(GL_SHADER_STORAGE_BUFFER,0,GLsizeiptr(p.stateBytes),state.data());}
 std::array<uint32_t,4096> zero{};glBindBuffer(GL_SHADER_STORAGE_BUFFER,buffers_[15]);
 for(size_t at=0;at<allocation_.bytes[15];at+=sizeof(zero)){const size_t count=std::min(sizeof(zero),allocation_.bytes[15]-at);glBufferSubData(GL_SHADER_STORAGE_BUFFER,GLintptr(at),GLsizeiptr(count),zero.data());}
 if(!Good()){Release(true);return TransportResultV5::GL_FAILED;}
 currentBank_=0;sourceStore_=15;sourcePrefix_=dedupPrefix_=angle_=0;ready_=true;return TransportResultV5::OK;
}
void TransportV5::Common(bool source){
 const auto& p=*profile_;U("uK",p.k);U("uKw",p.kw);U("uPad",old_->pad);U("uWater",16*p.k);U("uColumn",16*(p.k+p.kw));U("uHeader",headerWord_);U("uWords",uint32_t(p.stateBytes/4));U("uM3Words",uint32_t(p.stateBytes/4));
 const auto& v5=*SelectLineageProfile(p.k,p.kw);
 U("uV5Cohort",uint32_t(v5.extension[0].begin/4));U("uV5Depth",uint32_t(v5.extension[1].begin/4));U("uV5Certificate",uint32_t(v5.extension[2].begin/4));U("uV5Aux",uint32_t(v5.extension[3].begin/4));
 const auto origin=uint32_t((source?p.sourceWorkBytes-32768-256:p.quantumWorkBytes-32768-512)/4);
 U("uV5OriginScratch",origin);U("uV5SourceLedger",source?old_->sourceLedger.begin:ledger_);U("uV5SourceMeta",uint32_t((source?p.source[1].begin:p.quantum[0].begin)/4));
 U("uV5CombinedAuxWord",uint32_t(layout_.combinedEventOffset/4)+p.scopeRows*64);U("uV5FrameAuxWord",p.quantumFrameRows*64);U("uV5ParameterRev",config_.lift.revision);Pair("uV5InputEpoch",permit_.identity.inputEpoch);Pair("uV5PageSession",permit_.identity.pageSession);Quad("uV5Sha0",config_.lift.sha.data());Quad("uV5Sha1",config_.lift.sha.data()+4);
 U("uV5EventArena",permit_.eventArena);U("uV5ActualAuxStart",auxStart_);U("uV5Cursor",origin+8192);U("uV5Input",origin+8192+64);U("uV5Mode",uint32_t(config_.lift.mode));U("uV5Rate",config_.lift.liftRateQ16);U("uV5Band",config_.lift.bandQ);U("uV5Ramp",config_.lift.rampQ16);U("uV5Decay",config_.lift.decayQ16);U("uV5Speed",config_.lift.liftSpeedQ);U("uV5Channel",config_.lift.liftCapQ);
 Pair("uNonce",permit_.nonce);Pair("uFrame",permit_.frame);Pair("uControl",permit_.control);
 U("uM3BaseBank",permit_.baseBank);U("uM3CandidateBank",permit_.candidateBank);U("uM3SpareBank",2);U("uM3Carry",permit_.carry);U("uM3PublicationSlot",permit_.publicationSlot);U("uM3EventArena",permit_.eventArena);U("uM3Uploaded",permit_.uploadedBytes);
 if(source){
  U("uM3Scratch",uint32_t(p.source[3].begin/4));U("uM3Input",uint32_t(p.source[2].begin/4));U("uM3Payload",uint32_t(p.source[2].begin/4)+32);U("uM3RawDedup",uint32_t(p.source[0].begin/4));U("uM3SourceMeta",uint32_t(p.source[1].begin/4));U("uM3Dedup",96);U("uM3SourceEvents",old_->sourceLedger.begin);U("uM3OriginScratch",old_->sourceOrigins.begin);U("uM3ExpectedState",96+4096*64);U("uM3Index0",old_->index0.begin);U("uM3Index1",old_->index1.begin);U("uM3SourceRows",p.sourceRows);U("uM3Settle",config_.settle);U("uM3EmitQ",config_.emit);
  const uint32_t at=uint32_t(int32_t(angle_)+7680)*4;I("uM3Sin",int32_t(kAngleWords[at+1]));I("uM3Cos",int32_t(kAngleWords[at+2]));I("uM3Tangent",int32_t(kAngleWords[at]));return;
 }
 const uint32_t scratch=uint32_t(p.quantum[1].begin/4);
 U("uM3Scratch",scratch);U("uM3SourceMeta",uint32_t(p.quantum[0].begin/4));U("uM3SourceEvents",ledger_);U("uSourceEvents",ledger_);U("uM3OriginScratch",old_->quantumOrigins.begin);U("uM3FullBranchClear",captureArmed_?1:0);U("uM3ScopeRows",p.scopeRows);
 U("uSandIndex",sandIndex_);U("uWaterIndex",waterIndex_);U("uArrivalIndex",arrivalIndex_);U("uEdges",edges_);U("uFrozen",frozen_);U("uRows",rows_);U("uAngles",angles_);U("uSpare",ledger_+4096*16);U("uReplayFrozen",ledger_+4096*16+old_->stateWords);U("uM3StateWords",old_->stateWords);U("uM3Header",headerWord_);U("uM3CombinedEventWord",uint32_t(layout_.combinedEventOffset/4));U("uM3SelectedBank",3);U("uM3CompareResult",scratch+80);U("uM3ActualEventStart",permit_.eventStart);U("uM3ExpectedPasses",uint32_t(p.passCount));U("uM3ActualBarriers",uint32_t(p.passCount)+2+(captureArmed_?5:0));
 U("uGravity",config_.gravity);U("uSettle",config_.settle);U("uKAir",config_.kAir);U("uKFlight",config_.kFlight);U("uKWet",config_.kWet);U("uSoft",config_.soft);U("uFlux",config_.flux);U("uM3WaterRate",config_.waterRate);U("uM3Omega",config_.omega);I("uM3Target",target_);U("uQuantumOrdinal",permit_.ordinal);
 Pair("uM3Nonce",permit_.nonce);Pair("uM3Frame",permit_.frame);U("uM3Ordinal",permit_.ordinal);U("uM3BaseSourceLo",uint32_t(permit_.sourceCommit));U("uM3BaseSourceHi",uint32_t(permit_.sourceCommit>>32));U("uM3ProposedPhysicsLo",uint32_t(permit_.physicsCommit+1));U("uM3ProposedPhysicsHi",uint32_t((permit_.physicsCommit+1)>>32));
}
uint32_t TransportV5::PlannedUpload(uint32_t kind){
 if(!profile_||(kind!=1&&kind!=2))return 0;
 const auto previous=uploadCounter_,program=activeProgram_;dryUniforms_=true;uploadCounter_=kind==1?2176:256;
 if(kind==1){Use(0);Common(true);uploadCounter_+=72*12;}
 else{for(uint32_t p=1;p<20;++p){Use(p);Common(false);}uploadCounter_+=uint32_t(profile_->passCount)*48;}
 const auto result=uploadCounter_;uploadCounter_=previous;activeProgram_=program;dryUniforms_=false;return result;
}
bool TransportV5::ReserveFullCredit(){
 if(!config_.fullBranchTrace||!profile_||captureArmed_||!fullFrozen_)return false;
 const size_t flow=size_t(profile_->scopeRows)*256+size_t(SelectLineageProfile(profile_->k,profile_->kw)->ea)*128;
 const size_t extra=20*flow+5*(profile_->stateBytes+10224000)+40960+256;
 return captureGpu_.Reserve(uint64_t(reinterpret_cast<uintptr_t>(context_)),allocation_.total+extra);
}
bool TransportV5::ArmFullCapture(){
 if(!captureGpu_.Held()||busy_||!Current()||captureArmed_)return false;
 GLint64 maximum=0;glGetInteger64v(GL_MAX_SHADER_STORAGE_BLOCK_SIZE,&maximum);
 const size_t flow=size_t(profile_->scopeRows)*256+size_t(SelectLineageProfile(profile_->k,profile_->kw)->ea)*128;
 if(maximum<=0||std::max({flow,size_t(10224000),profile_->stateBytes,size_t(40960)})>size_t(maximum)){captureFailure_="FULL_UNSUPPORTED_RESOURCE";captureGpu_.ReleaseAfterFenceDelete();return false;}
 glGenBuffers(GLsizei(fullBuffers_.size()),fullBuffers_.data());
 for(size_t i=0;i<fullBuffers_.size();++i){glBindBuffer(GL_SHADER_STORAGE_BUFFER,fullBuffers_[i]);glBufferData(GL_SHADER_STORAGE_BUFFER,GLsizeiptr(i<20?flow:i==20?40960:i==31?256:(i%2?profile_->stateBytes:10224000)),nullptr,GL_DYNAMIC_COPY);}
 if(!Good()){glDeleteBuffers(GLsizei(fullBuffers_.size()),fullBuffers_.data());fullBuffers_={};captureGpu_.ReleaseAfterFenceDelete();captureFailure_="FULL_ALLOCATION_GL_FAILED";return false;}
 captureArmed_=true;fullPlan_=0;fullFrozen_=true;captureRoots_={};captureSource_={};baseMask_=enteredMask_=fullMask_=fullQuantumMask_=pendingFullMask_=0;return true;
}
void TransportV5::RetireFullCapture(){
 if(busy_||!Current())return;
 glDeleteBuffers(GLsizei(fullBuffers_.size()),fullBuffers_.data());fullBuffers_={};captureGpu_.ReleaseAfterFenceDelete();captureArmed_=false;fullFrozen_=true;
}
void TransportV5::SealCaptureRoot(bool accepted,const QuotaSnapshotV3& quota){
 pendingRoot_.completion=tail_;pendingRoot_.evidence=evidence_;pendingRoot_.quota=quota;pendingRoot_.checked=Now();pendingRoot_.complete=mapped_||tail_[0]==1381192263u;pendingRoot_.accepted=accepted;
 if(captureArmed_){const auto root=permit_.kind==1?0:permit_.ordinal;if(root<5)captureRoots_[root]=pendingRoot_;}
 if(accepted){acceptedRoot_=pendingRoot_;if(permit_.kind==1)acceptedSource_=captureSource_;}
}
TransportResultV5 TransportV5::Begin(const RootPermitV5& permit,int32_t target,const SourcePacketV5* packet,uint32_t sourcePrefix,uint32_t dedupPrefix,Authorize authorize,void* user){
 if(!ready_||busy_||!Current())return TransportResultV5::WOULD_BLOCK;
 if(permit.baseBank!=currentBank_||permit.candidateBank!=1-currentBank_||sourcePrefix>4096||dedupPrefix>4096||target< -7680||target>7680||!authorize||(permit.kind==1)!=bool(packet))return TransportResultV5::INVALID;
 permit_=permit;target_=target;authorize_=authorize;authorization_=user;if(!Allowed())return Denied();
 if(captureArmed_){
  if(fullPlan_!=permit.plan){if(fullPlan_&&!fullFrozen_)return TransportResultV5::WOULD_BLOCK;fullPlan_=permit.plan;fullMask_=fullQuantumMask_=baseMask_=enteredMask_=0;fullFrozen_=false;}
  const uint32_t root=permit.kind==1?0:permit.ordinal;if(root>4)return TransportResultV5::INVALID;
  Copy(buffers_[permit.baseBank],0,fullBuffers_[21+root*2],0,profile_->stateBytes/4);
  const GLuint dst=fullBuffers_[22+root*2];size_t at=0;
  const size_t starts[]={0,96,old_->sourceLedger.begin,profile_->source[0].begin/4,profile_->source[1].begin/4};
  const size_t counts[]={96,4096*64,4096*16,4096*512,4096*32};
  for(size_t j=0;j<5;++j){Copy(buffers_[15],starts[j],dst,at,counts[j]);at+=counts[j];}Barrier();if(!Good())return TransportResultV5::GL_FAILED;baseMask_|=1u<<root;
  pendingFullMask_=0;
 }
 auxStart_=permit.frame==auxFrame_?auxEnd_:0;
 evidence_={};tail_={};nextPass_=0;mapped_=false;sourcePrefix_=sourcePrefix;dedupPrefix_=dedupPrefix;
 auto entry=pendingRoot_.cursor;auto entryScope=pendingRoot_.scope;pendingRoot_={};pendingRoot_.cursor=entry;pendingRoot_.scope=entryScope;pendingRoot_.permit=permit;pendingRoot_.input=liftInput_;pendingRoot_.quota=pendingQuota_;pendingRoot_.started=pendingStarted_;pendingRoot_.sourceGeneration=pendingSourceGeneration_;pendingRoot_.entered=false;
 if(captureArmed_){const auto root=permit.kind==1?0:permit.ordinal;captureRoots_[root]=pendingRoot_;if(packet)captureSource_=*packet;}

 if(permit.kind==1){
  captureSource_=*packet;
  glBindBuffer(GL_SHADER_STORAGE_BUFFER,buffers_[15]);glBufferSubData(GL_SHADER_STORAGE_BUFFER,GLintptr(profile_->source[2].begin),sizeof(packet->words),packet->words.data());
 }else{
  const auto inputAt=profile_->quantumWorkBytes-256;glBindBuffer(GL_SHADER_STORAGE_BUFFER,buffers_[16]);glBufferSubData(GL_SHADER_STORAGE_BUFFER,GLintptr(inputAt),256,liftInput_.data());
  Copy(buffers_[15],old_->sourceLedger.begin,buffers_[16],ledger_,size_t(sourcePrefix)*16);
  Copy(buffers_[15],profile_->source[1].begin/4,buffers_[16],profile_->quantum[0].begin/4,size_t(sourcePrefix)*32);
  Barrier();
 }
 uploadCounter_=permit.kind==1?2176:256;
 if(permit.kind==1){Use(0);Common(true);}else for(uint32_t p=1;p<20;++p){Use(p);Common(false);}
 if(!Good())return TransportResultV5::GL_FAILED;busy_=true;return TransportResultV5::OK;
}
void TransportV5::SourcePass(uint32_t pass){
 Use(0);uint32_t in=permit_.baseBank,out=permit_.candidateBank,event=22+permit_.eventArena;
 bool replay=pass>=2&&pass<66&&(pass-2)%8==6;
 if(replay){in=permit_.candidateBank;out=2;}else if(pass>=2&&pass<66&&(pass-2)%8==5)out=2;
 if(pass==67)out=2;
 // Private candidate is already in ACTUAL. The fixed promotion slot copies
 // through the independently audited SPARE; CURRENT remains read-only.
 if(pass==68){in=2;out=permit_.candidateBank;}
 if(pass==69||pass==70)event=3+permit_.publicationSlot;
 if(pass==71)event=11+permit_.publicationSlot;
 glBindBufferBase(GL_SHADER_STORAGE_BUFFER,0,buffers_[in]);glBindBufferBase(GL_SHADER_STORAGE_BUFFER,1,buffers_[out]);glBindBufferBase(GL_SHADER_STORAGE_BUFFER,2,buffers_[15]);glBindBufferBase(GL_SHADER_STORAGE_BUFFER,3,buffers_[event]);
 U("uPass",pass);U("uM3Trial",pass>=2&&pass<66?(pass-2)/8:0);U("uM3ReplayPass",replay?1:0);
 glDispatchCompute(1,1,1);++evidence_.submittedPasses;Barrier();
}
void TransportV5::QuantumPass(uint32_t index){
 const auto& pass=profile_->passes[index];Use(1+uint32_t(pass.opcode));
 GLuint in=buffers_[permit_.baseBank],out=buffers_[permit_.candidateBank],event=buffers_[25+permit_.eventArena];
 const bool core=pass.opcode==Opcode::OCCUPIED_SCOPE_CORE;
 const bool scope=uint32_t(pass.opcode)>=15||core||pass.opcode==Opcode::SCOPE_INIT||pass.opcode==Opcode::WATER_TARGET_40||pass.opcode==Opcode::WATER_MATCH_ORDERED||pass.opcode==Opcode::WATER_PROJECTION_AUDIT;
 if(scope){out=pass.role==4?buffers_[permit_.candidateBank]:buffers_[21];event=pass.role==4?buffers_[8+permit_.eventArena]:buffers_[21];if(core&&(pass.nativeStage==22||pass.nativeStage==23))out=buffers_[2];}
 if(pass.opcode==Opcode::SCOPE_FINALIZE){in=pass.role==4?buffers_[20]:buffers_[21];out=pass.role==4?buffers_[permit_.candidateBank]:buffers_[20];event=pass.role==4?buffers_[8+permit_.eventArena]:buffers_[25+permit_.eventArena];}
 if(pass.opcode==Opcode::SEMANTIC_COMPARE||pass.opcode==Opcode::PRIVATE_PROMOTE){in=buffers_[20];out=buffers_[21];event=buffers_[permit_.candidateBank];}
 if(pass.opcode==Opcode::PACK_PUBLICATION||pass.opcode==Opcode::PUBLICATION_AUDIT)event=buffers_[3+permit_.publicationSlot];
 if(pass.opcode==Opcode::SEAL_COMPLETION)event=buffers_[11+permit_.publicationSlot];
 glBindBufferBase(GL_SHADER_STORAGE_BUFFER,0,in);glBindBufferBase(GL_SHADER_STORAGE_BUFFER,1,out);glBindBufferBase(GL_SHADER_STORAGE_BUFFER,2,buffers_[16]);glBindBufferBase(GL_SHADER_STORAGE_BUFFER,3,event);
 U("uV5AuxWord",scope&&pass.role!=4?uint32_t(layout_.combinedEventOffset/4)+profile_->scopeRows*64:profile_->quantumFrameRows*64);U("uV5AuxCapacity",SelectLineageProfile(profile_->k,profile_->kw)->ea*(pass.role==4?4:1));
 U("uM3RootPass",index);U("uM3Role",pass.role);U("uM3WriteBank",pass.role==4?permit_.candidateBank:4);U("uPass",pass.nativeStage);U("uSortK",pass.sortK);U("uSortJ",pass.sortJ);U("uColor",pass.color);
 U("uEventStart",pass.role==4?permit_.eventStart:0);U("uM3EventStart",pass.role==4?permit_.eventStart:0);U("uEventCapacity",pass.role==4||pass.role==0?profile_->quantumFrameRows:profile_->scopeRows);U("uM3EventCapacity",pass.role==4||pass.role==0?profile_->quantumFrameRows:profile_->scopeRows);U("uM3EventWord",scope&&pass.role!=4?uint32_t(layout_.combinedEventOffset/4):0);
 glDispatchCompute((pass.count+63)/64,1,1); // Original dispatch; only read-only copies below are added.
 if(captureArmed_&&(pass.opcode==Opcode::INPUT_E_REQUEST||pass.opcode==Opcode::SCOPE_FINALIZE)&&pass.role>=1&&pass.role<=5){
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT|GL_BUFFER_UPDATE_BARRIER_BIT);
  const size_t cursor=(profile_->quantumWorkBytes-32768-512)/4+8192;
  Copy(buffers_[16],cursor,fullBuffers_[20],size_t(permit_.ordinal)*2048+768+64*(pass.role-1)+(pass.opcode==Opcode::SCOPE_FINALIZE?32:0),32);
 }
++evidence_.submittedPasses;Barrier();
 if(captureArmed_&&pass.opcode==Opcode::SCOPE_FINALIZE){
  const uint32_t slot=(permit_.ordinal-1)*5+pass.role-1;
  Copy(pass.role==4?buffers_[8+permit_.eventArena]:buffers_[21],pass.role==4?size_t(permit_.eventStart)*64:layout_.combinedEventOffset/4,fullBuffers_[slot],0,size_t(profile_->scopeRows)*64);
  Copy(pass.role==4?buffers_[8+permit_.eventArena]:buffers_[21],pass.role==4?size_t(profile_->quantumFrameRows)*64+size_t(auxStart_)*32:layout_.combinedEventOffset/4+profile_->scopeRows*64,fullBuffers_[slot],size_t(profile_->scopeRows)*64,size_t(SelectLineageProfile(profile_->k,profile_->kw)->ea)*32);Barrier();pendingFullMask_|=1u<<slot;
 }
 if(pass.opcode==Opcode::PRIVATE_PROMOTE){
  Copy(buffers_[16],profile_->quantum[1].begin/4+6144,buffers_[25+permit_.eventArena],0,6*128);
  if(config_.fullBranchTrace)Copy(buffers_[16],profile_->quantum[1].begin/4+6144,fullBuffers_[20],size_t(permit_.ordinal)*2048,6*128);
  Barrier();
 }
}
TransportResultV5 TransportV5::Pump(){
 if(!busy_)return TransportResultV5::INVALID;if(!Current())return TransportResultV5::CONTEXT_LOST;
 if(!Allowed())return Denied();
 if(mapped_)return TransportResultV5::OK;
 const uint32_t count=permit_.kind==1?72:uint32_t(profile_->passCount);
 if(!fence_){
  const auto begin=Now();uint32_t submitted=0;
  while(nextPass_<count&&submitted<10&&Now()-begin<8000000){
   if(!Allowed())return Denied();
   if(permit_.kind==1)SourcePass(nextPass_);else QuantumPass(nextPass_);++nextPass_;++submitted;
   if(!Good())return TransportResultV5::GL_FAILED;if(nextPass_==1){pendingRoot_.entered=true;pendingRoot_.cursor[15]|=2u;const uint32_t root=permit_.kind==1?0:permit_.ordinal;enteredMask_|=1u<<root;if(captureArmed_)captureRoots_[root]=pendingRoot_;}
  }
  if(!Allowed())return Denied();glFlush();if(!Good())return TransportResultV5::GL_FAILED;
  if(nextPass_!=count)return TransportResultV5::IN_PROGRESS;
  if(!Allowed())return Denied();fence_=glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE,0);glFlush();if(!fence_||!Good())return TransportResultV5::GL_FAILED;
 }
 if(!Allowed())return Denied();
 const auto wait=glClientWaitSync(fence_,0,0);if(!Good()||wait==GL_WAIT_FAILED)return TransportResultV5::GL_FAILED;
 if(wait==GL_TIMEOUT_EXPIRED)return TransportResultV5::IN_PROGRESS;
 evidence_.fenceSignalledNs=Now();if(!Allowed())return Denied();
 glBindBuffer(GL_COPY_READ_BUFFER,buffers_[11+permit_.publicationSlot]);auto mapped=glMapBufferRange(GL_COPY_READ_BUFFER,0,1024,GL_MAP_READ_BIT);if(!mapped||!Good())return TransportResultV5::GL_FAILED;
 std::memcpy(tail_.data(),mapped,1024);const bool unmapped=glUnmapBuffer(GL_COPY_READ_BUFFER)==GL_TRUE;if(!unmapped||!Good())return TransportResultV5::GL_FAILED;
 glDeleteSync(fence_);fence_=nullptr;evidence_.uploadedBytes=uploadCounter_;evidence_.gl=rawGl_;evidence_.egl=rawEgl_==EGL_SUCCESS?0:uint32_t(rawEgl_);mapped_=true;return TransportResultV5::OK;
}
bool TransportV5::Commit(const RootPermitV5& permit,const Words256& tail){
 if(!busy_||!mapped_||permit.nonce!=permit_.nonce||tail!=tail_||tail[28]<1||tail[28]>3)return false;
 if(captureArmed_){fullMask_|=pendingFullMask_;if(permit.kind==2)fullQuantumMask_|=1u<<(permit.ordinal-1);}
 if(permit.kind==2){auxFrame_=permit.frame;auxEnd_=tail[207];}
 currentBank_=permit.candidateBank;sourcePrefix_=tail[56];dedupPrefix_=tail[58];angle_=tail[107];busy_=mapped_=false;return true;
}
bool TransportV5::RetireRejected(const RootPermitV5& permit){
 if(!busy_||permit.nonce!=permit_.nonce)return false;
 if(mapped_){if(captureArmed_&&tail_[28]==5){fullMask_|=pendingFullMask_;fullQuantumMask_|=1u<<(permit_.ordinal-1);}busy_=mapped_=false;return true;}
 if(!Current())return false;
 if(fence_){const auto result=glClientWaitSync(fence_,0,0);if(result!=GL_ALREADY_SIGNALED&&result!=GL_CONDITION_SATISFIED)return false;glDeleteSync(fence_);fence_=nullptr;}
 else if(nextPass_&&!mapped_){fence_=glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE,0);glFlush();return false;}
 busy_=mapped_=false;return Good();
}
void TransportV5::Release(bool current){
 current=current&&Current();if(current){if(fence_)glDeleteSync(fence_);glDeleteBuffers(GLsizei(buffers_.size()),buffers_.data());glDeleteBuffers(GLsizei(fullBuffers_.size()),fullBuffers_.data());}
 // Unmapped completed roots may retire by owner deletion; otherwise credit
 // remains pinned until the bridge acknowledges actual context destruction.
 if(current&&!busy_)captureGpu_.ReleaseAfterFenceDelete();else captureGpu_.ContextLost();
 captureArmed_=false;
 fence_=nullptr;buffers_={};fullBuffers_={};for(auto& p:programs_){if(p.id){programcache::Forget(p.id);if(current)glDeleteProgram(p.id);}p={};}
 captureRoots_={};pendingRoot_={};acceptedRoot_={};captureSource_={};acceptedSource_={};
 ready_=busy_=mapped_=false;profile_=nullptr;old_=nullptr;context_=EGL_NO_CONTEXT;
}
}
