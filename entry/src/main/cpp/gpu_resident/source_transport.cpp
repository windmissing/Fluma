#include "source_transport.h"
#include "foundation.h"
#include <vector>
#include <cstring>
#include <limits>
namespace grs {
namespace {
uint32_t Hex(char c){return c>='0'&&c<='9'?uint32_t(c-'0'):c>='a'&&c<='f'?uint32_t(c-'a'+10):0xffu;}
bool HashMatches(const char* s,const std::array<uint32_t,8>& words){for(uint32_t i=0;i<8;i++){uint32_t v=0;for(uint32_t j=0;j<4;j++){uint32_t hi=Hex(s[i*8+j*2]),lo=Hex(s[i*8+j*2+1]);if(hi>15||lo>15)return false;v|=(hi*16+lo)<<(j*8);}if(v!=words[i])return false;}return s[64]==0;}
void put(Words256& h,size_t i,U64 value){h[i]=value.lo;h[i+1]=value.hi;}
}
bool PlanSourceWorkspace(uint32_t K,size_t alignment,size_t maxBytes,SourceWorkspace& w,SourcePublication& p){
 StateSegments s;if(!PlanStateSegments(K,alignment,maxBytes,s))return false;size_t next=s.total;SourceWorkspace a;SourcePublication b;uint32_t pad=K==192?256:512;
 auto append=[&](size_t bytes,size_t& at){size_t aligned;if(!AlignUp(next,alignment,maxBytes,aligned)||bytes>maxBytes-aligned)return false;at=aligned/4;next=aligned+bytes;return true;};
 if(!append(128*K,a.motion)||!append(4*pad,a.indexA)||!append(4*pad,a.indexB)||!append(4*(K+64+128),a.scan)||!append(64*128,a.edges)||!append(16*128,a.audit)||!append(256*4096,a.dedup)||!append(64*4096,a.sourceEvents)||!append(128*16,a.command)||!append(2048,a.status)||!append(245776,a.angles)||!append(1024,a.stats)||!append(1024,a.terminal))return false;a.words=(next+3)/4;
 next=0;if(!append(64*K,b.sand)||!append(64*64,b.water)||!append(32*128,b.column)||!append(1024,b.header))return false;b.words=(next+3)/4;w=a;p=b;return true;
}
bool SourceGpuTransport::Current()const{return context!=EGL_NO_CONTEXT&&eglGetCurrentContext()==context&&eglGetCurrentDisplay()==display&&eglGetCurrentSurface(EGL_DRAW)==surface&&thread==std::this_thread::get_id();}
bool SourceGpuTransport::GlOK(){GLenum e=glGetError();if(e!=GL_NO_ERROR&&stats.firstGlError==0)stats.firstGlError=e;return e==GL_NO_ERROR;}
bool SourceGpuTransport::SetUniform(const char* name,uint32_t value){GLint at=glGetUniformLocation(program,name);if(at<0)return false;glUniform1ui(at,value);return true;}
void SourceGpuTransport::SetPair(const char* name,U64 value){GLint at=glGetUniformLocation(program,name);if(at>=0)glUniform2ui(at,value.lo,value.hi);}
TransportCode SourceGpuTransport::Create(uint32_t capacity,const Identity& id){
 if(ready||fence)return TransportCode::BUSY;if(capacity!=192&&capacity!=512)return TransportCode::PROTOCOL_REJECTED;
 if(eglGetCurrentContext()==EGL_NO_CONTEXT)return TransportCode::NOT_READY;
 input={1,0};lastSourceFrame={};lastQuantumFrame={};lastQuantumOrdinal=0;submittedAny=diagnosticFreeze=fatalFrozen=false;arenaFrames[0]=arenaFrames[1]={};arenaUsed[0]=arenaUsed[1]=0;lastCompletion={};context=eglGetCurrentContext();display=eglGetCurrentDisplay();surface=eglGetCurrentSurface(EGL_DRAW);thread=std::this_thread::get_id();stats={};K=capacity;pad=K==192?256:512;identity=id;
 auto info=GetFoundationInfo();if(!HashMatches(info.layoutSha,id.layoutSha)||!HashMatches(info.angleSha,id.angleSha)||!HashMatches(SourceKernelSha(),id.shaderSha))return TransportCode::PROTOCOL_REJECTED;
 GLint global=0,stage=0,align=0,invocations=0,group=0;GLint64 block=0;
 glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS,&global);glGetIntegerv(GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS,&stage);glGetIntegerv(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT,&align);glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS,&invocations);glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE,0,&group);glGetInteger64v(GL_MAX_SHADER_STORAGE_BLOCK_SIZE,&block);
 if(!GlOK()||global<4||stage<4||invocations<64||group<64||align<=0||align%4!=0||block<=0)return TransportCode::CAPABILITY_UNSUPPORTED;
 if(!PlanStateSegments(K,size_t(align),size_t(block),segments)||!PlanSourceWorkspace(K,size_t(align),size_t(block),workspace,publication))return TransportCode::MEMORY_PROFILE_UNSUPPORTED;
 GLuint shader=glCreateShader(GL_COMPUTE_SHADER);const char* source=SourceKernelSource();glShaderSource(shader,1,&source,nullptr);glCompileShader(shader);GLint good=0;glGetShaderiv(shader,GL_COMPILE_STATUS,&good);if(!good){glDeleteShader(shader);return TransportCode::SHADER_FAILED;}
 program=glCreateProgram();glAttachShader(program,shader);glLinkProgram(program);glDeleteShader(shader);glGetProgramiv(program,GL_LINK_STATUS,&good);if(!good){glDeleteProgram(program);program=0;return TransportCode::SHADER_FAILED;}
 auto allocate=[&](GLuint& buffer,size_t bytes,const void* data){if(bytes>size_t(block)||bytes>size_t(std::numeric_limits<GLsizeiptr>::max()))return false;glGenBuffers(1,&buffer);glBindBuffer(GL_SHADER_STORAGE_BUFFER,buffer);glBufferData(GL_SHADER_STORAGE_BUFFER,GLsizeiptr(bytes),data,GL_DYNAMIC_COPY);if(!GlOK())return false;stats.allocatedBytes+=bytes;return true;};
 Words256 h{};h[0]=wire::magicWords::stateHeader;h[1]=1;h[2]=256;put(h,4,id.run);put(h,6,id.owner);put(h,8,id.context);put(h,10,id.surface);put(h,20,id.geometry);put(h,22,id.config);for(uint32_t i=24;i<=31;i++)h[i]=1;h[39]=h[40]=1;h[43]=K;h[44]=64;h[77]=1;h[78]=2;put(h,84,input);for(uint32_t i=0;i<8;i++){h[88+i]=id.shaderSha[i];h[96+i]=id.angleSha[i];h[104+i]=id.layoutSha[i];}
 if(owner.Initialize(id,ProductCompletionPolicy(),h,1)!=ControlCode::OK)return TransportCode::PROTOCOL_REJECTED;
 std::vector<uint32_t> initial(segments.total/4,0);std::copy(h.begin(),h.end(),initial.begin()+segments.header/4);
 bool allocated=allocate(banks[0],segments.total,initial.data())&&allocate(banks[1],segments.total,initial.data())&&allocate(work,workspace.words*4,nullptr);
 size_t arenaBytes=256*size_t(133*K+508);for(int i=0;i<2&&allocated;i++)allocated=allocate(events[i],arenaBytes,nullptr)&&allocate(pubs[i],publication.words*4,nullptr)&&allocate(results[i],1024,nullptr);
 if(!allocated){Release();return TransportCode::MEMORY_PROFILE_UNSUPPORTED;}
 size_t angleCount=0;const uint32_t* angle=ColdAngleWords(angleCount);glBindBuffer(GL_SHADER_STORAGE_BUFFER,work);glBufferSubData(GL_SHADER_STORAGE_BUFFER,GLintptr(workspace.angles*4),GLsizeiptr(angleCount*4),angle);stats.uploadedBytes+=angleCount*4+2*segments.total;
 if(!GlOK()){Release();return TransportCode::GL_FAILED;}ready=true;lost=false;return TransportCode::OK;
}
TransportCode SourceGpuTransport::Submit(const std::array<uint32_t,32>& command,RootRequest r){r.kind=1;r.quantumOrdinal=0;return SubmitRoot(&command,r,0);}
TransportCode SourceGpuTransport::SubmitQuantum(RootRequest r,int angle){r.kind=2;return SubmitRoot(nullptr,r,angle);}
TransportCode SourceGpuTransport::SubmitRoot(const std::array<uint32_t,32>* command,RootRequest r,int angle){
 if(lost)return TransportCode::STATE_LOST;if(!ready)return TransportCode::NOT_READY;if(!Current())return TransportCode::WRONG_OWNER;if(fence||diagnosticFreeze)return TransportCode::BUSY;if(fatalFrozen)return TransportCode::PROTOCOL_REJECTED;RetireReleases();
 if(r.kind==1&&(submittedAny&&Compare64(r.frame,lastSourceFrame)<=0||Compare64(r.frame,lastQuantumFrame)<=0))return TransportCode::PROTOCOL_REJECTED;
 if(r.kind==2&&(r.quantumOrdinal==0||r.quantumOrdinal>4||angle< -7680||angle>7680||Compare64(r.frame,lastSourceFrame)<0||Compare64(r.frame,lastQuantumFrame)<0||(Compare64(r.frame,lastQuantumFrame)==0?r.quantumOrdinal!=lastQuantumOrdinal+1:r.quantumOrdinal!=1)))return TransportCode::PROTOCOL_REJECTED;
 if(r.eventArena>=2||r.publicationSlot>=2)return TransportCode::PROTOCOL_REJECTED;
 if(Compare64(r.frame,arenaFrames[r.eventArena])<0)return TransportCode::PROTOCOL_REJECTED;
 r.eventStart=Compare64(r.frame,arenaFrames[r.eventArena])==0?arenaUsed[r.eventArena]:0;
 auto plan=r.kind==1?SourcePlan(K):QuantumPlan(K);r.dispatchCount=plan.count;r.barrierCount=plan.count+1;r.uploadedBytes=command?128:0;

 uint32_t uniformBytes=0;
 if(command){glBindBuffer(GL_SHADER_STORAGE_BUFFER,work);glBufferSubData(GL_SHADER_STORAGE_BUFFER,GLintptr(workspace.command*4),128,command->data());stats.uploadedBytes+=128;}glUseProgram(program);
 auto set=[&](const char* n,size_t v){GLint at=glGetUniformLocation(program,n);if(at>=0){glUniform1ui(at,uint32_t(v));uniformBytes+=4;}};
 set("uRootKind",r.kind);set("uQuantumOrdinal",r.quantumOrdinal);set("uMotion",workspace.motion);set("uEdges",workspace.edges);set("uGravity",10276045);set("uDamping",1048576);set("uFlux",25165824);size_t ac=0;set("uSoft",ColdAngleWords(ac)[(7680+7168)*4]);glUniform1i(glGetUniformLocation(program,"uRequestedAngle"),angle);uniformBytes+=4;
 set("uK",K);set("uPad",pad);set("uStateWords",segments.total/4);set("uHeader",segments.header/4);set("uColumn",segments.column/4);set("uScan",workspace.scan);set("uCommand",workspace.command);set("uDedup",workspace.dedup);set("uSourceEvents",workspace.sourceEvents);set("uStatus",workspace.status);set("uTerminal",workspace.terminal);set("uSpare",workspace.spare);set("uAngles",workspace.angles);set("uPackage",131072);set("uEventStart",r.eventStart);set("uEventCapacity",r.eventCapacity);set("uEventArena",r.eventArena);set("uPublicationSlot",r.publicationSlot);set("uPubHeader",publication.header);set("uPubSand",publication.sand);set("uPubWater",publication.water);set("uPubColumn",publication.column);set("uActualDispatches",r.dispatchCount);set("uActualBarriers",r.barrierCount);set("uActualUploadedBytes",r.uploadedBytes);set("uCarry",r.carry);
 SetPair("uRootNonce",r.nonce);SetPair("uFrameSerial",r.frame);SetPair("uControlSerial",r.control);SetPair("uInputGeneration",input);SetPair("uGeometryEpoch",identity.geometry);uniformBytes+=5*8;glUniform3ui(glGetUniformLocation(program,"uBanks"),owner.Bank(),1-owner.Bank(),2);uniformBytes+=12;
 uint32_t perPassBytes=0;for(const char* name:{"uPass","uScanWidth","uIndexA","uIndexB","uSortK","uSortJ","uColor"})if(glGetUniformLocation(program,name)>=0)perPassBytes+=4;
 // Include command plus actual scalar/vector uniform payload; no hidden per-pass control upload.
 r.uploadedBytes+=uniformBytes+4+perPassBytes*plan.count;set("uActualUploadedBytes",r.uploadedBytes);stats.uploadedBytes+=uniformBytes+perPassBytes*plan.count;
 auto beginCode=owner.Begin(r);if(beginCode==ControlCode::BUSY||beginCode==ControlCode::BACKPRESSURE)return TransportCode::BUSY;if(beginCode!=ControlCode::OK)return TransportCode::PROTOCOL_REJECTED;if(Compare64(r.frame,arenaFrames[r.eventArena])>0){arenaFrames[r.eventArena]=r.frame;arenaUsed[r.eventArena]=0;}pending=r;pendingSlot=r.publicationSlot;if(r.kind==1){lastSourceFrame=r.frame;submittedAny=true;}else{lastQuantumFrame=r.frame;lastQuantumOrdinal=r.quantumOrdinal;}stats.firstGlError=0;
 size_t readIndex=workspace.indexA,writeIndex=workspace.indexB;
 for(uint32_t i=0;i<plan.count;i++){
  const auto& pass=plan.passes[i];GLuint output=pass.stage==Pack||pass.stage==PackAudit?pubs[pendingSlot]:pass.stage==Seal?results[pendingSlot]:events[r.eventArena];
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER,0,banks[owner.Bank()]);glBindBufferBase(GL_SHADER_STORAGE_BUFFER,1,banks[1-owner.Bank()]);glBindBufferBase(GL_SHADER_STORAGE_BUFFER,2,work);glBindBufferBase(GL_SHADER_STORAGE_BUFFER,3,output);set("uPass",pass.stage);set("uScanWidth",pass.k);set("uIndexA",readIndex);set("uIndexB",writeIndex);set("uSortK",pass.k);set("uSortJ",pass.j);set("uColor",pass.color);
  glDispatchCompute(GLuint((segments.total/4+63)/64),1,1);stats.dispatches++;glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);stats.barriers++;if(!GlOK())break;if(pass.stage==Sort)std::swap(readIndex,writeIndex);
 }
 glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT|GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT|GL_PIXEL_BUFFER_BARRIER_BIT);stats.barriers++;fence=glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE,0);stats.fences++;glFlush();if(!GlOK()||!fence){owner.Stop();return TransportCode::GL_FAILED;}return TransportCode::OK;
}
TransportCode SourceGpuTransport::Poll(AcceptedRoot& out){
 out={};if(lost)return TransportCode::STATE_LOST;if(!Current())return TransportCode::WRONG_OWNER;if(!fence)return TransportCode::NOT_READY;stats.polls++;
 GLenum state=glClientWaitSync(fence,0,0);if(state==GL_TIMEOUT_EXPIRED)return TransportCode::BUSY;
 if(state==GL_WAIT_FAILED){owner.Poll(FenceState::Failed,nullptr,0,stats.firstGlError,out);return TransportCode::GL_FAILED;}
 if(owner.State()==FlightState::Quarantined){owner.Poll(FenceState::Signaled,nullptr,0,stats.firstGlError,out);glDeleteSync(fence);fence=nullptr;return TransportCode::PROTOCOL_REJECTED;}
 Words256 completion{};glBindBuffer(GL_COPY_READ_BUFFER,results[pendingSlot]);void* data=glMapBufferRange(GL_COPY_READ_BUFFER,0,1024,GL_MAP_READ_BIT);if(!data){owner.Poll(FenceState::Failed,nullptr,0,stats.firstGlError,out);return TransportCode::GL_FAILED;}
 std::memcpy(completion.data(),data,1024);lastCompletion=completion;stats.mappedBytes+=1024;bool unmapped=glUnmapBuffer(GL_COPY_READ_BUFFER)==GL_TRUE;if(!unmapped||!GlOK()){if(stats.firstGlError==0)stats.firstGlError=GL_INVALID_OPERATION;}
 auto code=owner.Poll(FenceState::Signaled,completion.data(),256,stats.firstGlError,out);if(code==ControlCode::OK)arenaUsed[pending.eventArena]=completion[completion256::eventEnd];else fatalFrozen=true;glDeleteSync(fence);fence=nullptr;return code==ControlCode::OK?TransportCode::OK:TransportCode::PROTOCOL_REJECTED;
}
TransportCode SourceGpuTransport::FreezeDiagnostics(bool freeze){if(!Current())return TransportCode::WRONG_OWNER;if(fence)return TransportCode::BUSY;diagnosticFreeze=freeze;return TransportCode::OK;}
TransportCode SourceGpuTransport::CaptureAcceptedState(std::vector<uint32_t>& words){
 if(!Current())return TransportCode::WRONG_OWNER;if(!diagnosticFreeze||fence||!ready)return TransportCode::BUSY;
 glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);stats.barriers++;glBindBuffer(GL_COPY_READ_BUFFER,banks[owner.Bank()]);void* data=glMapBufferRange(GL_COPY_READ_BUFFER,0,GLsizeiptr(segments.total),GL_MAP_READ_BIT);if(!data)return TransportCode::GL_FAILED;
 words.resize(segments.total/4);std::memcpy(words.data(),data,segments.total);stats.coldMappedBytes+=segments.total;bool unmapped=glUnmapBuffer(GL_COPY_READ_BUFFER)==GL_TRUE;return unmapped&&GlOK()?TransportCode::OK:TransportCode::GL_FAILED;
}
ControlCode SourceGpuTransport::Stop(){U64 next;if(!Add64(input,{1,0},next))return ControlCode::TOKEN_EXHAUSTED;input=next;return owner.Stop();}
void SourceGpuTransport::ContextLost(){owner.ContextLost();ready=false;lost=true;program=banks[0]=banks[1]=work=events[0]=events[1]=pubs[0]=pubs[1]=results[0]=results[1]=0;fence=nullptr;releases[0]=releases[1]=nullptr;}
TransportCode SourceGpuTransport::Release(){if(context==EGL_NO_CONTEXT)return TransportCode::OK;if(!Current())return TransportCode::WRONG_OWNER;if(fence)return TransportCode::BUSY;RetireReleases();for(auto sync:releases)if(sync)return TransportCode::BUSY;if(program)glDeleteProgram(program);glDeleteBuffers(2,banks);glDeleteBuffers(1,&work);glDeleteBuffers(2,events);glDeleteBuffers(2,pubs);glDeleteBuffers(2,results);program=banks[0]=banks[1]=work=events[0]=events[1]=pubs[0]=pubs[1]=results[0]=results[1]=0;ready=false;context=EGL_NO_CONTEXT;return TransportCode::OK;}
ControlCode SourceGpuTransport::Acquire(uint32_t slot,U64 generation,PublicationLease& lease,GLuint& buffer){if(!Current())return ControlCode::STALE;RetireReleases();auto code=owner.Acquire(slot,generation,lease);if(code==ControlCode::OK)buffer=pubs[slot];return code;}
ControlCode SourceGpuTransport::ReleasePublication(uint32_t slot,U64 generation,GLsync sync){if(!Current()||slot>=2||!sync)return ControlCode::INVALID_INPUT;auto code=owner.Release(slot,generation,false);if(code==ControlCode::OK){releases[slot]=sync;releaseGenerations[slot]=generation;}return code;}
void SourceGpuTransport::RetireReleases(){for(uint32_t i=0;i<2;i++)if(releases[i]){GLenum state=glClientWaitSync(releases[i],0,0);if(state==GL_ALREADY_SIGNALED||state==GL_CONDITION_SATISFIED){owner.RetireRelease(i,releaseGenerations[i],true);
 glDeleteSync(releases[i]);releases[i]=nullptr;}}}
}
