#include "transport.h"
#include "kernel_assets.h"
#include "shader_variant.h"
#include "cold_contract.h"
#include "../angles_generated.h"
#include "../program_cache/program_factory.h"
#include <chrono>
#include <cstring>
#include <string>
namespace grs::m3a {
namespace {
uint64_t Now(){return uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());}
uint64_t Value(const Words256& w,size_t at){return uint64_t(w[at])|(uint64_t(w[at+1])<<32);}
}
bool TransportV4::Current()const{return context_!=EGL_NO_CONTEXT&&eglGetCurrentContext()==context_&&eglGetCurrentDisplay()==display_&&eglGetCurrentSurface(EGL_DRAW)==surface_&&thread_==std::this_thread::get_id();}
bool TransportV4::Good(){auto gl=glGetError();auto egl=eglGetError();if(gl&&!rawGl_)rawGl_=gl;if(egl!=EGL_SUCCESS&&rawEgl_==EGL_SUCCESS)rawEgl_=egl;return !rawGl_&&rawEgl_==EGL_SUCCESS;}
bool TransportV4::Allowed(){return Current()&&Now()<=permit_.deadline&&authorize_&&authorize_(authorization_,permit_,Now());}
TransportResultV4 TransportV4::Denied()const{return !Current()?TransportResultV4::CONTEXT_LOST:Now()>permit_.deadline?TransportResultV4::DEADLINE:TransportResultV4::STALE;}
void TransportV4::Use(uint32_t index){activeProgram_=index;if(!dryUniforms_)glUseProgram(programs_[index].id);}
void TransportV4::U(const char* name,uint32_t value){uploadCounter_+=4;if(!dryUniforms_)glUniform1ui(programs_[activeProgram_].locations.Find(name),value);}
void TransportV4::I(const char* name,int32_t value){uploadCounter_+=4;if(!dryUniforms_)glUniform1i(programs_[activeProgram_].locations.Find(name),value);}
void TransportV4::Pair(const char* name,uint64_t value){uploadCounter_+=8;if(!dryUniforms_)glUniform2ui(programs_[activeProgram_].locations.Find(name),uint32_t(value),uint32_t(value>>32));}
void TransportV4::Barrier(){glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT|GL_BUFFER_UPDATE_BARRIER_BIT);++evidence_.barriers;}
void TransportV4::Copy(GLuint from,size_t fromWord,GLuint to,size_t toWord,size_t words){
 glBindBuffer(GL_COPY_READ_BUFFER,from);glBindBuffer(GL_COPY_WRITE_BUFFER,to);glCopyBufferSubData(GL_COPY_READ_BUFFER,GL_COPY_WRITE_BUFFER,GLintptr(fromWord*4),GLintptr(toWord*4),GLsizeiptr(words*4));
}
TransportResultV4 TransportV4::Create(const Profile& p,const ControlIdentityV4& id,uint64_t geometry,const PhysicalConfigV4& config,
 const std::array<uint32_t,8>& shader,const std::array<uint32_t,8>& angle,const std::array<uint32_t,8>& layoutSha){
 if(busy_||ready_||!config.Valid()||!id.run||!geometry)return TransportResultV4::INVALID;
 context_=eglGetCurrentContext();display_=eglGetCurrentDisplay();surface_=eglGetCurrentSurface(EGL_DRAW);thread_=std::this_thread::get_id();if(!Current())return TransportResultV4::CONTEXT_LOST;
 GLint alignment=0,bindings=0,blocks=0,fragment=0,invocations=0,shared=0;GLint64 maximum=0;
 glGetIntegerv(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT,&alignment);glGetInteger64v(GL_MAX_SHADER_STORAGE_BLOCK_SIZE,&maximum);glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS,&bindings);glGetIntegerv(GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS,&blocks);glGetIntegerv(GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS,&fragment);glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS,&invocations);glGetIntegerv(GL_MAX_COMPUTE_SHARED_MEMORY_SIZE,&shared);
 if(alignment<=0||maximum<=0||!WorkLayout::Make(p.k,p.kw,size_t(alignment),layout_)||!layout_.Supports(size_t(maximum),bindings,blocks,fragment,invocations,shared)||!ResidentBuffersV4::Make(p,size_t(alignment),allocation_))return TransportResultV4::UNSUPPORTED;
 for(auto n:allocation_.bytes)if(n>size_t(maximum)||n>size_t(PTRDIFF_MAX))return TransportResultV4::UNSUPPORTED;
 alignment_=size_t(alignment);profile_=&p;for(const auto& old:m2::kProfiles)if(old.k==p.k&&old.kw==p.kw)old_=&old;
 if(!old_)return TransportResultV4::INVALID;config_=config;
 sandIndex_=32;waterIndex_=sandIndex_+old_->pad;arrivalIndex_=waterIndex_+p.kw;edges_=arrivalIndex_+p.kw;frozen_=edges_+127*16;rows_=frozen_+512;angles_=rows_+p.k+p.kw;ledger_=angles_+61444;
 for(uint32_t n=0;n<16;++n){
  std::string source=n==0?SourceKernelV4():QuantumKernelV4();
  if(n){const auto at=source.find('\n');if(at==std::string::npos)return TransportResultV4::SHADER_FAILED;source.insert(at+1,kOpcodeDefines[n-1]);}
  if(source.size()>kMaximumKernelSourceBytes)return TransportResultV4::UNSUPPORTED;
  GLenum original=0;auto& program=programs_[n];
  // One existing T475 Store; exact specialized bytes and independent V4 ABI
  // enter the key. No second file worker, CPU decision or global route patch.
  if(!programcache::Build(n==0?0:1,source.c_str(),programcache::Hash(source).c_str(),"GRS_M3_V4_FEATURES15_FOUR_SSBO",program.id,original)||!programcache::Reflect(program.locations,program.id)){if(original&&!rawGl_)rawGl_=original;Release(true);return TransportResultV4::SHADER_FAILED;}
 }
 glGenBuffers(GLsizei(buffers_.size()),buffers_.data());
 for(size_t i=0;i<buffers_.size();++i){glBindBuffer(GL_SHADER_STORAGE_BUFFER,buffers_[i]);glBufferData(GL_SHADER_STORAGE_BUFFER,GLsizeiptr(allocation_.bytes[i]),nullptr,GL_DYNAMIC_COPY);if(!Good()){Release(true);return TransportResultV4::GL_FAILED;}}
 if(config_.fullBranchTrace){
  const size_t flowBytes=size_t(p.scopeRows)*256;
  if(flowBytes>size_t(maximum)||32768>maximum){Release(true);return TransportResultV4::UNSUPPORTED;}
  glGenBuffers(GLsizei(fullBuffers_.size()),fullBuffers_.data());
  for(size_t i=0;i<fullBuffers_.size();++i){glBindBuffer(GL_SHADER_STORAGE_BUFFER,fullBuffers_[i]);glBufferData(GL_SHADER_STORAGE_BUFFER,GLsizeiptr(i==20?32768:flowBytes),nullptr,GL_DYNAMIC_COPY);if(!Good()){Release(true);return TransportResultV4::GL_FAILED;}}
 }
 // A fresh independent run, initialized only once at cold creation.
 std::vector<uint32_t> state(p.stateBytes/4);const uint32_t h=uint32_t(state.size())-256;
 auto pair=[&](uint32_t at,uint64_t value){state[h+at]=uint32_t(value);state[h+at+1]=uint32_t(value>>32);};
 state[h]=1213420103;state[h+1]=4;state[h+2]=256;pair(4,id.run);pair(6,id.owner);pair(8,id.context);pair(10,id.surface);pair(20,geometry);pair(22,id.config);pair(84,id.window);
 const uint32_t versions[]={1,1,1,3,4,4,4,15,0};for(size_t i=0;i<9;++i)state[h+24+i]=versions[i];
 state[h+39]=state[h+40]=1;state[h+43]=p.k;state[h+44]=p.kw;state[h+122]=1;state[h+123]=65536;state[h+124]=state[h+125]=2;state[h+145]=1;state[h+146]=7;state[h+156]=512;state[h+160]=1;
 for(size_t i=0;i<8;++i){state[h+88+i]=shader[i];state[h+96+i]=angle[i];state[h+104+i]=layoutSha[i];}
 for(size_t i=0;i<3;++i){glBindBuffer(GL_SHADER_STORAGE_BUFFER,buffers_[i]);glBufferSubData(GL_SHADER_STORAGE_BUFFER,0,GLsizeiptr(p.stateBytes),state.data());}
 glBindBuffer(GL_SHADER_STORAGE_BUFFER,buffers_[16]);glBufferSubData(GL_SHADER_STORAGE_BUFFER,GLintptr(angles_*4),sizeof(kAngleWords),kAngleWords);
 if(!Good()){Release(true);return TransportResultV4::GL_FAILED;}
 // Materialize each specialized program before READY. This is a private cold
 // no-op/JIT warmup, never an accepted SOURCE/quantum or a device deadline
 // claim. Binding0 remains immutable and all scratch outputs are discarded.
 for(uint32_t n=0;n<16;++n){
  Use(n);Common(n==0);U("uPass",0);U("uM3Role",1);U("uM3RootPass",0);
  const uint32_t failed=1;const GLuint work=buffers_[n==0?15:16];
  glBindBuffer(GL_SHADER_STORAGE_BUFFER,work);glBufferSubData(GL_SHADER_STORAGE_BUFFER,0,4,&failed);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER,0,buffers_[0]);glBindBufferBase(GL_SHADER_STORAGE_BUFFER,1,buffers_[1]);glBindBufferBase(GL_SHADER_STORAGE_BUFFER,2,work);glBindBufferBase(GL_SHADER_STORAGE_BUFFER,3,buffers_[8]);
  const auto begin=Now();glDispatchCompute(1,1,1);glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT|GL_BUFFER_UPDATE_BARRIER_BIT);glFinish();const auto end=Now();
  const auto error=glGetError();programcache::Warmup(programs_[n].id,begin,end,error);
  if(error||programcache::Prepared(programs_[n].id)!=GL_NO_ERROR||!Good()){if(error&&!rawGl_)rawGl_=error;Release(true);return TransportResultV4::GL_FAILED;}
 }
 // Restore private state banks changed by the warmup; accepted bank was never
 // bound for writing. The first real root resets all WORK scratch itself.
 for(size_t i=1;i<3;++i){glBindBuffer(GL_SHADER_STORAGE_BUFFER,buffers_[i]);glBufferSubData(GL_SHADER_STORAGE_BUFFER,0,GLsizeiptr(p.stateBytes),state.data());}
 currentBank_=0;sourceStore_=15;sourcePrefix_=dedupPrefix_=angle_=0;ready_=true;return TransportResultV4::OK;
}
void TransportV4::Common(bool source){
 const auto& p=*profile_;U("uK",p.k);U("uKw",p.kw);U("uPad",old_->pad);U("uWater",16*p.k);U("uColumn",16*(p.k+p.kw));U("uHeader",uint32_t(p.stateBytes/4)-256);U("uWords",uint32_t(p.stateBytes/4));U("uM3Words",uint32_t(p.stateBytes/4));
 Pair("uNonce",permit_.nonce);Pair("uFrame",permit_.frame);Pair("uControl",permit_.control);
 U("uM3BaseBank",permit_.baseBank);U("uM3CandidateBank",permit_.candidateBank);U("uM3SpareBank",2);U("uM3Carry",permit_.carry);U("uM3PublicationSlot",permit_.publicationSlot);U("uM3EventArena",permit_.eventArena);U("uM3Uploaded",permit_.uploadedBytes);
 if(source){
  U("uM3Scratch",uint32_t(p.source[3].begin/4));U("uM3Input",uint32_t(p.source[2].begin/4));U("uM3Payload",uint32_t(p.source[2].begin/4)+32);U("uM3RawDedup",uint32_t(p.source[0].begin/4));U("uM3SourceMeta",uint32_t(p.source[1].begin/4));U("uM3Dedup",96);U("uM3SourceEvents",old_->sourceLedger.begin);U("uM3OriginScratch",old_->sourceOrigins.begin);U("uM3ExpectedState",96+4096*64);U("uM3Index0",old_->index0.begin);U("uM3Index1",old_->index1.begin);U("uM3SourceRows",p.sourceRows);U("uM3Settle",config_.settle);U("uM3EmitQ",config_.emit);
  const uint32_t at=uint32_t(int32_t(angle_)+7680)*4;I("uM3Sin",int32_t(kAngleWords[at+1]));I("uM3Cos",int32_t(kAngleWords[at+2]));I("uM3Tangent",int32_t(kAngleWords[at]));return;
 }
 const uint32_t scratch=uint32_t(p.quantum[1].begin/4);
 U("uM3Scratch",scratch);U("uM3SourceMeta",uint32_t(p.quantum[0].begin/4));U("uM3SourceEvents",ledger_);U("uSourceEvents",ledger_);U("uM3OriginScratch",old_->quantumOrigins.begin);U("uM3FullBranchClear",config_.fullBranchTrace?1:0);U("uM3ScopeRows",p.scopeRows);
 U("uSandIndex",sandIndex_);U("uWaterIndex",waterIndex_);U("uArrivalIndex",arrivalIndex_);U("uEdges",edges_);U("uFrozen",frozen_);U("uRows",rows_);U("uAngles",angles_);U("uSpare",ledger_+4096*16);U("uReplayFrozen",ledger_+4096*16+old_->stateWords);U("uM3StateWords",old_->stateWords);U("uM3Header",old_->stateWords-256);U("uM3CombinedEventWord",uint32_t(layout_.combinedEventOffset/4));U("uM3SelectedBank",3);U("uM3CompareResult",scratch+80);U("uM3ActualEventStart",permit_.eventStart);U("uM3ExpectedPasses",uint32_t(p.passCount));U("uM3ActualBarriers",uint32_t(p.passCount)+2+(config_.fullBranchTrace?5:0));
 U("uGravity",config_.gravity);U("uSettle",config_.settle);U("uKAir",config_.kAir);U("uKFlight",config_.kFlight);U("uKWet",config_.kWet);U("uSoft",config_.soft);U("uFlux",config_.flux);U("uM3WaterRate",config_.waterRate);U("uM3Omega",config_.omega);I("uM3Target",target_);U("uQuantumOrdinal",permit_.ordinal);
 Pair("uM3Nonce",permit_.nonce);Pair("uM3Frame",permit_.frame);U("uM3Ordinal",permit_.ordinal);U("uM3BaseSourceLo",uint32_t(permit_.sourceCommit));U("uM3BaseSourceHi",uint32_t(permit_.sourceCommit>>32));U("uM3ProposedPhysicsLo",uint32_t(permit_.physicsCommit+1));U("uM3ProposedPhysicsHi",uint32_t((permit_.physicsCommit+1)>>32));
}
uint32_t TransportV4::PlannedUpload(uint32_t kind){
 if(!profile_||(kind!=1&&kind!=2))return 0;
 const auto previous=uploadCounter_,program=activeProgram_;dryUniforms_=true;uploadCounter_=kind==1?2176:0;
 if(kind==1){Use(0);Common(true);uploadCounter_+=72*12;}
 else{for(uint32_t p=1;p<16;++p){Use(p);Common(false);}uploadCounter_+=uint32_t(profile_->passCount)*48;}
 const auto result=uploadCounter_;uploadCounter_=previous;activeProgram_=program;dryUniforms_=false;return result;
}
TransportResultV4 TransportV4::Begin(const RootPermitV4& permit,int32_t target,const SourcePacketV4* packet,uint32_t sourcePrefix,uint32_t dedupPrefix,Authorize authorize,void* user){
 if(!ready_||busy_||!Current())return TransportResultV4::WOULD_BLOCK;
 if(permit.baseBank!=currentBank_||permit.candidateBank!=1-currentBank_||sourcePrefix>4096||dedupPrefix>4096||target< -7680||target>7680||!authorize||(permit.kind==1)!=bool(packet))return TransportResultV4::INVALID;
 permit_=permit;target_=target;authorize_=authorize;authorization_=user;if(!Allowed())return Denied();
 if(config_.fullBranchTrace){
  if(fullPlan_!=permit.plan){if(fullPlan_&&!fullFrozen_)return TransportResultV4::WOULD_BLOCK;fullPlan_=permit.plan;fullMask_=fullQuantumMask_=0;fullFrozen_=false;}
  pendingFullMask_=0;
 }
 evidence_={};tail_={};nextPass_=0;mapped_=false;sourcePrefix_=sourcePrefix;dedupPrefix_=dedupPrefix;
 if(permit.kind==1){
  glBindBuffer(GL_SHADER_STORAGE_BUFFER,buffers_[15]);glBufferSubData(GL_SHADER_STORAGE_BUFFER,GLintptr(profile_->source[2].begin),sizeof(packet->words),packet->words.data());
 }else{
  Copy(buffers_[15],old_->sourceLedger.begin,buffers_[16],ledger_,size_t(sourcePrefix)*16);
  Copy(buffers_[15],profile_->source[1].begin/4,buffers_[16],profile_->quantum[0].begin/4,size_t(sourcePrefix)*32);
  Barrier();
 }
 uploadCounter_=permit.kind==1?2176:0;
 if(permit.kind==1){Use(0);Common(true);}else for(uint32_t p=1;p<16;++p){Use(p);Common(false);}
 if(!Good())return TransportResultV4::GL_FAILED;busy_=true;return TransportResultV4::OK;
}
void TransportV4::SourcePass(uint32_t pass){
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
void TransportV4::QuantumPass(uint32_t index){
 const auto& pass=profile_->passes[index];Use(1+uint32_t(pass.opcode));
 GLuint in=buffers_[permit_.baseBank],out=buffers_[permit_.candidateBank],event=buffers_[25+permit_.eventArena];
 const bool core=pass.opcode==Opcode::OCCUPIED_SCOPE_CORE;
 const bool scope=core||pass.opcode==Opcode::SCOPE_INIT||pass.opcode==Opcode::WATER_TARGET_40||pass.opcode==Opcode::WATER_MATCH_ORDERED||pass.opcode==Opcode::WATER_PROJECTION_AUDIT;
 if(scope){out=pass.role==4?buffers_[permit_.candidateBank]:buffers_[21];event=pass.role==4?buffers_[8+permit_.eventArena]:buffers_[21];if(core&&(pass.nativeStage==22||pass.nativeStage==23))out=buffers_[2];}
 if(pass.opcode==Opcode::SCOPE_FINALIZE){in=pass.role==4?buffers_[20]:buffers_[21];out=pass.role==4?buffers_[permit_.candidateBank]:buffers_[20];event=pass.role==4?buffers_[8+permit_.eventArena]:buffers_[25+permit_.eventArena];}
 if(pass.opcode==Opcode::SEMANTIC_COMPARE||pass.opcode==Opcode::PRIVATE_PROMOTE){in=buffers_[20];out=buffers_[21];event=buffers_[permit_.candidateBank];}
 if(pass.opcode==Opcode::PACK_PUBLICATION||pass.opcode==Opcode::PUBLICATION_AUDIT)event=buffers_[3+permit_.publicationSlot];
 if(pass.opcode==Opcode::SEAL_COMPLETION)event=buffers_[11+permit_.publicationSlot];
 glBindBufferBase(GL_SHADER_STORAGE_BUFFER,0,in);glBindBufferBase(GL_SHADER_STORAGE_BUFFER,1,out);glBindBufferBase(GL_SHADER_STORAGE_BUFFER,2,buffers_[16]);glBindBufferBase(GL_SHADER_STORAGE_BUFFER,3,event);
 U("uM3RootPass",index);U("uM3Role",pass.role);U("uM3WriteBank",pass.role==4?permit_.candidateBank:4);U("uPass",pass.nativeStage);U("uSortK",pass.sortK);U("uSortJ",pass.sortJ);U("uColor",pass.color);
 U("uEventStart",pass.role==4?permit_.eventStart:0);U("uM3EventStart",pass.role==4?permit_.eventStart:0);U("uEventCapacity",pass.role==4||pass.role==0?profile_->quantumFrameRows:profile_->scopeRows);U("uM3EventCapacity",pass.role==4||pass.role==0?profile_->quantumFrameRows:profile_->scopeRows);U("uM3EventWord",scope&&pass.role!=4?uint32_t(layout_.combinedEventOffset/4):0);
 glDispatchCompute((pass.count+63)/64,1,1);++evidence_.submittedPasses;Barrier();
 if(config_.fullBranchTrace&&pass.opcode==Opcode::SCOPE_FINALIZE){
  const uint32_t slot=(permit_.ordinal-1)*5+pass.role-1;
  Copy(pass.role==4?buffers_[8+permit_.eventArena]:buffers_[21],pass.role==4?size_t(permit_.eventStart)*64:layout_.combinedEventOffset/4,fullBuffers_[slot],0,size_t(profile_->scopeRows)*64);Barrier();pendingFullMask_|=1u<<slot;
 }
 if(pass.opcode==Opcode::PRIVATE_PROMOTE){
  Copy(buffers_[16],profile_->quantum[1].begin/4+6144,buffers_[25+permit_.eventArena],0,6*128);
  if(config_.fullBranchTrace)Copy(buffers_[16],profile_->quantum[1].begin/4+6144,fullBuffers_[20],size_t(permit_.ordinal-1)*2048,6*128);
  Barrier();
 }
}
TransportResultV4 TransportV4::Pump(){
 if(!busy_)return TransportResultV4::INVALID;if(!Current())return TransportResultV4::CONTEXT_LOST;
 if(!Allowed())return Denied();
 if(mapped_)return TransportResultV4::OK;
 const uint32_t count=permit_.kind==1?72:uint32_t(profile_->passCount);
 if(!fence_){
  const auto begin=Now();uint32_t submitted=0;
  while(nextPass_<count&&submitted<10&&Now()-begin<8000000){
   if(!Allowed())return Denied();
   if(permit_.kind==1)SourcePass(nextPass_);else QuantumPass(nextPass_);++nextPass_;++submitted;
   if(!Good())return TransportResultV4::GL_FAILED;
  }
  if(!Allowed())return Denied();glFlush();if(!Good())return TransportResultV4::GL_FAILED;
  if(nextPass_!=count)return TransportResultV4::IN_PROGRESS;
  if(!Allowed())return Denied();fence_=glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE,0);glFlush();if(!fence_||!Good())return TransportResultV4::GL_FAILED;
 }
 if(!Allowed())return Denied();
 const auto wait=glClientWaitSync(fence_,0,0);if(!Good()||wait==GL_WAIT_FAILED)return TransportResultV4::GL_FAILED;
 if(wait==GL_TIMEOUT_EXPIRED)return TransportResultV4::IN_PROGRESS;
 evidence_.fenceSignalledNs=Now();if(!Allowed())return Denied();
 glBindBuffer(GL_COPY_READ_BUFFER,buffers_[11+permit_.publicationSlot]);auto mapped=glMapBufferRange(GL_COPY_READ_BUFFER,0,1024,GL_MAP_READ_BIT);if(!mapped||!Good())return TransportResultV4::GL_FAILED;
 std::memcpy(tail_.data(),mapped,1024);const bool unmapped=glUnmapBuffer(GL_COPY_READ_BUFFER)==GL_TRUE;if(!unmapped||!Good())return TransportResultV4::GL_FAILED;
 glDeleteSync(fence_);fence_=nullptr;evidence_.uploadedBytes=uploadCounter_;evidence_.gl=rawGl_;evidence_.egl=rawEgl_==EGL_SUCCESS?0:uint32_t(rawEgl_);mapped_=true;return TransportResultV4::OK;
}
bool TransportV4::Commit(const RootPermitV4& permit,const Words256& tail){
 if(!busy_||!mapped_||permit.nonce!=permit_.nonce||tail!=tail_||tail[28]<1||tail[28]>3)return false;
 if(config_.fullBranchTrace){fullMask_|=pendingFullMask_;if(permit.kind==2)fullQuantumMask_|=1u<<(permit.ordinal-1);}
 currentBank_=permit.candidateBank;sourcePrefix_=tail[56];dedupPrefix_=tail[58];angle_=tail[107];busy_=mapped_=false;return true;
}
bool TransportV4::RetireRejected(const RootPermitV4& permit){
 if(!busy_||permit.nonce!=permit_.nonce)return false;
 if(mapped_){if(config_.fullBranchTrace&&tail_[28]==5){fullMask_|=pendingFullMask_;fullQuantumMask_|=1u<<(permit_.ordinal-1);}busy_=mapped_=false;return true;}
 if(!Current())return false;
 if(fence_){const auto result=glClientWaitSync(fence_,0,0);if(result!=GL_ALREADY_SIGNALED&&result!=GL_CONDITION_SATISFIED)return false;glDeleteSync(fence_);fence_=nullptr;}
 else if(nextPass_&&!mapped_){fence_=glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE,0);glFlush();return false;}
 busy_=mapped_=false;return Good();
}
void TransportV4::Release(bool current){
 current=current&&Current();if(current){if(fence_)glDeleteSync(fence_);glDeleteBuffers(GLsizei(buffers_.size()),buffers_.data());glDeleteBuffers(GLsizei(fullBuffers_.size()),fullBuffers_.data());}
 fence_=nullptr;buffers_={};fullBuffers_={};for(auto& p:programs_){if(p.id){programcache::Forget(p.id);if(current)glDeleteProgram(p.id);}p={};}
 ready_=busy_=mapped_=false;profile_=nullptr;old_=nullptr;context_=EGL_NO_CONTEXT;
}
bool TransportV4::CaptureColdV4(uint32_t* out,size_t capacity,const QuotaSnapshotV3& quota,size_t& stateWords,size_t& eventWords){
 stateWords=eventWords=0;cold::Layout layout;
 if(!ready_||busy_||!Current()||!out||!cold::Layout::Make(*profile_,alignment_,layout,config_.fullBranchTrace)||capacity<layout.words||tail_[0]!=1381192263||tail_[1]!=4||tail_[28]==4||InspectQuotaSnapshotV3(quota).result!=ControlResult::OK)return false;
 const uint32_t kind=tail_[27],rows=tail_[kind==1?169:53];
 if((kind!=1&&kind!=2)||tail_[kind==1?172:51]>1||rows>(kind==1?profile_->sourceRows:profile_->quantumFrameRows))return false;
 eventWords=size_t(rows)*(kind==1?128:64);stateWords=layout.events-256;
 const size_t used=layout.events+eventWords;if(used>capacity)return false;
 std::memset(out,0,used*4);std::memcpy(out,tail_.data(),1024);std::memcpy(out+layout.quota,quota.data(),1024);
 auto copy=[&](GLuint buffer,size_t offset,size_t words,size_t destination){
  if(!words)return true;if(offset>SIZE_MAX/4||words>SIZE_MAX/4||destination>used||words>used-destination)return false;
  glBindBuffer(GL_COPY_READ_BUFFER,buffer);auto mapped=glMapBufferRange(GL_COPY_READ_BUFFER,GLintptr(offset*4),GLsizeiptr(words*4),GL_MAP_READ_BIT);if(!mapped)return false;
  std::memcpy(out+destination,mapped,words*4);const bool unmap=glUnmapBuffer(GL_COPY_READ_BUFFER)==GL_TRUE;return unmap&&Good();
 };
 if(!copy(buffers_[currentBank_],0,profile_->stateBytes/4,layout.state))return false;
 if(kind==1){if(!copy(buffers_[15],profile_->source[2].begin/4,544,layout.input))return false;}
 else if(!copy(buffers_[25+tail_[51]],0,6*128,layout.control))return false;
 if(config_.fullBranchTrace){
  auto* meta=out+layout.branchMeta;meta[0]=0x3442464d;meta[1]=1;meta[2]=20;meta[3]=profile_->scopeRows;meta[4]=uint32_t(fullPlan_);meta[5]=uint32_t(fullPlan_>>32);meta[6]=fullMask_;meta[7]=fullQuantumMask_;
  for(uint32_t slot=0;slot<20;++slot)if(fullMask_&(1u<<slot))if(!copy(fullBuffers_[slot],0,size_t(profile_->scopeRows)*64,layout.branchEvents+size_t(slot)*profile_->scopeRows*64))return false;
  for(uint32_t q=0;q<4;++q)if(fullQuantumMask_&(1u<<q))if(!copy(fullBuffers_[20],q*2048,6*128,layout.branchControl+q*2048))return false;
 }
 return copy(buffers_[kind==1?22+tail_[172]:8+tail_[51]],0,eventWords,layout.events)&&Good();
}
}
