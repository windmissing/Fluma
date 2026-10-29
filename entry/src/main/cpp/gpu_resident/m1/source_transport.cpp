#include "source_transport.h"
#include "../foundation.h"
#include <cstring>
#include <chrono>
#include <limits>
namespace grs {namespace m1 {namespace {
uint64_t now(){return uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());}
bool hash(const char* s,const std::array<uint32_t,8>& a){if(std::strlen(s)!=64)return false;for(uint32_t i=0;i<32;i++){auto hex=[](char c)->int{return c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:-1;};int hi=hex(s[2*i]),lo=hex(s[2*i+1]);if(hi<0||lo<0||uint32_t(hi*16+lo)!=((a[i/4]>>(8*(i%4)))&255))return false;}return true;}
}
bool SourceCoreTransport::Current()const{return context!=EGL_NO_CONTEXT&&context==eglGetCurrentContext()&&display==eglGetCurrentDisplay()&&surface==eglGetCurrentSurface(EGL_DRAW)&&thread==std::this_thread::get_id();}
bool SourceCoreTransport::Good(){auto e=glGetError();if(e&&stats.firstGlError==0)stats.firstGlError=e;return e==GL_NO_ERROR;}
bool SourceCoreTransport::Set(const char* n,uint32_t v){auto p=glGetUniformLocation(program,n);if(p<0)return false;glUniform1ui(p,v);return true;}
bool SourceCoreTransport::Pair(const char* n,U64 v){auto p=glGetUniformLocation(program,n);if(p<0)return false;glUniform2ui(p,v.lo,v.hi);return true;}
TransportCode SourceCoreTransport::Create(uint32_t k,uint32_t kw,const Identity& id){
 if(context!=EGL_NO_CONTEXT||fence)return TransportCode::BUSY;if((k!=192&&k!=512)||(kw!=64&&kw!=128))return TransportCode::PROTOCOL_REJECTED;if(eglGetCurrentContext()==EGL_NO_CONTEXT)return TransportCode::NOT_READY;
 if(!hash(manifestSha,id.layoutSha)||!hash(SourceCoreSha(),id.shaderSha)||!hash(GetFoundationInfo().angleSha,id.angleSha))return TransportCode::PROTOCOL_REJECTED;
 context=eglGetCurrentContext();display=eglGetCurrentDisplay();surface=eglGetCurrentSurface(EGL_DRAW);thread=std::this_thread::get_id();identity=id;stats={};trace.Clear();K=k;Kw=kw;lastFrame={};arenaFrame[0]=arenaFrame[1]={};tail={};lost=frozen=fatal=false;
 auto fail=[&](TransportCode c){Release();return c;};GLint bindings=0,blocks=0,alignment=0,invocations=0,group=0;GLint64 limit=0;glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS,&bindings);glGetIntegerv(GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS,&blocks);glGetIntegerv(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT,&alignment);glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS,&invocations);glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE,0,&group);glGetInteger64v(GL_MAX_SHADER_STORAGE_BLOCK_SIZE,&limit);
 if(!Good()||bindings<4||blocks<4||alignment<=0||alignment%4||invocations<64||group<64||limit<=0)return fail(TransportCode::CAPABILITY_UNSUPPORTED);
 headerAt=16*size_t(K)+16*size_t(Kw)+16*128;words=headerAt+256;workWords=96+4096*64+words+4096*16;pubWords=16*size_t(K)+20*size_t(Kw)+16*128+256;
 for(size_t size:{words,workWords,pubWords,size_t(K+1)*64,size_t(256)})if(size>size_t(limit)/4||size>size_t(std::numeric_limits<GLsizeiptr>::max())/4)return fail(TransportCode::MEMORY_PROFILE_UNSUPPORTED);
 if(owner.Initialize(id,K,Kw)!=ControlCode::OK)return fail(TransportCode::PROTOCOL_REJECTED);
 auto sh=glCreateShader(GL_COMPUTE_SHADER);const char* src=SourceCoreKernel();glShaderSource(sh,1,&src,nullptr);glCompileShader(sh);GLint ok=0;glGetShaderiv(sh,GL_COMPILE_STATUS,&ok);if(!ok){glDeleteShader(sh);return fail(TransportCode::SHADER_FAILED);}program=glCreateProgram();glAttachShader(program,sh);glLinkProgram(program);glDeleteShader(sh);glGetProgramiv(program,GL_LINK_STATUS,&ok);if(!ok)return fail(TransportCode::SHADER_FAILED);
 auto alloc=[&](GLuint& b,size_t n,const void* data){glGenBuffers(1,&b);glBindBuffer(GL_SHADER_STORAGE_BUFFER,b);glBufferData(GL_SHADER_STORAGE_BUFFER,GLsizeiptr(n*4),data,GL_DYNAMIC_COPY);stats.allocatedBytes+=n*4;return b!=0&&Good();};std::vector<uint32_t> cold(words,0),workspace(workWords,0);std::copy(owner.Header().begin(),owner.Header().end(),cold.begin()+headerAt);
 for(size_t i=0;i<2;i++)if(!alloc(banks[i],words,cold.data())||!alloc(events[i],size_t(K+1)*64,nullptr)||!alloc(pubs[i],pubWords,nullptr)||!alloc(results[i],256,nullptr))return fail(TransportCode::MEMORY_PROFILE_UNSUPPORTED);
 if(!alloc(work,workWords,workspace.data()))return fail(TransportCode::MEMORY_PROFILE_UNSUPPORTED);stats.uploadedBytes=2*words*4+workWords*4;ready=true;return TransportCode::OK;
}
TransportCode SourceCoreTransport::Submit(const std::array<uint32_t,32>& command,RootRequest r){
 if(lost)return TransportCode::STATE_LOST;if(!Current())return TransportCode::WRONG_OWNER;if(!ready)return TransportCode::NOT_READY;if(fence||frozen)return TransportCode::BUSY;if(fatal)return TransportCode::PROTOCOL_REJECTED;Retire();if(fatal)return TransportCode::GL_FAILED;
 if(r.kind!=1||r.quantumOrdinal||r.eventArena>1||r.publicationSlot>1||Compare64(r.frame,lastFrame)<=0||r.eventArena!=(r.frame.lo&1u))return TransportCode::PROTOCOL_REJECTED;
 r.eventStart=0;r.eventCapacity=K+1;r.dispatchCount=8;r.barrierCount=9;r.uploadedBytes=128+12*4+3*8+8*4;
 auto code=owner.Begin(r,command);if(code==ControlCode::BACKPRESSURE||code==ControlCode::BUSY)return TransportCode::BUSY;if(code!=ControlCode::OK)return TransportCode::PROTOCOL_REJECTED;
 pending=r;lastFrame=r.frame;arenaFrame[r.eventArena]=r.frame;stats.firstGlError=0;glUseProgram(program);glBindBuffer(GL_SHADER_STORAGE_BUFFER,work);glBufferSubData(GL_SHADER_STORAGE_BUFFER,0,128,command.data());
 bool uniforms=Set("uK",K)&&Set("uKw",Kw)&&Set("uWater",16*K)&&Set("uColumn",16*K+16*Kw)&&Set("uHeader",uint32_t(headerAt))&&Set("uWords",uint32_t(words))&&Set("uEmitQ",1048576)&&Set("uBaseBank",owner.Bank())&&Set("uPublicationSlot",r.publicationSlot)&&Set("uEventArena",r.eventArena)&&Set("uCarry",r.carry)&&Set("uUploaded",r.uploadedBytes)&&Pair("uNonce",r.nonce)&&Pair("uFrame",r.frame)&&Pair("uControl",r.control);
 if(!uniforms){owner.Stop();fatal=true;return TransportCode::SHADER_FAILED;}stats.uploadedBytes+=r.uploadedBytes;
 for(uint32_t pass=0;pass<8;pass++){
  GLuint output=pass==5||pass==6?pubs[r.publicationSlot]:pass==7?results[r.publicationSlot]:events[r.eventArena];glBindBufferBase(GL_SHADER_STORAGE_BUFFER,0,banks[owner.Bank()]);glBindBufferBase(GL_SHADER_STORAGE_BUFFER,1,banks[1-owner.Bank()]);glBindBufferBase(GL_SHADER_STORAGE_BUFFER,2,work);glBindBufferBase(GL_SHADER_STORAGE_BUFFER,3,output);Set("uPass",pass);
  TransportTrace row;row.identity=identity;row.root=r;row.ordinal=pass;row.stage=pass;row.groups=uint32_t((words+63)/64);row.barrierBits=GL_SHADER_STORAGE_BARRIER_BIT;row.stateBytes=words*4;row.workspaceBytes=workWords*4;row.outputBytes=(pass==5||pass==6?pubWords:pass==7?256:size_t(K+1)*64)*4;row.beginNs=now();glDispatchCompute(row.groups,1,1);glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);stats.dispatches++;stats.barriers++;bool good=Good();row.endNs=now();row.rawGl=stats.firstGlError;trace.Push(row);if(!good){fatal=true;break;}
 }
 glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT|GL_SHADER_STORAGE_BARRIER_BIT);stats.barriers++;fence=glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE,0);stats.fences++;glFlush();if(!fence||!Good()){owner.Stop();fatal=true;return TransportCode::GL_FAILED;}return TransportCode::OK;
}
TransportCode SourceCoreTransport::Poll(AcceptedRoot& out){
 out={};if(lost)return TransportCode::STATE_LOST;if(!Current())return TransportCode::WRONG_OWNER;if(!fence)return TransportCode::NOT_READY;stats.polls++;auto f=glClientWaitSync(fence,0,0);if(f==GL_TIMEOUT_EXPIRED)return TransportCode::BUSY;if(f==GL_WAIT_FAILED){owner.Poll(FenceState::Failed,nullptr,0,stats.firstGlError,out);fatal=true;return TransportCode::GL_FAILED;}
 if(owner.State()==FlightState::Quarantined){owner.Poll(FenceState::Signaled,nullptr,0,stats.firstGlError,out);glDeleteSync(fence);fence=nullptr;return TransportCode::PROTOCOL_REJECTED;}
 glBindBuffer(GL_COPY_READ_BUFFER,results[pending.publicationSlot]);auto p=glMapBufferRange(GL_COPY_READ_BUFFER,0,1024,GL_MAP_READ_BIT);if(!p){owner.Poll(FenceState::Failed,nullptr,0,stats.firstGlError,out);fatal=true;return TransportCode::GL_FAILED;}std::memcpy(tail.data(),p,1024);stats.mappedBytes+=1024;bool good=glUnmapBuffer(GL_COPY_READ_BUFFER)==GL_TRUE&&Good();if(!good&&stats.firstGlError==0)stats.firstGlError=GL_INVALID_OPERATION;
 auto code=owner.Poll(FenceState::Signaled,tail.data(),256,stats.firstGlError,out);TransportTrace row;row.kind=TraceKind::Completion;row.identity=identity;row.root=pending;row.result=uint32_t(code);row.mappedBytes=1024;row.rawGl=stats.firstGlError;row.endNs=now();trace.Push(row);glDeleteSync(fence);fence=nullptr;if(code!=ControlCode::OK){fatal=true;return TransportCode::PROTOCOL_REJECTED;}return TransportCode::OK;
}
void SourceCoreTransport::Retire(){for(size_t i=0;i<2;i++)if(releases[i]){auto f=glClientWaitSync(releases[i],0,0);if(f==GL_WAIT_FAILED){fatal=true;return;}if(f==GL_ALREADY_SIGNALED||f==GL_CONDITION_SATISFIED){owner.Release(uint32_t(i),releaseGen[i],true);glDeleteSync(releases[i]);releases[i]=nullptr;}}}
ControlCode SourceCoreTransport::Acquire(uint32_t slot,U64 generation,PublicationLease& lease,GLuint& buffer){buffer=0;if(lost)return ControlCode::OWNER_STATE_LOST;if(!Current())return ControlCode::STALE;Retire();auto code=owner.Acquire(slot,generation,lease);if(code==ControlCode::OK)buffer=pubs[slot];return code;}
ControlCode SourceCoreTransport::ReleasePublication(uint32_t slot,U64 gen,GLsync f){if(!Current()||slot>1||!f||releases[slot])return ControlCode::INVALID_INPUT;auto c=owner.Release(slot,gen,false);if(c==ControlCode::QUARANTINED){releases[slot]=f;releaseGen[slot]=gen;return ControlCode::OK;}return c;}
ControlCode SourceCoreTransport::Stop(){fatal=true;return owner.Stop();} // Requires a new empty run after explicit stop.
void SourceCoreTransport::ContextLost(){owner.ContextLost();ready=false;lost=true;program=banks[0]=banks[1]=work=events[0]=events[1]=pubs[0]=pubs[1]=results[0]=results[1]=0;fence=releases[0]=releases[1]=nullptr;}
TransportCode SourceCoreTransport::Release(){if(context==EGL_NO_CONTEXT)return TransportCode::OK;if(!Current())return TransportCode::WRONG_OWNER;if(fence)return TransportCode::BUSY;Retire();if(releases[0]||releases[1]||owner.HasLeases())return TransportCode::BUSY;
 if(program)glDeleteProgram(program);glDeleteBuffers(2,banks);glDeleteBuffers(1,&work);glDeleteBuffers(2,events);glDeleteBuffers(2,pubs);glDeleteBuffers(2,results);program=banks[0]=banks[1]=work=events[0]=events[1]=pubs[0]=pubs[1]=results[0]=results[1]=0;ready=false;owner.ContextLost();context=EGL_NO_CONTEXT;return TransportCode::OK;}
TransportCode SourceCoreTransport::Freeze(bool value){if(!Current())return TransportCode::WRONG_OWNER;if(fence)return TransportCode::BUSY;frozen=value;return TransportCode::OK;}
TransportCode SourceCoreTransport::CaptureAccepted(std::vector<uint32_t>& out){if(!Current())return TransportCode::WRONG_OWNER;if(!ready||!frozen||fence)return TransportCode::BUSY;glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);stats.barriers++;glBindBuffer(GL_COPY_READ_BUFFER,banks[owner.Bank()]);auto p=glMapBufferRange(GL_COPY_READ_BUFFER,0,GLsizeiptr(words*4),GL_MAP_READ_BIT);if(!p)return TransportCode::GL_FAILED;out.resize(words);std::memcpy(out.data(),p,words*4);stats.coldMappedBytes+=words*4;bool ok=glUnmapBuffer(GL_COPY_READ_BUFFER)==GL_TRUE;return ok&&Good()?TransportCode::OK:TransportCode::GL_FAILED;}
}}

