#include "quantum_candidate.h"
#include "../angles_generated.h"
#include <cstring>
#include <chrono>
namespace grs {namespace m1 {
bool QuantumCandidate::Current()const{return context!=EGL_NO_CONTEXT&&eglGetCurrentContext()==context&&eglGetCurrentDisplay()==display&&eglGetCurrentSurface(EGL_DRAW)==surface&&thread==std::this_thread::get_id();}
void QuantumCandidate::Set(const char* name,uint32_t v){glUniform1ui(uniformLocations.Find(name),v);submittedUniformBytes+=4;}
void QuantumCandidate::Pass(uint32_t stage,uint32_t n){Set("uPass",stage);TransportTrace row;row.identity=traceIdentity;row.root=traceRoot;row.ordinal=dispatches;row.stage=stage;row.groups=(n+63)/64;row.barrierBits=GL_SHADER_STORAGE_BARRIER_BIT|GL_BUFFER_UPDATE_BARRIER_BIT;row.stateBytes=uint64_t(words)*4;row.workspaceBytes=uint64_t(workWords)*4;row.outputBytes=traceOutputBytes;auto ns=[](){return uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());};row.beginNs=ns();submitObserver.Emit(SubmitStage::DispatchEnter,((uint64_t(traceRoot.nonce.hi)<<32)|traceRoot.nonce.lo),0,dispatches);glDispatchCompute(row.groups,1,1);submitObserver.Emit(SubmitStage::DispatchExit,((uint64_t(traceRoot.nonce.hi)<<32)|traceRoot.nonce.lo),0,dispatches);glMemoryBarrier(row.barrierBits);row.rawGl=glGetError();if(row.rawGl&&!firstGlError)firstGlError=row.rawGl;row.endNs=ns();trace.Push(row);dispatches++;}
void QuantumCandidate::Sort(uint32_t stage,uint32_t n){for(uint32_t k=2;k<=n;k*=2)for(uint32_t j=k/2;j;j/=2){Set("uSortK",k);Set("uSortJ",j);Pass(stage,n);}}
uint32_t QuantumCandidate::ExpectedDispatches()const{auto l=[](uint32_t n){uint32_t v=0;while(n>1){v++;n/=2;}return v*(v+1)/2;};return 27+l(pad)+2*l(Kw);}
// T324 §2.1/INV-3: the planned pass table reproduces the ORIGINAL monolithic
// dispatch order exactly — the Enqueue body (stages 0..20) followed by the
// FullM1Transport::SubmitQuantum tail (stages 21..28 with binding/output
// switches). Same uniform set order, same barriers, same TestHook stages
// (hooks stay in the transport, fired at table boundaries 20/25/27).
// color=0xFFFFFFFF: no uColor. sortK=0: plain Pass. bind3/out: see header.
void QuantumCandidate::BuildPassTable(){
 passTable.clear();
 auto pass=[&](uint32_t stage,uint32_t n){passTable.push_back({stage,n,0xFFFFFFFFu,0,0,0,0});};
 auto colorPass=[&](uint32_t stage,uint32_t n,uint32_t color){passTable.push_back({stage,n,color,0,0,0,0});};
 auto sort=[&](uint32_t stage,uint32_t n){for(uint32_t k=2;k<=n;k*=2)for(uint32_t j=k/2;j;j/=2)passTable.push_back({stage,n,0xFFFFFFFFu,k,j,0,0});};
 pass(0,words);pass(1,1);
 for(uint32_t color=0;color<2;color++){colorPass(2,127,color);colorPass(3,1,color);colorPass(4,127,color);}
 pass(5,128);pass(6,pad);sort(7,pad);pass(8,Kw);sort(9,Kw);pass(10,1);pass(11,1);
 for(uint32_t color=0;color<3;color++){colorPass(12,127,color);colorPass(13,1,color);colorPass(14,127,color);}
 pass(15,1);pass(16,Kw);sort(17,Kw);pass(18,1);pass(19,1);pass(20,words);
 // SubmitQuantum tail: binding-3 and traceOutputBytes switches included.
 pass(21,1);pass(22,words);pass(23,1);pass(24,1);
 passTable.push_back({28,words,0xFFFFFFFFu,0,0,1,1});          // other bank, out=words
 passTable.push_back({25,1,0xFFFFFFFFu,0,0,2,2});             // publication, out=pub
 pass(26,1);
 passTable.push_back({27,1,0xFFFFFFFFu,0,0,3,3});             // results, out=1024
}
// Executes the planned slice with the original side effects. bind3 channels
// map to the buffers the transport staged at Enqueue time; the transport is
// a friend and pre-staged {otherBank,pub,results} before the first batch.
void QuantumCandidate::RunPassRange(uint32_t from,uint32_t to){
 for(uint32_t i=from;i<to&&i<passTable.size();i++){
  const auto& op=passTable[i];
  if(op.color!=0xFFFFFFFFu)Set("uColor",op.color);
  if(op.sortK){Set("uSortK",op.sortK);Set("uSortJ",op.sortJ);}
  if(op.bind3){glBindBufferBase(GL_SHADER_STORAGE_BUFFER,3,bindTargets[op.bind3-1]);}
  if(op.out==1)traceOutputBytes=uint64_t(words)*4;
  else if(op.out==2)traceOutputBytes=uint64_t(pubWords)*4;
  else if(op.out==3)traceOutputBytes=1024;
  if(op.stage==27){uploadedStage33=submittedUniformBytes+8;Set("uUploaded",uploadedStage33);}
  Pass(op.stage,op.n);
 }
}
TransportCode QuantumCandidate::Create(uint32_t k,uint32_t kw){if(context!=EGL_NO_CONTEXT)return TransportCode::BUSY;if((k!=192&&k!=512)||(kw!=64&&kw!=128))return TransportCode::PROTOCOL_REJECTED;if(eglGetCurrentContext()==EGL_NO_CONTEXT)return TransportCode::NOT_READY;
 allocated=0;lost=frozen=ready=false;context=eglGetCurrentContext();display=eglGetCurrentDisplay();surface=eglGetCurrentSurface(EGL_DRAW);thread=std::this_thread::get_id();K=k;Kw=kw;pad=K==192?256:512;water=16*K;column=water+16*Kw;header=column+2048;words=header+256;waterIndex=sandIndex+pad;arrivalIndex=waterIndex+Kw;edges=arrivalIndex+Kw;frozenAt=edges+127*16;rowsAt=frozenAt+512;angles=rowsAt+K+Kw;sourceEvents=angles+61444;workWords=sourceEvents+4096*16;eventCapacity=4*(48*K+16*Kw+254)+K;
 GLint blocks=0,bindings=0,inv=0,group=0;GLint64 limit=0;glGetIntegerv(GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS,&blocks);glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS,&bindings);glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS,&inv);glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE,0,&group);glGetInteger64v(GL_MAX_SHADER_STORAGE_BLOCK_SIZE,&limit);if(blocks<4||bindings<4||inv<64||group<64||limit<GLint64(eventCapacity)*256){Release();return TransportCode::CAPABILITY_UNSUPPORTED;}
 auto sh=glCreateShader(GL_COMPUTE_SHADER);const char* text=QuantumKernel();glShaderSource(sh,1,&text,nullptr);submitObserver.Emit(SubmitStage::ShaderEnter);glCompileShader(sh);GLint ok;glGetShaderiv(sh,GL_COMPILE_STATUS,&ok);submitObserver.Emit(SubmitStage::ShaderExit,0,uint32_t(ok));if(!ok){glDeleteShader(sh);Release();return TransportCode::SHADER_FAILED;}program=glCreateProgram();glAttachShader(program,sh);submitObserver.Emit(SubmitStage::ProgramEnter);glLinkProgram(program);glDeleteShader(sh);glGetProgramiv(program,GL_LINK_STATUS,&ok);submitObserver.Emit(SubmitStage::ProgramExit,0,uint32_t(ok));if(!ok){Release();return TransportCode::SHADER_FAILED;}
 SubmitScope resources(submitObserver,SubmitStage::ResourceEnter,SubmitStage::ResourceExit);auto alloc=[&](GLuint& b,uint32_t n){glGenBuffers(1,&b);glBindBuffer(GL_SHADER_STORAGE_BUFFER,b);glBufferData(GL_SHADER_STORAGE_BUFFER,GLsizeiptr(n)*4,nullptr,GL_DYNAMIC_COPY);allocated+=uint64_t(n)*4;};alloc(candidate,words);alloc(workspace,workWords);alloc(events,eventCapacity*64);glBindBuffer(GL_SHADER_STORAGE_BUFFER,workspace);glBufferSubData(GL_SHADER_STORAGE_BUFFER,angles*4,sizeof(kAngleWords),kAngleWords);if(glGetError()!=GL_NO_ERROR){Release();return TransportCode::GL_FAILED;}if(!uniformLocations.Prepare(program)){Release();return TransportCode::SHADER_FAILED;}resources.End(0);BuildPassTable();return TransportCode::OK;}
TransportCode QuantumCandidate::Enqueue(GLuint base,GLuint ledger,size_t offset,GLuint prefix,uint32_t count,const QuantumParameters& p,RootRequest r){if(r.kind!=2||r.quantumOrdinal<1||r.quantumOrdinal>4||(r.nonce.lo|r.nonce.hi)==0||(r.frame.lo|r.frame.hi)==0||(r.control.lo|r.control.hi)==0||count>eventCapacity||(r.eventCapacity&&(r.eventCapacity>eventCapacity||r.eventCapacity<count))||p.gravity<120||p.gravity>1073741824||p.settle==0||p.settle>1073741824||p.kAir>960||p.kFlight>960||p.kWet==0||p.kWet>960||p.kappa>960||p.soft>=605394||p.requestedAngle< -7680||p.requestedAngle>7680)return TransportCode::PROTOCOL_REJECTED;
 glBindBuffer(GL_COPY_READ_BUFFER,ledger);glBindBuffer(GL_COPY_WRITE_BUFFER,workspace);glCopyBufferSubData(GL_COPY_READ_BUFFER,GL_COPY_WRITE_BUFFER,GLintptr(offset*4),sourceEvents*4,4096*16*4);if(count&&prefix!=events){glBindBuffer(GL_COPY_READ_BUFFER,prefix);glBindBuffer(GL_COPY_WRITE_BUFFER,events);glCopyBufferSubData(GL_COPY_READ_BUFFER,GL_COPY_WRITE_BUFFER,0,0,count*64*4);}glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT|GL_SHADER_STORAGE_BARRIER_BIT);dispatches=0;trace.Clear();traceRoot=r;traceOutputBytes=uint64_t(eventCapacity)*256;firstGlError=0;submittedUniformBytes=0;ready=false;status={};glUseProgram(program);glBindBufferBase(GL_SHADER_STORAGE_BUFFER,0,base);glBindBufferBase(GL_SHADER_STORAGE_BUFFER,1,candidate);glBindBufferBase(GL_SHADER_STORAGE_BUFFER,2,workspace);glBindBufferBase(GL_SHADER_STORAGE_BUFFER,3,events);
 const std::pair<const char*,uint32_t> u[]={{"uK",K},{"uKw",Kw},{"uPad",pad},{"uWater",water},{"uColumn",column},{"uHeader",header},{"uWords",words},{"uSandIndex",sandIndex},{"uWaterIndex",waterIndex},{"uArrivalIndex",arrivalIndex},{"uEdges",edges},{"uFrozen",frozenAt},{"uRows",rowsAt},{"uAngles",angles},{"uSourceEvents",sourceEvents},{"uEventStart",count},{"uEventCapacity",r.eventCapacity?r.eventCapacity:eventCapacity},{"uGravity",p.gravity},{"uSettle",p.settle},{"uKAir",p.kAir},{"uKFlight",p.kFlight},{"uKWet",p.kWet},{"uKappa",p.kappa},{"uSoft",p.soft},{"uFlux",p.flux},{"uQuantumOrdinal",r.quantumOrdinal}};for(auto x:u)Set(x.first,x.second);glUniform1i(uniformLocations.Find("uRequestedAngle"),p.requestedAngle);glUniform2ui(uniformLocations.Find("uNonce"),r.nonce.lo,r.nonce.hi);glUniform2ui(uniformLocations.Find("uFrame"),r.frame.lo,r.frame.hi);glUniform2ui(uniformLocations.Find("uControl"),r.control.lo,r.control.hi);submittedUniformBytes+=28;
 // T324 §2.2: staging only — uploads, uniforms, per-root counters. The
 // dispatch loop (original Pass(0..20) inline) now executes from the pass
 // table via RunPassRange in transport batches; the table reproduces the
 // original order bit-for-bit (INV-3). Fence creation belongs to the last
 // batch (FENCE_PENDING) in the full transport; standalone submissions with
 // deferFence=false keep the original inline fence+flush semantics.
 if(!deferFence){submitObserver.Emit(SubmitStage::FenceEnter,((uint64_t(traceRoot.nonce.hi)<<32)|traceRoot.nonce.lo));fence=glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE,0);submitObserver.Emit(SubmitStage::FenceExit,((uint64_t(traceRoot.nonce.hi)<<32)|traceRoot.nonce.lo),fence?0:1);}glFlush();return (deferFence||fence)&&glGetError()==GL_NO_ERROR&&firstGlError==0?TransportCode::OK:TransportCode::GL_FAILED;
}
TransportCode QuantumCandidate::Submit(SourceCoreTransport& src,const QuantumParameters& p,RootRequest r){if(lost)return TransportCode::STATE_LOST;if(!Current())return TransportCode::WRONG_OWNER;if(fence||frozen||parent)return TransportCode::BUSY;if(src.K!=K||src.Kw!=Kw)return TransportCode::PROTOCOL_REJECTED;U64 previousFrame{src.owner.Header()[49],src.owner.Header()[50]};if(Compare64(r.frame,previousFrame)<0)return TransportCode::PROTOCOL_REJECTED;uint32_t prefix=Compare64(r.frame,previousFrame)==0?src.tail[53]:0;auto pin=src.PinCandidate();if(pin!=TransportCode::OK)return pin;parent=&src;auto result=Enqueue(src.banks[src.owner.Bank()],src.work,96+4096*64+src.words,src.events[src.pending.eventArena],prefix,p,r);if(result!=TransportCode::OK&&!fence){src.candidatePinned=false;parent=nullptr;}return result;}
TransportCode QuantumCandidate::Poll(){if(lost)return TransportCode::STATE_LOST;if(!Current())return TransportCode::WRONG_OWNER;if(!fence)return TransportCode::NOT_READY;auto f=glClientWaitSync(fence,0,0);if(f==GL_TIMEOUT_EXPIRED)return TransportCode::BUSY;if(f==GL_WAIT_FAILED)return TransportCode::GL_FAILED;glBindBuffer(GL_COPY_READ_BUFFER,workspace);void* p=glMapBufferRange(GL_COPY_READ_BUFFER,0,64,GL_MAP_READ_BIT);if(!p)return TransportCode::GL_FAILED;std::memcpy(status.data(),p,64);bool good=glUnmapBuffer(GL_COPY_READ_BUFFER)==GL_TRUE&&glGetError()==GL_NO_ERROR&&firstGlError==0;glDeleteSync(fence);fence=nullptr;bool stale=parent&&(parent->lost||parent->fatal);if(parent){parent->candidatePinned=false;parent=nullptr;}ready=good&&!stale&&status[0]==0&&status[12]==1;return !good?TransportCode::GL_FAILED:ready?TransportCode::OK:TransportCode::PROTOCOL_REJECTED;}
TransportCode QuantumCandidate::Freeze(bool on){if(!Current())return TransportCode::WRONG_OWNER;if(fence)return TransportCode::BUSY;frozen=on;return TransportCode::OK;}
TransportCode QuantumCandidate::Capture(std::vector<uint32_t>& out,std::vector<uint32_t>& log){if(!Current())return TransportCode::WRONG_OWNER;if(!frozen||fence)return TransportCode::BUSY;auto read=[](GLuint b,uint32_t n,std::vector<uint32_t>& v){if(n==0){v.clear();return true;}glBindBuffer(GL_COPY_READ_BUFFER,b);auto p=glMapBufferRange(GL_COPY_READ_BUFFER,0,GLsizeiptr(n)*4,GL_MAP_READ_BIT);if(!p)return false;v.resize(n);std::memcpy(v.data(),p,n*4);return glUnmapBuffer(GL_COPY_READ_BUFFER)==GL_TRUE;};return read(candidate,words,out)&&read(events,status[1]*64,log)&&glGetError()==GL_NO_ERROR?TransportCode::OK:TransportCode::GL_FAILED;}
TransportCode QuantumCandidate::Release(){if(context==EGL_NO_CONTEXT)return TransportCode::OK;if(!Current())return TransportCode::WRONG_OWNER;if(fence||parent)return TransportCode::BUSY;if(program)glDeleteProgram(program);glDeleteBuffers(1,&candidate);glDeleteBuffers(1,&workspace);glDeleteBuffers(1,&events);
#ifdef GRS_T279_HOST_TEST
 glDeleteBuffers(1,&fixtureBase);glDeleteBuffers(1,&fixtureLedger);fixtureBase=fixtureLedger=0;
#endif
 program=candidate=workspace=events=0;context=EGL_NO_CONTEXT;ready=false;return TransportCode::OK;}
void QuantumCandidate::ContextLost(){if(parent){parent->ContextLost();parent=nullptr;}context=EGL_NO_CONTEXT;fence=nullptr;program=candidate=workspace=events=0;lost=true;ready=false;}
#ifdef GRS_T279_HOST_TEST
TransportCode QuantumCandidate::Fixture(const std::vector<uint32_t>& state,const std::vector<uint32_t>& ledger,const QuantumParameters& p,RootRequest r,uint32_t cap){if(!Current())return TransportCode::WRONG_OWNER;if(fence||frozen||parent)return TransportCode::BUSY;if(state.size()!=words||ledger.size()!=4096*16)return TransportCode::PROTOCOL_REJECTED;auto upload=[](GLuint& b,const std::vector<uint32_t>& v){if(!b)glGenBuffers(1,&b);glBindBuffer(GL_SHADER_STORAGE_BUFFER,b);glBufferData(GL_SHADER_STORAGE_BUFFER,v.size()*4,v.data(),GL_DYNAMIC_COPY);};upload(fixtureBase,state);upload(fixtureLedger,ledger);r.eventCapacity=cap;return Enqueue(fixtureBase,fixtureLedger,0,0,0,p,r);}
#endif
}}
